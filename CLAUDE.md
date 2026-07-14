# compression — Claude notes

A lab of compression-algorithm experiments in C. Each algorithm lives in its
own top-level directory (`rle/`, `huffman/`, etc.) with a self-contained
`src/`, `include/`, `tests/`, and `Makefile`, so experiments can be built
and studied in isolation.

## Per-directory READMEs (mandatory)

Every algorithm directory must contain a `README.md` explaining the concept
behind the algorithm — not just how to build it. When adding a new algorithm
directory, or working in one that lacks a README, create the README as part
of the same change. Do not leave an algorithm directory without one.

The README should cover, in this order:

1. **What it is** — one paragraph naming the algorithm and its family
   (entropy coder, dictionary coder, transform, etc.)
2. **How it works** — the core idea in plain prose, then a small worked
   example on a short input
3. **Wire format** — how encoded output is laid out; use "internal to this
   implementation" if the format is deliberately unspecified
4. **Complexity** — time and space, encode and decode
5. **Trade-offs** — where it shines, where it breaks down, what it's
   typically paired with
6. **References** — original paper, canonical write-up, or standard

Target ~100–150 lines. This is a lab; the README is a learning aid, not a
formal spec.

Per-directory READMEs are algorithm-specific. Cross-cutting notes
(comparisons across algorithms, shared math, benchmarking methodology)
belong under `docs/` instead.

## Algorithm conventions

- All algorithms follow the same API shape: `<name>_encode(in, in_len, out,
  out_cap)` and `<name>_decode(...)`, returning bytes written or
  `<PREFIX>_ERROR` on failure.
- Tests are round-trip only where the wire format has legitimate
  implementation freedom (Huffman, LZ*); byte-pinned where it does not (RLE).
- Every `<algorithm>/Makefile` supports `make`, `make test`, `make clean`
  and writes artifacts to `build/`.
- Each algorithm directory is self-contained *between algorithms* — no
  shared headers between `rle/`, `huffman/`, etc. Duplicating a small
  helper between them is preferred over factoring one into a `common/`
  directory until three consumers exist. `common/` currently hosts the
  8-byte length header (`hdr.h`) and the MSB-first bit stream
  (`bitstream.h`); anything shared by only two algorithms stays
  duplicated.
- Tests use [ctestprobe](https://github.com/ocrosby/ctestprobe) linked
  from a sibling checkout at `../ctestprobe`. Each test function is
  `static void`, uses `CTP_ASSERT` / `CTP_ASSERT_EQ_INT` /
  `CTP_ASSERT_EQ_BYTES` for assertions, and is wired up in `main` via
  `ctestprobe_register` before `ctestprobe_run_all` and
  `ctestprobe_console_report`.

## Build

Requires ctestprobe cloned as a sibling of this repo:

    git clone https://github.com/ocrosby/ctestprobe.git ../ctestprobe

Then each experiment builds in isolation:

    cd <algorithm>
    make test

The algorithm's Makefile invokes both ctestprobe's and `common/`'s `lib`
targets via sub-make if their static libraries aren't built yet.
