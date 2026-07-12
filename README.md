# compression

A lab of compression-algorithm experiments in C.

Each algorithm lives in its own top-level directory with a self-contained
`src/`, `tests/`, and `Makefile`, so experiments can be built and studied
in isolation without a repo-wide build system fighting the exercise.

## Layout

```
compression/
├── docs/            # algorithm notes, references, complexity analysis
├── <algorithm>/     # one directory per experiment
│   ├── src/
│   ├── include/
│   ├── tests/
│   └── Makefile
└── README.md
```

## Planned experiments

- **rle** — Run-Length Encoding (simplest; establishes the pattern)
- **huffman** — canonical Huffman coding
- **lz77** — sliding-window dictionary coding
- **lz78** — explicit-dictionary variant
- **lzw** — LZ78 + implicit dictionary (GIF, `compress(1)`)
- **arithmetic** — arithmetic coding
- **bwt** — Burrows–Wheeler transform + move-to-front + entropy stage

None are implemented yet — this README documents the intended shape.

## Building an experiment

```sh
cd rle
make
make test
```
