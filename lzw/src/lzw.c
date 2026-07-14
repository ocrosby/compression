#include "lzw.h"

#include <string.h>

#define LZW_CODE_BITS   12
#define LZW_MAX_CODES   (1u << LZW_CODE_BITS)   /* 4096 */
#define LZW_FIRST_CODE  256u                    /* 0..255 are literals */
#define LZW_HDR_BYTES   8u                      /* little-endian orig_len */
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

/* Pack 12 bits of `code` MSB-first into `body` starting at *bit_pos.
 * Returns 0 on success, non-zero if the write would exceed body_cap. */
static int pack_code(uint8_t *body, size_t *bit_pos, size_t body_cap,
                     uint16_t code) {
    size_t end_bits = *bit_pos + LZW_CODE_BITS;
    if ((end_bits + 7u) / 8u > body_cap) return -1;
    for (int b = LZW_CODE_BITS - 1; b >= 0; b--) {
        size_t bp = (*bit_pos)++;
        if ((bp % 8u) == 0u) body[bp / 8u] = 0u;
        if ((code >> b) & 1u) {
            body[bp / 8u] |= (uint8_t)(1u << (7u - (bp % 8u)));
        }
    }
    return 0;
}

/* Read 12 bits MSB-first from `body` starting at *bit_pos into *code.
 * Returns 0 on success, non-zero if there aren't enough remaining bits. */
static int read_code(const uint8_t *body, size_t body_bits,
                     size_t *bit_pos, uint16_t *code) {
    if (*bit_pos + LZW_CODE_BITS > body_bits) return -1;
    uint16_t c = 0;
    for (int b = 0; b < LZW_CODE_BITS; b++) {
        size_t bp = (*bit_pos)++;
        int bit = (body[bp / 8u] >> (7u - (bp % 8u))) & 1;
        c = (uint16_t)((c << 1) | (uint16_t)bit);
    }
    *code = c;
    return 0;
}

size_t lzw_encode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (out_cap < LZW_HDR_BYTES) return LZW_ERROR;

    for (size_t i = 0; i < 8; i++) {
        out[i] = (uint8_t)((in_len >> (i * 8)) & 0xFFu);
    }

    hash_entry_t table[LZW_HASH_SIZE];
    memset(table, 0xFF, sizeof table);   /* sets code = LZW_EMPTY_CODE */

    uint8_t *body = out + LZW_HDR_BYTES;
    size_t   body_cap = out_cap - LZW_HDR_BYTES;
    size_t   bit_pos = 0;
    uint16_t next_code = (uint16_t)LZW_FIRST_CODE;
    uint16_t w = in[0];  /* prefix code; literal for first byte */

    for (size_t i = 1; i < in_len; i++) {
        uint8_t c = in[i];
        uint16_t found = hash_lookup(table, w, c);
        if (found != LZW_EMPTY_CODE) {
            w = found;
            continue;
        }
        if (pack_code(body, &bit_pos, body_cap, w) != 0) return LZW_ERROR;
        if (next_code < LZW_MAX_CODES) {
            hash_insert(table, w, c, next_code++);
        }
        w = c;
    }
    if (pack_code(body, &bit_pos, body_cap, w) != 0) return LZW_ERROR;

    return LZW_HDR_BYTES + (bit_pos + 7u) / 8u;
}

size_t lzw_decode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (in_len < LZW_HDR_BYTES) return LZW_ERROR;

    size_t orig_len = 0;
    for (size_t i = 0; i < 8; i++) orig_len |= (size_t)in[i] << (i * 8);
    if (orig_len > out_cap) return LZW_ERROR;
    if (orig_len == 0) return 0;

    const uint8_t *body = in + LZW_HDR_BYTES;
    size_t body_bits = (in_len - LZW_HDR_BYTES) * 8u;
    size_t bit_pos = 0;

    /* Decoder dictionary: parent-link chain from a code to its parts.
     * For literal codes 0..255 we treat byte_of[k] == k so string walks
     * bottom out cleanly at the single-byte root. */
    uint16_t prefix[LZW_MAX_CODES];
    uint8_t  byte_of[LZW_MAX_CODES];
    for (int i = 0; i < 256; i++) byte_of[i] = (uint8_t)i;
    uint16_t next_code = (uint16_t)LZW_FIRST_CODE;

    uint16_t k;
    if (read_code(body, body_bits, &bit_pos, &k) != 0) return LZW_ERROR;
    if (k >= LZW_FIRST_CODE) return LZW_ERROR;  /* first code must be a literal */

    size_t out_pos = 0;
    out[out_pos++] = (uint8_t)k;
    uint16_t prev = k;
    uint8_t  first_of_prev = (uint8_t)k;

    uint8_t stack[LZW_MAX_CODES];

    while (out_pos < orig_len) {
        if (read_code(body, body_bits, &bit_pos, &k) != 0) return LZW_ERROR;

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
