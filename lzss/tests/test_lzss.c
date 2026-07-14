#include <stdint.h>
#include <string.h>

#include "ctestprobe.h"
#include "lzss.h"

/*
 * LZSS has legitimate implementation freedom in match-search policy
 * (chain depth, lazy matching, tie-breaking) so we test round-trip
 * behaviour rather than pinning exact wire bytes.
 */
static void assert_round_trip(const uint8_t *in, size_t in_len) {
    uint8_t enc[16384];
    uint8_t dec[16384];

    size_t n_enc = lzss_encode(in, in_len, enc, sizeof enc);
    CTP_ASSERT(n_enc != LZSS_ERROR);

    size_t n_dec = lzss_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_BYTES(dec, n_dec, in, in_len);
}

static void test_encode_empty(void) {
    uint8_t out[16];
    size_t n = lzss_encode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_decode_empty(void) {
    uint8_t out[16];
    size_t n = lzss_decode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_round_trip_single_byte(void) {
    uint8_t in[] = {'A'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_two_bytes(void) {
    uint8_t in[] = {'A', 'B'};
    assert_round_trip(in, sizeof in);
}

/* Exactly MIN_MATCH bytes of a run — smallest useful match. */
static void test_round_trip_min_match(void) {
    uint8_t in[] = {'A', 'A', 'A', 'A'};
    assert_round_trip(in, sizeof in);
}

/* Long run — exercises overlapping-match copy (offset < length). */
static void test_round_trip_long_run(void) {
    uint8_t in[128];
    memset(in, 'Z', sizeof in);
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_two_symbols(void) {
    uint8_t in[] = {'A', 'B', 'A', 'B', 'A', 'A', 'B', 'B', 'A', 'B', 'A', 'B'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_english_text(void) {
    const char *msg =
        "the quick brown fox jumps over the lazy dog "
        "the quick brown fox jumps over the lazy dog";
    assert_round_trip((const uint8_t *)msg, strlen(msg));
}

/* Non-adjacent repetition — the match reaches back over intervening
 * literals, exercising a nontrivial 12-bit offset. */
static void test_round_trip_far_repetition(void) {
    uint8_t in[600];
    memcpy(in, "quick brown fox", 15);
    for (size_t i = 15; i < 550; i++) in[i] = (uint8_t)('a' + (i % 7));
    memcpy(in + 550, "quick brown fox", 15);
    memset(in + 565, 'x', 35);
    assert_round_trip(in, sizeof in);
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

/* Long repetitive input — exercises the window sliding past positions
 * and the hash-chain walks staying inside the window. */
static void test_round_trip_windowed(void) {
    uint8_t in[8000];
    for (size_t i = 0; i < sizeof in; i++) {
        in[i] = (uint8_t)('a' + (i % 16));
    }
    assert_round_trip(in, sizeof in);
}

static void test_encode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
    uint8_t out[4];  /* smaller than the 8-byte header */
    size_t n = lzss_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZSS_ERROR);
}

static void test_decode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B', 'C'};
    uint8_t enc[128];
    uint8_t dec[2];  /* need 9 */

    size_t n_enc = lzss_encode(in, sizeof in, enc, sizeof enc);
    CTP_ASSERT(n_enc != LZSS_ERROR);

    size_t n_dec = lzss_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_INT(n_dec, LZSS_ERROR);
}

static void test_decode_rejects_truncated_header(void) {
    uint8_t in[] = {0x01, 0x02, 0x03};
    uint8_t out[16];
    size_t n = lzss_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZSS_ERROR);
}

/* A match token as the very first token has no history to reference; a
 * zero offset is also invalid. */
static void test_decode_rejects_zero_offset(void) {
    uint8_t in[] = {
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* orig_len = 5 */
        0x00,        /* flag byte: all tokens are matches */
        0x00, 0x00,  /* match: off=0, len=3 → invalid */
    };
    uint8_t out[16];
    size_t n = lzss_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, LZSS_ERROR);
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("encode_empty",                    test_encode_empty);
    ctestprobe_register("decode_empty",                    test_decode_empty);
    ctestprobe_register("round_trip_single_byte",          test_round_trip_single_byte);
    ctestprobe_register("round_trip_two_bytes",            test_round_trip_two_bytes);
    ctestprobe_register("round_trip_min_match",            test_round_trip_min_match);
    ctestprobe_register("round_trip_long_run",             test_round_trip_long_run);
    ctestprobe_register("round_trip_two_symbols",          test_round_trip_two_symbols);
    ctestprobe_register("round_trip_english_text",         test_round_trip_english_text);
    ctestprobe_register("round_trip_far_repetition",       test_round_trip_far_repetition);
    ctestprobe_register("round_trip_binary_data",          test_round_trip_binary_data);
    ctestprobe_register("round_trip_all_256_symbols",      test_round_trip_all_256_symbols);
    ctestprobe_register("round_trip_windowed",             test_round_trip_windowed);
    ctestprobe_register("encode_output_buffer_too_small",  test_encode_output_buffer_too_small);
    ctestprobe_register("decode_output_buffer_too_small",  test_decode_output_buffer_too_small);
    ctestprobe_register("decode_rejects_truncated_header", test_decode_rejects_truncated_header);
    ctestprobe_register("decode_rejects_zero_offset",      test_decode_rejects_zero_offset);
    return ctestprobe_main(argc, argv);
}
