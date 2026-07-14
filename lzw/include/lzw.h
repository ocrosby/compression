#ifndef LZW_H
#define LZW_H

#include <stddef.h>
#include <stdint.h>

/*
 * Basic Lempel-Ziv-Welch (LZW) coding.
 *
 * Fixed 12-bit code width; dictionary initialised to the 256 single-byte
 * strings and grown up to 4096 entries. When the dictionary fills the
 * encoder keeps emitting existing codes without adding new ones (no CLEAR
 * code). The encoded stream is a self-contained frame: an 8-byte
 * little-endian original-length prefix followed by MSB-first packed
 * 12-bit codes. Any trailing padding bits are ignored on decode because
 * the original length bounds the output.
 *
 * Both functions return the number of bytes written to `out`, or
 * LZW_ERROR on failure. An empty input yields an empty output (return
 * value 0, no header emitted).
 *
 * Failure modes:
 *   - `out_cap` is too small to hold the result (either function)
 *   - `lzw_decode` input is shorter than the header
 *   - `lzw_decode` input contains an out-of-range code, or a code that
 *     violates the LZW invariant (K > next_code, or K == next_code as
 *     the very first code)
 *   - `lzw_decode` input decodes to more bytes than the header claims
 */

#define LZW_ERROR ((size_t)-1)

size_t lzw_encode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap);

size_t lzw_decode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap);

#endif
