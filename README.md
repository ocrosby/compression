# compression

A lab of compression-algorithm experiments in C.

![CI](https://github.com/ocrosby/compression/actions/workflows/ci.yml/badge.svg)

## Table of contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Development](#development)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

## Overview

Each classical compression algorithm lives in its own top-level directory
with a self-contained `src/`, `include/`, `tests/`, and `Makefile`, so
experiments can be built and studied in isolation without a repo-wide
build system fighting the exercise. The goal is understanding, not
performance — every implementation trades cleverness for clarity, and
every algorithm directory ships a `README.md` explaining the concept.

```text
compression/
├── <algorithm>/
│   ├── src/
│   ├── include/
│   ├── tests/
│   ├── Makefile
│   └── README.md    # concept, worked example, complexity, references
└── README.md
```

## Features

- **`rle/`** — byte-level run-length encoding with `(count, byte)` pairs;
  runs longer than 255 split; decoder rejects odd-length input and zero
  counts
- **`huffman/`** — canonical Huffman coding per RFC 1951 §3.2.2; frame is
  an 8-byte length prefix, 256 code-length bytes, and an MSB-first packed
  bit stream
- **`shannon-fano/`** — Fano's top-down split variant sharing the wire
  format with `huffman/` via canonical code reassignment
- Byte-pinned tests where the wire format is deterministic (RLE); round-
  trip tests where it isn't (Huffman, Shannon–Fano)
- Shared test harness via [ctestprobe](https://github.com/ocrosby/ctestprobe)
  from a sibling checkout — no vendored dependency, no submodule
- CI runs each algorithm's suite with `-Wall -Wextra -Wpedantic`, plus a
  parallel sanitize job under ASan + UBSan

## Requirements

- A C11-capable compiler (`cc`, `clang`, or `gcc`)
- `make` and `ar` (POSIX toolchain)
- [ctestprobe](https://github.com/ocrosby/ctestprobe) v2.2.0 or newer,
  cloned as a sibling of this repo (`../ctestprobe` from the repo root)

## Installation

Clone both repos side by side:

```sh
git clone https://github.com/ocrosby/compression.git
git clone https://github.com/ocrosby/ctestprobe.git
```

There is nothing to install repo-wide — each algorithm builds in place.

## Usage

Pick an algorithm and build it. For example, running the run-length
encoding suite:

```sh
cd compression/rle
make test
```

Green output looks like:

```text
Test Results:
  [PASS] encode_empty                             0.001 ms
  [PASS] encode_single_byte                       0.000 ms
  [PASS] encode_run_of_five                       0.000 ms
  ...
  [PASS] round_trip                               0.000 ms

12 registered, 12 passed, 0 failed
```

Each algorithm's `Makefile` invokes ctestprobe's `lib` target via sub-make
if `libctestprobe.a` isn't built yet.

## Configuration

Algorithms have no runtime configuration. Test binaries accept ctestprobe's
CLI flags — see the
[ctestprobe configuration reference](https://github.com/ocrosby/ctestprobe#configuration).
The most useful in this repo:

```sh
./build/test_huffman --list                # list registered tests
./build/test_huffman --filter=round_trip   # run only tests whose name matches
./build/test_huffman --tap                 # TAP v14 output for CI parsers
./build/test_huffman --junit=out.xml       # Surefire JUnit XML report
./build/test_huffman --suite=huffman       # override JUnit suite/classname
```

## Development

Each algorithm supports the same three targets:

```sh
make        # build the test binary
make test   # build and run
make clean  # remove build/
```

To run every algorithm under sanitizers locally, from the parent of both
repos:

```sh
export CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  -fno-omit-frame-pointer"

make -C ctestprobe clean lib
for d in rle huffman shannon-fano; do
  make -C compression/$d clean test
done
```

CI runs both a plain-warning build and the sanitizer sweep on every push
and PR to `main` — see
[`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## Roadmap

Implemented: **rle**, **huffman**, **shannon-fano**.

Planned:

- **lz77** — sliding-window dictionary coding
- **lz78** — explicit-dictionary variant
- **lzw** — LZ78 + implicit dictionary (as used in GIF and `compress(1)`)
- **arithmetic** — arithmetic coding
- **bwt** — Burrows–Wheeler transform + move-to-front + entropy stage

## Contributing

This is a personal learning lab; the code is public but not soliciting
contributions. If you spot a bug or a mistake in an algorithm's
explanation, opening an issue is welcome.

## License

TBD — no `LICENSE` file has been added to this repo yet.
