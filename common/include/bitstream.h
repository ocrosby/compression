#ifndef COMPRESSION_COMMON_BITSTREAM_H
#define COMPRESSION_COMMON_BITSTREAM_H

#include <stddef.h>
#include <stdint.h>

/*
 * MSB-first bit stream helpers shared by huffman and shannon-fano.
 * Future fixed-width coders (LZW, arithmetic) that pack sub-byte codes
 * should adopt these helpers as they land.
 *
 * Both writer and reader operate over a caller-owned byte buffer. The
 * writer zeroes each byte lazily the first time it is entered, so the
 * caller does not have to pre-clear the buffer.
 *
 * Values are packed with the most-significant bit first: writing
 * (value = 0b101, nbits = 3) at bit position 0 produces the byte
 * 0b10100000.
 *
 * `nbits` may be in [0, 32]. A zero-width call is a no-op.
 */

typedef struct {
    uint8_t *buf;
    size_t   cap;      /* buffer capacity in bytes */
    size_t   bit_pos;  /* next bit position to write */
} bit_writer_t;

void bit_writer_init(bit_writer_t *w, uint8_t *buf, size_t cap);

/* Write the low `nbits` bits of `value` MSB-first at the current
 * position. Returns 0 on success, -1 if the write would exceed `cap`. */
int bit_writer_put(bit_writer_t *w, uint32_t value, unsigned nbits);

/* Number of bytes used so far — ceil(bit_pos / 8). */
size_t bit_writer_bytes(const bit_writer_t *w);


typedef struct {
    const uint8_t *buf;
    size_t         bit_cap;  /* total bits available */
    size_t         bit_pos;
} bit_reader_t;

void bit_reader_init(bit_reader_t *r, const uint8_t *buf, size_t byte_len);

/* Read `nbits` MSB-first at the current position into *out.
 * Returns 0 on success, -1 if fewer than `nbits` bits remain. */
int bit_reader_get(bit_reader_t *r, unsigned nbits, uint32_t *out);

#endif
