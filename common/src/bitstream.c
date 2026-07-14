#include "bitstream.h"

void bit_writer_init(bit_writer_t *w, uint8_t *buf, size_t cap) {
    w->buf = buf;
    w->cap = cap;
    w->bit_pos = 0;
}

int bit_writer_put(bit_writer_t *w, uint32_t value, unsigned nbits) {
    if (nbits == 0) return 0;
    size_t end_bits = w->bit_pos + nbits;
    if ((end_bits + 7u) / 8u > w->cap) return -1;
    for (unsigned b = nbits; b-- > 0; ) {
        size_t bp = w->bit_pos++;
        if ((bp & 7u) == 0u) w->buf[bp >> 3] = 0;
        if ((value >> b) & 1u) {
            w->buf[bp >> 3] |= (uint8_t)(1u << (7u - (bp & 7u)));
        }
    }
    return 0;
}

size_t bit_writer_bytes(const bit_writer_t *w) {
    return (w->bit_pos + 7u) / 8u;
}

void bit_reader_init(bit_reader_t *r, const uint8_t *buf, size_t byte_len) {
    r->buf = buf;
    r->bit_cap = byte_len * 8u;
    r->bit_pos = 0;
}

int bit_reader_get(bit_reader_t *r, unsigned nbits, uint32_t *out) {
    if (nbits == 0) { *out = 0; return 0; }
    if (r->bit_pos + nbits > r->bit_cap) return -1;
    uint32_t acc = 0;
    for (unsigned b = 0; b < nbits; b++) {
        size_t bp = r->bit_pos++;
        uint32_t bit = (uint32_t)((r->buf[bp >> 3] >> (7u - (bp & 7u))) & 1u);
        acc = (acc << 1) | bit;
    }
    *out = acc;
    return 0;
}
