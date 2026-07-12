#ifndef RLE_H
#define RLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Byte-level Run-Length Encoding.
 *
 * Format: a stream of (count, byte) pairs where `count` is a uint8_t in
 * [1, 255]. Runs longer than 255 are split into multiple pairs.
 *
 * Both functions return the number of bytes written to `out`, or RLE_ERROR
 * on failure. An empty input yields an empty output (return value 0).
 *
 * Failure modes:
 *   - `out_cap` is too small to hold the result (either function)
 *   - `rle_decode` input has odd length (dangling count with no byte)
 *   - `rle_decode` input contains a count byte of zero
 */

#define RLE_ERROR ((size_t)-1)

size_t rle_encode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap);

size_t rle_decode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap);

#endif
