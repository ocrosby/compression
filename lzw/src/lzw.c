#include "lzw.h"

#include <string.h>

#include "bitstream.h"
#include "hdr.h"

#define LZW_CODE_BITS   12
#define LZW_MAX_CODES   (1u << LZW_CODE_BITS)   /* 4096 */
#define LZW_FIRST_CODE  256u                    /* 0..255 are literals */
#define LZW_HDR_BYTES   COMPRESSION_HDR_BYTES   /* little-endian orig_len */
#define LZW_EMPTY_CODE  0xFFFFu                 /* sentinel: hash slot empty */

/* Prime > LZW_MAX_CODES; ~76% load factor with linear probing. */
#define LZW_HASH_SIZE   5381u

typedef struct {
    uint16_t prefix;
    uint16_t code;    /* LZW_EMPTY_CODE marks an empty slot */
    uint8_t  byte;
} hash_entry_t;

static size_t hash_of(uint16_t prefix, uint8_t byte) {
    return (((size_t)prefix * 31u) + byte) % LZW_HASH_SIZE;
}

static uint16_t hash_lookup(const hash_entry_t *tbl,
                            uint16_t prefix, uint8_t byte) {
    size_t h = hash_of(prefix, byte);
    for (;;) {
        if (tbl[h].code == LZW_EMPTY_CODE) return LZW_EMPTY_CODE;
        if (tbl[h].prefix == prefix && tbl[h].byte == byte) return tbl[h].code;
        h = (h + 1u) % LZW_HASH_SIZE;
    }
}

static void hash_insert(hash_entry_t *tbl,
                        uint16_t prefix, uint8_t byte, uint16_t code) {
    size_t h = hash_of(prefix, byte);
    while (tbl[h].code != LZW_EMPTY_CODE) h = (h + 1u) % LZW_HASH_SIZE;
    tbl[h].prefix = prefix;
    tbl[h].byte   = byte;
    tbl[h].code   = code;
}

size_t lzw_encode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (out_cap < LZW_HDR_BYTES) return LZW_ERROR;

    hdr_write_len(out, in_len);

    hash_entry_t table[LZW_HASH_SIZE];
    memset(table, 0xFF, sizeof table);   /* sets code = LZW_EMPTY_CODE */

    bit_writer_t bw;
    bit_writer_init(&bw, out + LZW_HDR_BYTES, out_cap - LZW_HDR_BYTES);
    uint16_t next_code = (uint16_t)LZW_FIRST_CODE;
    uint16_t w = in[0];  /* prefix code; literal for first byte */

    for (size_t i = 1; i < in_len; i++) {
        uint8_t c = in[i];
        uint16_t found = hash_lookup(table, w, c);
        if (found != LZW_EMPTY_CODE) {
            w = found;
            continue;
        }
        if (bit_writer_put(&bw, w, LZW_CODE_BITS) != 0) return LZW_ERROR;
        if (next_code < LZW_MAX_CODES) {
            hash_insert(table, w, c, next_code++);
        }
        w = c;
    }
    if (bit_writer_put(&bw, w, LZW_CODE_BITS) != 0) return LZW_ERROR;

    return LZW_HDR_BYTES + bit_writer_bytes(&bw);
}

size_t lzw_decode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (in_len < LZW_HDR_BYTES) return LZW_ERROR;

    size_t orig_len = hdr_read_len(in);
    if (orig_len > out_cap) return LZW_ERROR;
    if (orig_len == 0) return 0;

    bit_reader_t br;
    bit_reader_init(&br, in + LZW_HDR_BYTES, in_len - LZW_HDR_BYTES);

    /* Decoder dictionary: parent-link chain from a code to its parts.
     * For literal codes 0..255 we treat byte_of[k] == k so string walks
     * bottom out cleanly at the single-byte root. */
    uint16_t prefix[LZW_MAX_CODES];
    uint8_t  byte_of[LZW_MAX_CODES];
    for (int i = 0; i < 256; i++) byte_of[i] = (uint8_t)i;
    uint16_t next_code = (uint16_t)LZW_FIRST_CODE;

    uint32_t k32;
    if (bit_reader_get(&br, LZW_CODE_BITS, &k32) != 0) return LZW_ERROR;
    if (k32 >= LZW_FIRST_CODE) return LZW_ERROR;  /* first code must be a literal */

    size_t out_pos = 0;
    out[out_pos++] = (uint8_t)k32;
    uint16_t k = (uint16_t)k32;
    uint16_t prev = k;
    uint8_t  first_of_prev = (uint8_t)k;

    uint8_t stack[LZW_MAX_CODES];

    while (out_pos < orig_len) {
        if (bit_reader_get(&br, LZW_CODE_BITS, &k32) != 0) return LZW_ERROR;
        k = (uint16_t)k32;

        uint8_t first_byte;
        if (k < next_code) {
            /* Known code: walk its chain into `stack`, then emit reversed. */
            size_t sp = 0;
            uint16_t w = k;
            for (;;) {
                stack[sp++] = byte_of[w];
                if (w < LZW_FIRST_CODE) break;
                w = prefix[w];
                if (sp >= LZW_MAX_CODES) return LZW_ERROR;  /* corrupt chain */
            }
            first_byte = stack[sp - 1];
            if (out_pos + sp > orig_len) return LZW_ERROR;
            while (sp > 0) out[out_pos++] = stack[--sp];
        } else if (k == next_code) {
            /* KwKwK: the string is prev's string + first_of_prev.
             * Walk `prev` (known), then append first_of_prev. */
            size_t sp = 0;
            uint16_t w = prev;
            for (;;) {
                stack[sp++] = byte_of[w];
                if (w < LZW_FIRST_CODE) break;
                w = prefix[w];
                if (sp >= LZW_MAX_CODES) return LZW_ERROR;
            }
            first_byte = stack[sp - 1];
            if (out_pos + sp + 1u > orig_len) return LZW_ERROR;
            while (sp > 0) out[out_pos++] = stack[--sp];
            out[out_pos++] = first_of_prev;
        } else {
            return LZW_ERROR;
        }

        if (next_code < LZW_MAX_CODES) {
            prefix[next_code]  = prev;
            byte_of[next_code] = first_byte;
            next_code++;
        }
        prev = k;
        first_of_prev = first_byte;
    }
    return out_pos;
}
