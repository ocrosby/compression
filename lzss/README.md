# Lempel-Ziv-Storer-Szymanski (LZSS)

## What it is

A sliding-window dictionary coder. LZSS is the practical refinement of
LZ77: at each step it either emits a *literal* byte or a *back-
reference* (offset, length) into the recent history, whichever is
cheaper. A single flag bit per token tells the decoder which is which.

This is the coder used, sometimes under other names, in LHA / ARJ /
early game-cartridge decompressors and as the match layer inside modern
formats like DEFLATE (before Huffman is applied on top).

## How it works

Maintain a sliding **window** of the last N bytes already emitted, plus
a **lookahead** of the next few bytes waiting to be coded.

At each position, search the window for the longest prefix that matches
the lookahead:

- If the best match is shorter than `MIN_MATCH`, emit the current byte
  as a literal (1 byte + 1 flag bit).
- Otherwise emit a back-reference `(offset, length)` and advance by
  `length` bytes.

The Storer-Szymanski insight is the *and-otherwise-a-literal* branch:
LZ77 always emits a triple `(offset, length, next_char)` — LZSS uses one
flag bit to save the bytes that would be wasted encoding "no match" as
a zero-length triple.

Worked example on `abcabcabcabc`:

    pos  window     lookahead      token           advance
    ---  --------   ------------   -------------   -------
    0    -          abcabcabcabc   literal 'a'     +1
    1    a          bcabcabcabc    literal 'b'     +1
    2    ab         cabcabcabc     literal 'c'     +1
    3    abc        abcabcabc      match (off=3, len=9)   +9
    12   abcabcabc  -              (done)

    encoded tokens: L'a' L'b' L'c' M(3,9)
    flag byte:      0b00000111   (bits 0..2 = literal, bit 3 = match)

The match at position 3 references three bytes back and copies nine
bytes forward — because the source and destination *overlap*, the copy
proceeds byte-by-byte and the just-copied bytes feed into subsequent
copies. This is how LZSS encodes long runs.

## Wire format

    bytes 0..7   original length, little-endian uint64
    bytes 8..    sequence of groups: <flag byte><up to 8 tokens>

Each **flag byte** carries the type of the next 8 tokens, LSB-first:

- flag bit = 1 → literal, 1 byte follows
- flag bit = 0 → match, 2 bytes follow

Each **match token** packs a 12-bit offset and a 4-bit length across
2 bytes:

    b0 = offset[11:4]
    b1 = (offset[3:0] << 4) | (length - 3)

Offset is in `[1, 4096]`; length is in `[3, 18]`. The offset gives the
match's distance back from the current output position; length is
biased by `MIN_MATCH = 3` so `length - 3 ∈ [0, 15]` fits in 4 bits.

The final flag byte may cover fewer than 8 tokens; the decoder stops
when the header's original length is reached, so unused flag bits are
ignored.

## Complexity

- Encode: O(n · C · L) where `C` is the hash-chain cap (32 here) and
  `L` is `MAX_MATCH` (18). O(H + W) extra space for the hash head and
  chain arrays (H = 4096 slots, W = 4096 slots).
- Decode: O(n) time, O(1) extra space beyond the output buffer.

A truly naive encoder searches the entire window at each position
(O(n · W · L)). This implementation uses a 3-byte hash to jump directly
to positions whose first 3 bytes agree, capped at 32 candidates per
position — the same idea as `gzip`'s inner loop, minus the many other
tricks (lazy matching, variable chain cap, longest-first hash inserts).

## Trade-offs

**Good on:** any input with local repetition — English text, source
code, log files, XML/JSON. LZSS discovers the repetition itself rather
than requiring a probability model, and it excels on inputs that repeat
short phrases at short-to-medium distances.

**Bad on:** high-entropy data (already-compressed, encrypted, random)
where matches are rare. Every literal costs 9 bits (1 flag + 8 data)
instead of 8, so incompressible input expands by ~12.5% plus the
8-byte header.

**Window / lookahead trade-off:** a bigger window catches more distant
repetitions but costs more bits per match token. A longer max match
helps on RLE-like data but wastes bits on typical text. This
implementation's `(12, 4)` split — 4096-byte window, 18-byte max match —
is the classic Okumura choice popularised by SLZ and reused by
countless "small LZSS" implementations since.

**No entropy stage:** LZSS emits bytes and fixed-width fields. Real
codecs stack an entropy coder (Huffman in DEFLATE, arithmetic in LZMA)
on top of the LZSS token stream, gaining another ~30% typically.

## References

- Storer & Szymanski, "Data Compression via Textual Substitution"
  (*JACM*, 1982)
- Ziv & Lempel, "A Universal Algorithm for Sequential Data Compression"
  (*IEEE Trans. Info. Theory*, 1977) — the LZ77 origin
- Okumura, "Data Compression Algorithms of LARC and LHarc" (1988) — the
  canonical small-C LZSS reference
- Nelson & Gailly, *The Data Compression Book* — Ch. 8
