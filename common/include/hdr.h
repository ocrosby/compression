#ifndef COMPRESSION_COMMON_HDR_H
#define COMPRESSION_COMMON_HDR_H

#include <stddef.h>
#include <stdint.h>

/*
 * 8-byte little-endian original-length header shared by every coder in
 * this repo whose wire format needs to bound decoded output (huffman,
 * shannon-fano). RLE has no header. Future coders that also emit an
 * uint64 length prefix (LZW, LZSS, arithmetic, ...) should adopt this
 * helper as they land.
 */

#define COMPRESSION_HDR_BYTES ((size_t)8)

/* Serialize `len` as a little-endian uint64 into hdr[0..7]. */
void hdr_write_len(uint8_t *hdr, size_t len);

/* Deserialize a little-endian uint64 from hdr[0..7]. */
size_t hdr_read_len(const uint8_t *hdr);

#endif
