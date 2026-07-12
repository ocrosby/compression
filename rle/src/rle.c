#include "rle.h"

size_t rle_encode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    size_t i = 0;
    size_t j = 0;
    while (i < in_len) {
        uint8_t byte = in[i];
        size_t run = 1;
        while (i + run < in_len && in[i + run] == byte && run < 255) {
            run++;
        }
        if (j + 2 > out_cap) {
            return RLE_ERROR;
        }
        out[j++] = (uint8_t)run;
        out[j++] = byte;
        i += run;
    }
    return j;
}

size_t rle_decode(const uint8_t *in, size_t in_len,
                  uint8_t *out, size_t out_cap) {
    if (in_len % 2 != 0) {
        return RLE_ERROR;
    }
    size_t j = 0;
    for (size_t i = 0; i < in_len; i += 2) {
        uint8_t count = in[i];
        uint8_t byte = in[i + 1];
        if (count == 0) {
            return RLE_ERROR;
        }
        if (j + count > out_cap) {
            return RLE_ERROR;
        }
        for (uint8_t k = 0; k < count; k++) {
            out[j++] = byte;
        }
    }
    return j;
}
