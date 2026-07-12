# Run-Length Encoding (RLE)

## What it is

The simplest lossless compressor. RLE replaces runs of identical bytes with
a `(count, byte)` pair. It is not an entropy coder — it makes no attempt to
model symbol probabilities. Its wins come purely from repetition.

## How it works

Walk the input and emit one pair per maximal run:

    input:  A A A B B C C C C
    output: (3,A) (2,B) (4,C)

In this implementation, `count` is a `uint8_t` in `[1, 255]`, so a run of
300 `Z` is split into two pairs: `(255,Z) (45,Z)`.

## Wire format

A raw stream of `(count, byte)` pairs. No header, no length prefix — the
decoded length is implicit in the sum of counts.

    byte 0: count_0   (1..255)
    byte 1: byte_0
    byte 2: count_1
    byte 3: byte_1
    ...

`rle_decode` treats these as failures:

- odd-length input (dangling count with no byte)
- a count byte of zero
- output buffer too small

## Complexity

- Encode: O(n) time, O(1) extra space
- Decode: O(n) time, O(1) extra space (n = decoded length)

The encoder's inner run-counting loop advances the outer index by the run
length, so each input byte is visited once total.

## Trade-offs

**Good on:** bitmap regions with wide flat color, ASCII with long
whitespace runs, sparse data dominated by a single byte value (zeros).

**Bad on:** natural text, most binary data, anything without runs — the
worst case *doubles* the input size (every byte becomes `(1, byte)`).

RLE is often used as a *stage* in a larger pipeline (e.g., after
Burrows–Wheeler + move-to-front, or inside a PCX / BMP variant) rather
than standalone.

## References

- Salomon, *Data Compression: The Complete Reference* — Ch. 1
- ITU-T T.4 (fax Group 3) — a bit-level RLE variant
