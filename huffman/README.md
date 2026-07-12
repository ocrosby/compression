# Canonical Huffman coding

## What it is

An entropy coder. Huffman assigns shorter bit codes to more frequent
symbols and longer codes to rarer ones, minimizing the expected code
length for a given symbol distribution. The codes form a *prefix code*:
no code is a prefix of another, so a bit stream can be decoded without
delimiters.

Canonical Huffman is the standard restatement where codes are determined
entirely by their *lengths* — which enables a compact codebook
representation on the wire.

## How it works

1. Count symbol frequencies in the input.
2. Build a Huffman tree: repeatedly merge the two lowest-frequency nodes
   into a parent whose frequency is their sum, until one root remains.
3. Assign codes by traversing the tree; the code length for each symbol
   is its depth.
4. Reassign codes canonically: sort symbols by `(length, symbol)`, then
   assign increasing binary values within each length class.
5. Emit code lengths (compact) instead of the tree, followed by the
   packed bit stream.

Worked example on `A A A B B C`:

    frequencies: A=3, B=2, C=1
    lengths:     A=1, B=2, C=2
    canonical:   A=0, B=10, C=11
    encoded:     0 0 0 10 10 11    (bits)

## Wire format

Self-contained frame: enough header metadata for decode to reconstruct
the codebook without external state. Concrete layout (header size,
code-length representation, bit-packing order) is internal to this
implementation and deliberately unspecified in the public API so the
implementer can pick.

Typical shape:

    [original length] [per-symbol code lengths] [packed bit stream]

## Complexity

- Encode: O(n + k log k) time where k is the alphabet size (≤ 256),
  O(k) extra space
- Decode: O(n) time with a table-driven decoder, O(k) extra space

Building the tree with a binary heap is O(k log k); a bucket-based
construction can drop it to O(k).

## Trade-offs

**Good on:** skewed symbol distributions — natural text, source code,
most binary formats with a dominant byte value.

**Bad on:** uniform distributions (near-random or already-compressed
data — the output expands slightly due to header overhead).

Cannot compress below the symbol entropy H(X); arithmetic coding can
approach the true entropy per-message limit that Huffman misses by up to
1 bit per symbol. Huffman is usually combined *upstream* with LZ77 (see
DEFLATE / gzip) so it codes match/literal tokens rather than raw bytes,
which is where its real-world wins come from.

**Edge case:** a single distinct symbol has entropy 0 but Huffman cannot
assign length 0 (an empty code is not decodable). This implementation
forces length 1 in that case.

## References

- Huffman, "A Method for the Construction of Minimum-Redundancy Codes"
  (*Proc. IRE*, 1952)
- RFC 1951 (DEFLATE) §3.2.2 — canonical Huffman as used in gzip / zip / png
- Sayood, *Introduction to Data Compression* — Ch. 3
