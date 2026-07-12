#include <stdint.h>
#include <string.h>

#include "ctestprobe.h"
#include "rle.h"

static void test_encode_empty(void) {
    uint8_t out[16];
    size_t n = rle_encode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_encode_single_byte(void) {
    uint8_t in[] = {'A'};
    uint8_t out[16];
    uint8_t want[] = {1, 'A'};
    size_t n = rle_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_BYTES(out, n, want, sizeof want);
}

static void test_encode_run_of_five(void) {
    uint8_t in[] = {'A', 'A', 'A', 'A', 'A'};
    uint8_t out[16];
    uint8_t want[] = {5, 'A'};
    size_t n = rle_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_BYTES(out, n, want, sizeof want);
}

static void test_encode_mixed_runs(void) {
    uint8_t in[] = {'A', 'A', 'B', 'C', 'C', 'C'};
    uint8_t out[16];
    uint8_t want[] = {2, 'A', 1, 'B', 3, 'C'};
    size_t n = rle_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_BYTES(out, n, want, sizeof want);
}

static void test_encode_splits_runs_over_255(void) {
    uint8_t in[300];
    memset(in, 'Z', sizeof in);
    uint8_t out[16];
    uint8_t want[] = {255, 'Z', 45, 'Z'};
    size_t n = rle_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_BYTES(out, n, want, sizeof want);
}

static void test_encode_output_buffer_too_small(void) {
    uint8_t in[] = {'A'};
    uint8_t out[1]; /* need 2 */
    size_t n = rle_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, RLE_ERROR);
}

static void test_decode_empty(void) {
    uint8_t out[16];
    size_t n = rle_decode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_decode_single_pair(void) {
    uint8_t in[] = {3, 'X'};
    uint8_t out[16];
    uint8_t want[] = {'X', 'X', 'X'};
    size_t n = rle_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_BYTES(out, n, want, sizeof want);
}

static void test_decode_output_buffer_too_small(void) {
    uint8_t in[] = {3, 'A'};
    uint8_t out[2]; /* need 3 */
    size_t n = rle_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, RLE_ERROR);
}

static void test_decode_rejects_odd_length_input(void) {
    uint8_t in[] = {3};
    uint8_t out[16];
    size_t n = rle_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, RLE_ERROR);
}

static void test_decode_rejects_zero_count(void) {
    uint8_t in[] = {0, 'A'};
    uint8_t out[16];
    size_t n = rle_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, RLE_ERROR);
}

static void test_round_trip(void) {
    const char *msg = "AAABBBBCCDAAA";
    size_t msg_len = strlen(msg);
    uint8_t enc[64];
    uint8_t dec[64];

    size_t n_enc = rle_encode((const uint8_t *)msg, msg_len, enc, sizeof enc);
    CTP_ASSERT(n_enc != RLE_ERROR);

    size_t n_dec = rle_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_BYTES(dec, n_dec, msg, msg_len);
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("encode_empty",                    test_encode_empty);
    ctestprobe_register("encode_single_byte",              test_encode_single_byte);
    ctestprobe_register("encode_run_of_five",              test_encode_run_of_five);
    ctestprobe_register("encode_mixed_runs",               test_encode_mixed_runs);
    ctestprobe_register("encode_splits_runs_over_255",     test_encode_splits_runs_over_255);
    ctestprobe_register("encode_output_buffer_too_small",  test_encode_output_buffer_too_small);
    ctestprobe_register("decode_empty",                    test_decode_empty);
    ctestprobe_register("decode_single_pair",              test_decode_single_pair);
    ctestprobe_register("decode_output_buffer_too_small",  test_decode_output_buffer_too_small);
    ctestprobe_register("decode_rejects_odd_length_input", test_decode_rejects_odd_length_input);
    ctestprobe_register("decode_rejects_zero_count",       test_decode_rejects_zero_count);
    ctestprobe_register("round_trip",                      test_round_trip);
    return ctestprobe_main(argc, argv);
}
