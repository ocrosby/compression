#include <stdint.h>
#include <string.h>

#include "ctestprobe.h"
#include "lzw.h"

/*
 * LZW has legitimate implementation freedom in code width, dictionary
 * reset policy, and bit-packing order, so we test round-trip behavior
 * rather than pinning exact wire bytes.
 */
static void assert_round_trip(const uint8_t *in, size_t in_len) {
    uint8_t enc[8192];
    uint8_t dec[8192];

    size_t n_enc = lzw_encode(in, in_len, enc, sizeof enc);
    CTP_ASSERT(n_enc != LZW_ERROR);

    size_t n_dec = lzw_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_BYTES(dec, n_dec, in, in_len);
}

static void test_encode_empty(void) {
    uint8_t out[16];
    size_t n = lzw_encode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_decode_empty(void) {
    uint8_t out[16];
    size_t n = lzw_decode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_round_trip_single_byte(void) {
    uint8_t in[] = {'A'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_two_identical_bytes(void) {
    uint8_t in[] = {'X', 'X'};
    assert_round_trip(in, sizeof in);
}

/* Classic KwKwK trigger: the encoder emits a code the instant it is
 * added, forcing the decoder into its special-case branch. */
static void test_round_trip_kwkwk(void) {
    uint8_t in[] = {'X', 'X', 'X'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_repeated_run(void) {
    uint8_t in[64];
    memset(in, 'Z', sizeof in);
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_two_symbols(void) {
    uint8_t in[] = {'A', 'B', 'A', 'B', 'A', 'A', 'B', 'B', 'A'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_english_text(void) {
    const char *msg =
        "the quick brown fox jumps over the lazy dog "
        "the quick brown fox jumps over the lazy dog";
    assert_round_trip((const uint8_t *)msg, strlen(msg));
}

static void test_round_trip_binary_data(void) {
    uint8_t in[512];
    for (size_t i = 0; i < sizeof in; i++) {
        in[i] = (uint8_t)((i * 31 + 7) & 0xFF);
    }
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_all_256_symbols(void) {
    uint8_t in[256];
    for (size_t i = 0; i < sizeof in; i++) in[i] = (uint8_t)i;
    assert_round_trip(in, sizeof in);
}

/* Exercises the dictionary-full path: 8KB of a small alphabet grows the
 * dictionary well past 4096 entries' worth of distinct phrases. */
static void test_round_trip_dictionary_full(void) {
    uint8_t in[8000];
    for (size_t i = 0; i < sizeof in; i++) {
        in[i] = (uint8_t)('a' + (i % 16));
    }
    assert_round_trip(in, sizeof in);
}

static void test_encode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
    uint8_t out[4];  /* smaller than the 8-byte header */
    size_t n = lzw_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZW_ERROR);
}

static void test_decode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B', 'C'};
    uint8_t enc[128];
    uint8_t dec[2];  /* need 9 */

    size_t n_enc = lzw_encode(in, sizeof in, enc, sizeof enc);
    CTP_ASSERT(n_enc != LZW_ERROR);

    size_t n_dec = lzw_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_INT(n_dec, LZW_ERROR);
}

static void test_decode_rejects_truncated_header(void) {
    uint8_t in[] = {0x01, 0x02, 0x03};
    uint8_t out[16];
    size_t n = lzw_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZW_ERROR);
}

/* A first code that is not a literal violates the LZW invariant. */
static void test_decode_rejects_bad_first_code(void) {
    /* orig_len = 4, then a 12-bit code of 300 (>= 256). */
    uint8_t in[] = {
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x12, 0xC0,  /* 0x12C = 300, packed as first 12 bits */
    };
    uint8_t out[16];
    size_t n = lzw_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZW_ERROR);
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("encode_empty",                   test_encode_empty);
    ctestprobe_register("decode_empty",                   test_decode_empty);
    ctestprobe_register("round_trip_single_byte",         test_round_trip_single_byte);
    ctestprobe_register("round_trip_two_identical_bytes", test_round_trip_two_identical_bytes);
    ctestprobe_register("round_trip_kwkwk",               test_round_trip_kwkwk);
    ctestprobe_register("round_trip_repeated_run",        test_round_trip_repeated_run);
    ctestprobe_register("round_trip_two_symbols",         test_round_trip_two_symbols);
    ctestprobe_register("round_trip_english_text",        test_round_trip_english_text);
    ctestprobe_register("round_trip_binary_data",         test_round_trip_binary_data);
    ctestprobe_register("round_trip_all_256_symbols",     test_round_trip_all_256_symbols);
    ctestprobe_register("round_trip_dictionary_full",     test_round_trip_dictionary_full);
    ctestprobe_register("encode_output_buffer_too_small", test_encode_output_buffer_too_small);
    ctestprobe_register("decode_output_buffer_too_small", test_decode_output_buffer_too_small);
    ctestprobe_register("decode_rejects_truncated_header", test_decode_rejects_truncated_header);
    ctestprobe_register("decode_rejects_bad_first_code",  test_decode_rejects_bad_first_code);
    return ctestprobe_main(argc, argv);
}
