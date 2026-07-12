#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>
#include <stdint.h>

/*
 * Canonical Huffman coding.
 *
 * The encoded stream is a self-contained frame: it carries enough
 * metadata for decode to reconstruct the codebook without external
 * state. Layout details (header size, code-length representation, bit
 * packing) are internal to this implementation.
 *
 * Both functions return the number of bytes written to `out`, or
 * HUFFMAN_ERROR on failure. An empty input yields an empty output
 * (return value 0).
 *
 * Failure modes:
 *   - `out_cap` is too small to hold the result (either function)
 *   - `huffman_decode` input is truncated, has an inconsistent header,
 *     or contains code lengths that do not form a valid prefix code
 */

#define HUFFMAN_ERROR ((size_t)-1)

size_t huffman_encode(const uint8_t *in, size_t in_len,
                      uint8_t *out, size_t out_cap);

size_t huffman_decode(const uint8_t *in, size_t in_len,
                      uint8_t *out, size_t out_cap);

#endif
