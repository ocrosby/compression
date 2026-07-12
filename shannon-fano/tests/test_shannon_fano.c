#include <stdint.h>
#include <string.h>

#include "ctestprobe.h"
#include "shannon_fano.h"

/*
 * Shannon-Fano has legitimate implementation freedom in tie-breaking
 * during the split and in bit-packing order, so we test round-trip
 * behavior rather than pinning exact wire bytes.
 */
static void assert_round_trip(const uint8_t *in, size_t in_len) {
    uint8_t enc[4096];
    uint8_t dec[4096];

    size_t n_enc = shannon_fano_encode(in, in_len, enc, sizeof enc);
    CTP_ASSERT(n_enc != SHANNON_FANO_ERROR);

    size_t n_dec = shannon_fano_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_BYTES(dec, n_dec, in, in_len);
}

static void test_encode_empty(void) {
    uint8_t out[16];
    size_t n = shannon_fano_encode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_decode_empty(void) {
    uint8_t out[16];
    size_t n = shannon_fano_decode(NULL, 0, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, 0);
}

static void test_round_trip_single_byte(void) {
    uint8_t in[] = {'A'};
    assert_round_trip(in, sizeof in);
}

static void test_round_trip_single_distinct_symbol(void) {
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
    for (size_t i = 0; i < sizeof in; i++) {
        in[i] = (uint8_t)i;
    }
    assert_round_trip(in, sizeof in);
}

/*
 * Classic 4-symbol distribution where Shannon-Fano is provably
 * sub-optimal vs. Huffman. The round trip must still be exact; this
 * test exists to exercise the pathological split case.
 */
static void test_round_trip_suboptimal_case(void) {
    uint8_t in[34];
    size_t p = 0;
    for (int i = 0; i < 15; i++) in[p++] = 'A';
    for (int i = 0; i < 7; i++)  in[p++] = 'B';
    for (int i = 0; i < 6; i++)  in[p++] = 'C';
    for (int i = 0; i < 6; i++)  in[p++] = 'D';
    assert_round_trip(in, p);
}

static void test_encode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C'};
    uint8_t out[1];
    size_t n = shannon_fano_encode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, SHANNON_FANO_ERROR);
}

static void test_decode_output_buffer_too_small(void) {
    uint8_t in[] = {'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B', 'C'};
    uint8_t enc[512];
    uint8_t dec[2]; /* need 9 */

    size_t n_enc = shannon_fano_encode(in, sizeof in, enc, sizeof enc);
    CTP_ASSERT(n_enc != SHANNON_FANO_ERROR);

    size_t n_dec = shannon_fano_decode(enc, n_enc, dec, sizeof dec);
    CTP_ASSERT_EQ_INT(n_dec, SHANNON_FANO_ERROR);
}

static void test_decode_rejects_truncated_input(void) {
    uint8_t in[] = {0x01, 0x02};
    uint8_t out[16];
    size_t n = shannon_fano_decode(in, sizeof in, out, sizeof out);
    CTP_ASSERT_EQ_INT(n, SHANNON_FANO_ERROR);
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("encode_empty",                    test_encode_empty);
    ctestprobe_register("decode_empty",                    test_decode_empty);
    ctestprobe_register("round_trip_single_byte",          test_round_trip_single_byte);
    ctestprobe_register("round_trip_single_distinct",      test_round_trip_single_distinct_symbol);
    ctestprobe_register("round_trip_two_symbols",          test_round_trip_two_symbols);
    ctestprobe_register("round_trip_english_text",         test_round_trip_english_text);
    ctestprobe_register("round_trip_binary_data",          test_round_trip_binary_data);
    ctestprobe_register("round_trip_all_256_symbols",      test_round_trip_all_256_symbols);
    ctestprobe_register("round_trip_suboptimal_case",      test_round_trip_suboptimal_case);
    ctestprobe_register("encode_output_buffer_too_small",  test_encode_output_buffer_too_small);
    ctestprobe_register("decode_output_buffer_too_small",  test_decode_output_buffer_too_small);
    ctestprobe_register("decode_rejects_truncated_input",  test_decode_rejects_truncated_input);
    return ctestprobe_main(argc, argv);
}
