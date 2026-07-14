#include "huffman.h"

#include <string.h>

#include "bitstream.h"
#include "hdr.h"

#define MAX_SYMBOLS 256
#define MAX_LEN     32
#define HDR_BYTES   (COMPRESSION_HDR_BYTES + MAX_SYMBOLS)

typedef struct {
    uint32_t freq;
    int16_t  parent;
} node_t;

static void heap_up(uint16_t *heap, size_t i, const node_t *nodes) {
    while (i > 0) {
        size_t p = (i - 1) / 2;
        if (nodes[heap[p]].freq <= nodes[heap[i]].freq) break;
        uint16_t t = heap[p]; heap[p] = heap[i]; heap[i] = t;
        i = p;
    }
}

static void heap_down(uint16_t *heap, size_t n, const node_t *nodes) {
    size_t i = 0;
    for (;;) {
        size_t l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < n && nodes[heap[l]].freq < nodes[heap[m]].freq) m = l;
        if (r < n && nodes[heap[r]].freq < nodes[heap[m]].freq) m = r;
        if (m == i) break;
        uint16_t t = heap[m]; heap[m] = heap[i]; heap[i] = t;
        i = m;
    }
}

/* Build Huffman tree from `freq[]` and derive per-symbol code lengths.
 * Returns 0 on success, HUFFMAN_ERROR if any code length exceeds MAX_LEN. */
static size_t compute_huffman_lengths(const uint32_t *freq, uint8_t *length) {
    node_t nodes[MAX_SYMBOLS * 2 - 1];
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        nodes[s].freq = freq[s];
        nodes[s].parent = -1;
    }

    uint16_t heap[MAX_SYMBOLS];
    size_t heap_n = 0;
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (freq[s]) {
            heap[heap_n++] = (uint16_t)s;
            heap_up(heap, heap_n - 1, nodes);
        }
    }

    int next = MAX_SYMBOLS;
    while (heap_n >= 2) {
        uint16_t a = heap[0]; heap[0] = heap[--heap_n]; heap_down(heap, heap_n, nodes);
        uint16_t b = heap[0]; heap[0] = heap[--heap_n]; heap_down(heap, heap_n, nodes);
        nodes[next].freq = nodes[a].freq + nodes[b].freq;
        nodes[next].parent = -1;
        nodes[a].parent = (int16_t)next;
        nodes[b].parent = (int16_t)next;
        heap[heap_n++] = (uint16_t)next;
        heap_up(heap, heap_n - 1, nodes);
        next++;
    }

    memset(length, 0, MAX_SYMBOLS);
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (freq[s] == 0) continue;
        int depth = 0;
        int p = s;
        while (nodes[p].parent != -1) { p = nodes[p].parent; depth++; }
        /* Single-symbol input yields depth 0; length 0 is not decodable, so force 1. */
        if (depth == 0) depth = 1;
        if (depth > MAX_LEN) return HUFFMAN_ERROR;
        length[s] = (uint8_t)depth;
    }
    return 0;
}

/* Assign canonical codes from `length[]` into `code[]` (RFC 1951 §3.2.2). */
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

size_t huffman_encode(const uint8_t *in, size_t in_len,
                      uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;

    uint32_t freq[MAX_SYMBOLS] = {0};
    for (size_t i = 0; i < in_len; i++) freq[in[i]]++;

    uint8_t length[MAX_SYMBOLS];
    if (compute_huffman_lengths(freq, length) == HUFFMAN_ERROR) return HUFFMAN_ERROR;

    if (out_cap < HDR_BYTES) return HUFFMAN_ERROR;

    uint32_t code[MAX_SYMBOLS];
    canonical_codes(length, code);

    hdr_write_len(out, in_len);
    memcpy(out + COMPRESSION_HDR_BYTES, length, MAX_SYMBOLS);

    bit_writer_t bw;
    bit_writer_init(&bw, out + HDR_BYTES, out_cap - HDR_BYTES);
    for (size_t i = 0; i < in_len; i++) {
        if (bit_writer_put(&bw, code[in[i]], length[in[i]]) != 0) {
            return HUFFMAN_ERROR;
        }
    }
    return HDR_BYTES + bit_writer_bytes(&bw);
}

size_t huffman_decode(const uint8_t *in, size_t in_len,
                      uint8_t *out, size_t out_cap) {
    if (in_len == 0) return 0;
    if (in_len < HDR_BYTES) return HUFFMAN_ERROR;

    size_t orig_len = hdr_read_len(in);
    if (orig_len > out_cap) return HUFFMAN_ERROR;
    if (orig_len == 0) return 0;

    uint8_t length[MAX_SYMBOLS];
    memcpy(length, in + COMPRESSION_HDR_BYTES, MAX_SYMBOLS);

    uint8_t max_len = 0;
    for (int s = 0; s < MAX_SYMBOLS; s++) {
        if (length[s] > max_len) max_len = length[s];
    }
    if (max_len == 0 || max_len > MAX_LEN) return HUFFMAN_ERROR;

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
            if (bit_reader_get(&br, 1, &bit) != 0) return HUFFMAN_ERROR;
            acc = (acc << 1) | bit;
            if (count[len] > 0 &&
                acc >= first_code[len] &&
                acc < first_code[len] + count[len]) {
                out[out_pos++] = sorted[first_sym[len] + (acc - first_code[len])];
                matched = 1;
                break;
            }
        }
        if (!matched) return HUFFMAN_ERROR;
    }
    return out_pos;
}
