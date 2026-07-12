#ifndef SHANNON_FANO_H
#define SHANNON_FANO_H

#include <stddef.h>
#include <stdint.h>

/*
 * Shannon–Fano coding (Fano's top-down split variant, 1949).
 *
 * A prefix-code entropy coder built by recursively splitting a
 * frequency-sorted symbol list into two groups of near-equal total
 * frequency. Historically the direct precursor to Huffman coding and
 * nearly optimal, but not guaranteed optimal.
 *
 * The encoded stream is a self-contained frame: it carries enough
 * metadata for decode to reconstruct the codebook without external
 * state. Layout details (header size, code-length representation, bit
 * packing) are internal to this implementation.
 *
 * Both functions return the number of bytes written to `out`, or
 * SHANNON_FANO_ERROR on failure. An empty input yields an empty output
 * (return value 0).
 *
 * Failure modes:
 *   - `out_cap` is too small to hold the result (either function)
 *   - `shannon_fano_decode` input is truncated, has an inconsistent
 *     header, or contains code lengths that do not form a valid prefix
 *     code
 */

#define SHANNON_FANO_ERROR ((size_t)-1)

size_t shannon_fano_encode(const uint8_t *in, size_t in_len,
                           uint8_t *out, size_t out_cap);

size_t shannon_fano_decode(const uint8_t *in, size_t in_len,
                           uint8_t *out, size_t out_cap);

#endif
