#include "lzss.h"

#include <string.h>

#include "hdr.h"

#define LZSS_HDR_BYTES   COMPRESSION_HDR_BYTES

#define LZSS_WINDOW_BITS 12
#define LZSS_WINDOW_SIZE (1u << LZSS_WINDOW_BITS)   /* 4096 */
#define LZSS_WINDOW_MASK (LZSS_WINDOW_SIZE - 1u)

#define LZSS_LENGTH_BITS 4
#define LZSS_MIN_MATCH   3u
#define LZSS_MAX_MATCH   (LZSS_MIN_MATCH + (1u << LZSS_LENGTH_BITS) - 1u)  /* 18 */

#define LZSS_HASH_BITS   12
#define LZSS_HASH_SIZE   (1u << LZSS_HASH_BITS)
#define LZSS_HASH_MASK   (LZSS_HASH_SIZE - 1u)

/* Cap on hash-chain candidates inspected per position. Bounds worst-case
 * encode time on adversarial inputs at O(n * MAX_CHAIN * MAX_MATCH). */
#define LZSS_MAX_CHAIN   32u

#define LZSS_HEAD_EMPTY  0xFFFFFFFFu

static uint32_t hash3(const uint8_t *p) {
    return (((uint32_t)p[0] << 8) ^ ((uint32_t)p[1] << 4) ^ (uint32_t)p[2])
           & LZSS_HASH_MASK;
}

size_t lzss_encode(const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (out_cap < LZSS_HDR_BYTES) return LZSS_ERROR;

    hdr_write_len(out, in_len);

    uint32_t head[LZSS_HASH_SIZE];
    uint32_t prev[LZSS_WINDOW_SIZE];
    for (size_t i = 0; i < LZSS_HASH_SIZE; i++) head[i] = LZSS_HEAD_EMPTY;
    /* prev[] left uninitialised: entries are always written before being read
     * within the valid window; the chain walk terminates on LZSS_HEAD_EMPTY
     * or on positions below window_start. */

    uint8_t *body = out + LZSS_HDR_BYTES;
    size_t   body_cap = out_cap - LZSS_HDR_BYTES;
    size_t   body_pos = 0;

    /* Flag byte and its 8 tokens are emitted as a group. flag_ptr points at
     * the flag byte reserved at the start of the group; it is patched with
     * the accumulated flag bits when the group closes. */
    uint8_t *flag_ptr = NULL;
    uint8_t  flag_bits = 0;
    int      token_i = 0;

    size_t pos = 0;
    while (pos < in_len) {
        if (token_i == 0) {
            if (body_pos + 1 > body_cap) return LZSS_ERROR;
            flag_ptr = &body[body_pos++];
            flag_bits = 0;
        }

        uint32_t best_len = 0;
        uint32_t best_off = 0;

        if (pos + LZSS_MIN_MATCH <= in_len) {
            uint32_t max_look = (uint32_t)(in_len - pos);
            if (max_look > LZSS_MAX_MATCH) max_look = LZSS_MAX_MATCH;

            uint32_t window_start =
                (pos > LZSS_WINDOW_SIZE) ? (uint32_t)(pos - LZSS_WINDOW_SIZE) : 0u;

            uint32_t h = hash3(in + pos);
            uint32_t candidate = head[h];
            uint32_t chain = 0;
            while (candidate != LZSS_HEAD_EMPTY &&
                   candidate >= window_start &&
                   chain < LZSS_MAX_CHAIN) {
                uint32_t len = 0;
                while (len < max_look &&
                       in[candidate + len] == in[pos + len]) {
                    len++;
                }
                if (len >= LZSS_MIN_MATCH && len > best_len) {
                    best_len = len;
                    best_off = (uint32_t)pos - candidate;
                    if (len == LZSS_MAX_MATCH) break;
                }
                candidate = prev[candidate & LZSS_WINDOW_MASK];
                chain++;
            }
        }

        uint32_t advance;
        if (best_len >= LZSS_MIN_MATCH) {
            if (body_pos + 2 > body_cap) return LZSS_ERROR;
            body[body_pos++] = (uint8_t)((best_off >> 4) & 0xFFu);
            body[body_pos++] =
                (uint8_t)(((best_off & 0xFu) << 4) | (best_len - LZSS_MIN_MATCH));
            advance = best_len;
        } else {
            if (body_pos + 1 > body_cap) return LZSS_ERROR;
            body[body_pos++] = in[pos];
            flag_bits |= (uint8_t)(1u << token_i);
            advance = 1;
        }

        /* Insert every position the token covers into the hash chain, so
         * future searches can find matches that begin mid-run. */
        for (uint32_t k = 0; k < advance; k++) {
            size_t p = pos + k;
            if (p + LZSS_MIN_MATCH > in_len) break;
            uint32_t hh = hash3(in + p);
            prev[p & LZSS_WINDOW_MASK] = head[hh];
            head[hh] = (uint32_t)p;
        }

        pos += advance;
        token_i++;
        if (token_i == 8) {
            *flag_ptr = flag_bits;
            token_i = 0;
        }
    }

    if (token_i > 0) *flag_ptr = flag_bits;

    return LZSS_HDR_BYTES + body_pos;
}

size_t lzss_decode(const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (in_len < LZSS_HDR_BYTES) return LZSS_ERROR;

    size_t orig_len = hdr_read_len(in);
    if (orig_len > out_cap) return LZSS_ERROR;
    if (orig_len == 0) return 0;

    const uint8_t *body = in + LZSS_HDR_BYTES;
    size_t body_len = in_len - LZSS_HDR_BYTES;
    size_t body_pos = 0;
    size_t out_pos = 0;

    while (out_pos < orig_len) {
        if (body_pos >= body_len) return LZSS_ERROR;
        uint8_t flags = body[body_pos++];

        for (int t = 0; t < 8 && out_pos < orig_len; t++) {
            if (flags & (uint8_t)(1u << t)) {
                if (body_pos >= body_len) return LZSS_ERROR;
                out[out_pos++] = body[body_pos++];
            } else {
                if (body_pos + 2 > body_len) return LZSS_ERROR;
                uint16_t b0 = body[body_pos++];
                uint16_t b1 = body[body_pos++];
                size_t   off = (size_t)((b0 << 4) | (b1 >> 4));
                size_t   len = (size_t)((b1 & 0xFu) + LZSS_MIN_MATCH);

                if (off == 0 || off > out_pos) return LZSS_ERROR;
                if (out_pos + len > orig_len) return LZSS_ERROR;

                size_t src = out_pos - off;
                for (size_t k = 0; k < len; k++) {
                    out[out_pos] = out[src + k];  /* overlap-safe: src fixed, k advances */
                    out_pos++;
                }
            }
        }
    }
    return out_pos;
}
