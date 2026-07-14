#ifndef LZSS_H
#define LZSS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Basic Lempel-Ziv-Storer-Szymanski (LZSS) coding.
 *
 * 4096-byte sliding window; match lengths in [3, 18]; matches are
 * encoded as a (12-bit offset, 4-bit length-3) pair. Each token is
 * either a literal byte or a match reference, distinguished by one
 * flag bit per token; tokens are grouped in eights preceded by a flag
 * byte, LSB-first.
 *
 * Frame layout:
 *   [0..7]  original length, little-endian uint64
 *   [8..]   sequence of groups: <flag byte><up to 8 tokens>
 *           token where flag bit is 1: 1 byte, literal
 *           token where flag bit is 0: 2 bytes,
 *               b0 = offset[11:4]
 *               b1 = (offset[3:0] << 4) | (length - 3)
 *
 * The trailing group's flag byte may cover fewer than 8 tokens; the
 * original length in the header tells the decoder when to stop, so any
 * unused flag bits are ignored.
 *
 * Both functions return the number of bytes written to `out`, or
 * LZSS_ERROR on failure. An empty input yields an empty output (return
 * value 0, no header emitted).
 *
 * Failure modes:
 *   - `out_cap` is too small to hold the result (either function)
 *   - `lzss_decode` input is shorter than the header
 *   - `lzss_decode` input is truncated mid-token
 *   - `lzss_decode` match references a zero offset or an offset that
 *     reaches before the start of the output
 *   - `lzss_decode` match would write past the header's original length
 */

#define LZSS_ERROR ((size_t)-1)

size_t lzss_encode(const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t out_cap);

size_t lzss_decode(const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t out_cap);

#endif
