# Shannon–Fano coding

## What it is

An entropy coder — the direct historical precursor to Huffman coding.
Shannon–Fano produces a prefix code by *top-down splitting* of a
frequency-sorted symbol list, rather than Huffman's *bottom-up merging*.
It is nearly optimal but not guaranteed optimal: for some inputs Huffman
produces a strictly shorter encoding.

"Shannon–Fano" as used here refers to *Fano's* algorithm (1949).
Shannon's own scheme (sometimes called Shannon–Fano–Elias) uses
cumulative probabilities and is a different construction; it is not
implemented in this lab.

## How it works

1. Count symbol frequencies.
2. Sort symbols by frequency, descending.
3. Split the sorted list into two contiguous groups whose total
   frequencies are as equal as possible.
4. Prepend `0` to the codes of the first group, `1` to the second.
5. Recurse into each group until every group has one symbol.

Worked example on `A A A A B B C C D` (frequencies `A=4, B=2, C=2, D=1`,
total 9):

    split point balances totals as {A}=4 vs {B,C,D}=5:
      A = 0
      B,C,D = 1?
    recurse into {B,C,D} (total 5), split as {B}=2 vs {C,D}=3:
      B = 10
      C,D = 11?
    recurse into {C,D} (total 3), split as {C}=2 vs {D}=1:
      C = 110
      D = 111

Final codebook: `A=0, B=10, C=110, D=111`.

## Wire format

Self-contained frame: enough header metadata for decode to reconstruct
the codebook without external state. Concrete layout (header size,
code-length representation, bit-packing order) is internal to this
implementation.

Typical shape:

    [original length] [per-symbol code lengths] [packed bit stream]

Since Shannon–Fano produces a valid prefix code with defined lengths per
symbol, the same length-header + canonical-reassignment approach used
for canonical Huffman applies here — decode never needs the split tree,
only the resulting per-symbol code lengths.

## Complexity

- Encode: O(n + k log k) time (frequency sort + recursive split),
  O(k) extra space where k is the alphabet size (≤ 256)
- Decode: O(n) time with a table-driven decoder, O(k) extra space

## Trade-offs

**Historically important.** First practical entropy coder (1949),
preceded Huffman (1952) by three years. Understanding Shannon–Fano
clarifies *why* Huffman is bottom-up: greedy merging of the two lowest-
frequency symbols guarantees optimality; greedy top-down splitting does
not.

**Sub-optimal.** The canonical counter-example is the four-symbol
distribution `{15, 7, 6, 6}`:

    Shannon–Fano: expected 2.09 bits/symbol
    Huffman:      expected 2.03 bits/symbol   (optimal)

The gap is small in practice but real, and it is not a tie-breaking
artifact — no split choice can close it. There is no reason to prefer
Shannon–Fano over Huffman in a modern pipeline; it survives as a
teaching algorithm and a stepping stone to arithmetic coding.

## References

- Fano, "The Transmission of Information" — Technical Report No. 65,
  Research Laboratory of Electronics, MIT (1949)
- Shannon, "A Mathematical Theory of Communication" — *Bell System
  Technical Journal*, 27, pp. 379–423 (1948), appendix
- Sayood, *Introduction to Data Compression* — Ch. 3 (Shannon–Fano
  vs. Huffman)
