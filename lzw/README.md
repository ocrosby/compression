# Lempel-Ziv-Welch (LZW)

## What it is

A dictionary coder. LZW builds an ever-growing table of previously-seen
strings and replaces each new occurrence with a fixed-width code. The
dictionary is *implicit* — encoder and decoder grow it in lock-step from
the stream itself, so no codebook is transmitted. LZW is the coder
behind `compress(1)`, GIF, and TIFF LZW.

This implementation is the "basic" form: fixed 12-bit codes, no CLEAR
code, dictionary frozen once full.

## How it works

The dictionary starts with codes `0..255` mapped to the 256 single-byte
strings. Codes `256..4095` are grown as encoding proceeds.

Encoder pseudocode:

    w = first byte
    for each subsequent byte c:
        if (w + c) is in the dict:
            w = w + c            # keep extending the match
        else:
            emit code for w
            add (w + c) to dict  # new entry gets the next code
            w = c
    emit code for w

Decoder pseudocode mirrors it and rebuilds the same dictionary from the
code stream alone, with one special case: the encoder can emit a code
the very moment it is added. The decoder detects this (`code ==
next_code`) and reconstructs the entry as `prev + prev[0]`. This is the
classic *KwKwK* trick.

Worked example on `AAAAA`:

    step  in  w     dict lookup    emit   dict add    output
    ----  --  ----  -------------  -----  ----------  ----------
    0     A   65    -              -      -           w=65
    1     A   (65,A) miss          65     (65,A)→256  A
    2     A   256   -              -      -           w=256
    3     A   (256,A) miss         256    (256,A)→257 AA         (KwKwK on decode)
    4     A   257   -              -      -           w=257
    end       -     -              257    -           AA

    encoded codes: 65, 256, 257    (three 12-bit codes)
    decoded:       A, AA, AA       = "AAAAA"

## Wire format

Self-contained frame:

    bytes 0..7   original length (little-endian uint64)
    bytes 8..    packed 12-bit codes, MSB-first

Two 12-bit codes pack into three bytes. If the total code count is odd,
the final nibble is padding. The decoder ignores trailing padding
because the header's original length tells it exactly when to stop.

## Complexity

- Encode: O(n) time, O(D) extra space, where D is the dictionary cap
  (4096 entries). Lookup is an open-addressed hash table sized to a
  prime above D for ~O(1) probe.
- Decode: O(n) time. Dictionary walk to reconstruct a code's string is
  bounded by the entry's chain depth (≤ D). Extra space is O(D) for the
  parent-link dictionary plus a D-byte reverse-walk stack.

## Trade-offs

**Good on:** inputs with reusable substrings — English text, source
code, tabular data, XML/JSON. LZW discovers the repetition itself, so
unlike Huffman it wins on structure, not just symbol skew.

**Bad on:** short inputs (dictionary hasn't warmed up), high-entropy
data (already-compressed, encrypted, random — expands 1.5x due to the
fixed 12-bit code per literal byte).

**Frozen dictionary:** once the dictionary fills, this implementation
stops adding entries and keeps coding against the frozen table. Real-
world LZW variants either reset the dictionary on a CLEAR code (GIF) or
grow to a threshold and monitor compression ratio (`compress(1)`).
Neither is implemented here — the point of "basic" is to isolate the
core idea from those refinements.

**Variable-width codes:** production LZW starts at 9-bit codes and
grows the width as the dictionary fills, saving 3 bits per code for the
first 256 new entries. This implementation uses a fixed 12-bit width to
keep the bit-packer trivial.

## References

- Welch, "A Technique for High-Performance Data Compression"
  (*IEEE Computer*, 1984)
- Ziv & Lempel, "Compression of Individual Sequences via Variable-Rate
  Coding" (*IEEE Trans. Info. Theory*, 1978) — the LZ78 basis
- Nelson & Gailly, *The Data Compression Book* — Ch. 9
- Salomon, *Data Compression: The Complete Reference* — §6.13
