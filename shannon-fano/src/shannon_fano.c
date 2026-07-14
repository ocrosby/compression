#include "shannon_fano.h"

#include <string.h>

#include "bitstream.h"
#include "hdr.h"

#define MAX_SYMBOLS 256
#define MAX_LEN     32
#define HDR_BYTES   (COMPRESSION_HDR_BYTES + MAX_SYMBOLS)

/* Recursive top-down split: assign a code-length increment of 1 per level
 * and recurse into two groups of near-equal total frequency.
 * `symbols` is sorted by frequency descending; `cum` is prefix-sum of freqs. */
static void sf_split(const uint32_t *cum, const uint8_t *symbols,
                     size_t start, size_t end, uint8_t depth,
                     uint8_t *length) {
    if (end - start == 1) {
        /* Single-symbol group: length 0 is not decodable, force 1. */
        length[symbols[start]] = depth > 0 ? depth : 1;
        return;
    }
    uint32_t total = cum[end] - cum[start];
    size_t split = start + 1;
    uint32_t best_diff = UINT32_MAX;
    for (size_t i = start + 1; i < end; i++) {
        uint32_t left  = cum[i] - cum[start];
        uint32_t right = total - left;
        uint32_t diff  = left > right ? left - right : right - left;
        if (diff < best_diff) {
            best_diff = diff;
            split = i;
        } else {
            /* diff is unimodal along the sorted list — past the minimum. */
            break;
        }
    }
    sf_split(cum, symbols, start, split, depth + 1, length);
    sf_split(cum, symbols, split, end,   depth + 1, length);
}

static size_t compute_sf_lengths(const uint32_t *freq, uint8_t *length) {
    memset(length, 0, MAX_SYMBOLS);

    uint8_t  symbols[MAX_SYMBOLS];
    uint32_t sfreq[MAX_SYMBOLS];
    size_t n = 0;
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (freq[s]) {
            symbols[n] = (uint8_t)s;
            sfreq[n]   = freq[s];
            n++;
        }
    }
    if (n == 0) return 0;
    if (n == 1) { length[symbols[0]] = 1; return 0; }

    /* Insertion sort by frequency descending; n <= 256. */
    for (size_t i = 1; i < n; i++) {
        uint8_t  sv = symbols[i];
        uint32_t fv = sfreq[i];
        size_t j = i;
        while (j > 0 && sfreq[j - 1] < fv) {
            symbols[j] = symbols[j - 1];
            sfreq[j]   = sfreq[j - 1];
            j--;
        }
        symbols[j] = sv;
        sfreq[j]   = fv;
    }

    uint32_t cum[MAX_SYMBOLS + 1];
    cum[0] = 0;
    for (size_t i = 0; i < n; i++) cum[i + 1] = cum[i] + sfreq[i];

    sf_split(cum, symbols, 0, n, 0, length);

    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s] > MAX_LEN) return SHANNON_FANO_ERROR;
    }
    return 0;
}

static void canonical_codes(const uint8_t *length, uint32_t *code) {
    uint16_t count[MAX_LEN + 1] = {0};
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s]) count[length[s]]++;
    }
    uint32_t next_code[MAX_LEN + 1] = {0};
    uint32_t c = 0;
    for (int l = 1; l <= MAX_LEN; l++) {
        next_code[l] = c;
        c = (c + count[l]) << 1;
    }
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        code[s] = length[s] ? next_code[length[s]]++ : 0;
    }
}

size_t shannon_fano_encode(const uint8_t *in, size_t in_len,
                           uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;

    uint32_t freq[MAX_SYMBOLS] = {0};
    for (size_t i = 0; i < in_len; i++) freq[in[i]]++;

    uint8_t length[MAX_SYMBOLS];
    if (compute_sf_lengths(freq, length) == SHANNON_FANO_ERROR) {
        return SHANNON_FANO_ERROR;
    }

    if (out_cap < HDR_BYTES) return SHANNON_FANO_ERROR;

    uint32_t code[MAX_SYMBOLS];
    canonical_codes(length, code);

    hdr_write_len(out, in_len);
    memcpy(out + COMPRESSION_HDR_BYTES, length, MAX_SYMBOLS);

    bit_writer_t bw;
    bit_writer_init(&bw, out + HDR_BYTES, out_cap - HDR_BYTES);
    for (size_t i = 0; i < in_len; i++) {
        if (bit_writer_put(&bw, code[in[i]], length[in[i]]) != 0) {
            return SHANNON_FANO_ERROR;
        }
    }
    return HDR_BYTES + bit_writer_bytes(&bw);
}

size_t shannon_fano_decode(const uint8_t *in, size_t in_len,
                           uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (in_len < HDR_BYTES) return SHANNON_FANO_ERROR;

    size_t orig_len = hdr_read_len(in);
    if (orig_len > out_cap) return SHANNON_FANO_ERROR;
    if (orig_len == 0) return 0;

    uint8_t length[MAX_SYMBOLS];
    memcpy(length, in + COMPRESSION_HDR_BYTES, MAX_SYMBOLS);

    uint8_t max_len = 0;
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s] > max_len) max_len = length[s];
    }
    if (max_len == 0 || max_len > MAX_LEN) return SHANNON_FANO_ERROR;

    uint16_t count[MAX_LEN + 1] = {0};
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s]) count[length[s]]++;
    }
    uint32_t first_code[MAX_LEN + 1] = {0};
    uint32_t c = 0;
    for (int l = 1; l <= max_len; l++) {
        first_code[l] = c;
        c = (c + count[l]) << 1;
    }
    uint16_t first_sym[MAX_LEN + 1] = {0};
    uint16_t idx = 0;
    for (int l = 1; l <= max_len; l++) {
        first_sym[l] = idx;
        idx += count[l];
    }
    uint8_t sorted[MAX_SYMBOLS];
    uint16_t pos[MAX_LEN + 1];
    memcpy(pos, first_sym, sizeof pos);
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s]) sorted[pos[length[s]]++] = (uint8_t)s;
    }

    bit_reader_t br;
    bit_reader_init(&br, in + HDR_BYTES, in_len - HDR_BYTES);
    size_t out_pos = 0;

    while (out_pos < orig_len) {
        uint32_t acc = 0;
        int matched = 0;
        for (int len = 1; len <= max_len; len++) {
            uint32_t bit;
            if (bit_reader_get(&br, 1, &bit) != 0) return SHANNON_FANO_ERROR;
            acc = (acc << 1) | bit;
            if (count[len] > 0 &&
                acc >= first_code[len] &&
                acc < first_code[len] + count[len]) {
                out[out_pos++] = sorted[first_sym[len] + (acc - first_code[len])];
                matched = 1;
                break;
            }
        }
        if (!matched) return SHANNON_FANO_ERROR;
    }
    return out_pos;
}
