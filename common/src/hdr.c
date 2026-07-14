#include "hdr.h"

void hdr_write_len(uint8_t *hdr, size_t len) {
    for (size_t i = 0; i < 8; i++) {
        hdr[i] = (uint8_t)((len >> (i * 8)) & 0xFFu);
    }
}

size_t hdr_read_len(const uint8_t *hdr) {
    size_t len = 0;
    for (size_t i = 0; i < 8; i++) {
        len |= (size_t)hdr[i] << (i * 8);
    }
    return len;
}
