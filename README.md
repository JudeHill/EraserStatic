# EraserStatic

EraserStatic is a C++ static analysis tool for detecting potential data races in C programs using `pthreads`. It parses source code, constructs a control-flow graph, runs a barrier-aware Lockset analysis, and can optionally use LLM-based extensions to analyse likely false positives and false negatives. 

This project is distributed under the terms of the GNU General Public License version 3.0. Portions of the parsing and control-flow graph infrastructure are derived from and extend the Eraser-CD project ([Eraser-CD](https://github.com/ProgrammerByte/Eraser-CD)), which is likewise distributed under the GNU General Public License version 3.0.

## Requirements

- C++20 compiler, e.g. GCC
- CMake
- Ninja
- Clang / libclang
- libcurl
- nlohmann/json
- CLI11
- API keys for LLM functionality, if using LLM options

## Build

```bash
cmake --preset dev
cmake --build --preset all

## Usage

Analyse a single C file:

```bash
eraser_static <input_file.c> <output_file> <options>
```

Analyse all `.c` / `.C` files in a directory:

```bash
eraser_static <input_directory> <output_file> <options>
```

Example:

```bash
eraser_static . out.log
```

The output is a data-race report listing potentially unprotected shared-variable accesses and their source locations.

## Options

| Option | Description |
|---|---|
| `-v`, `--verbose` | Enable verbose/debug output |
| `-g`, `--show-graph` | Visualise the generated CFG |
| `-s`, `--symmetric-join` | Use symmetric join during Lockset analysis |
| `-b`, `--no-barrier` | Disable barrier-aware analysis |
| `--no-llms` | Disable all LLM-based analysis |
| `--eval-llms` | Evaluate shared-variable LLM consistency |
| `--eval-fps` | Evaluate false-positive detection using LLMs |
| `--eval-fns` | Evaluate false-negative detection using LLMs |
| `--slow-llms` | Slow LLM requests to avoid API rate limiting |
| `--test-llms` | Test connectivity to configured LLM APIs |
| `--write-all` | Write all reported accesses instead of truncating output |
| `-t`, `--variant-responses` | Increase LLM temperature to allow non-deterministic responses |
| `--llm <model>` | Specify the LLM to use (`gemini`, `claude`) |

## Input Constraints

EraserStatic assumes a restricted class of structured C programs using `pthreads`. Violating these assumptions may lead to incomplete CFG construction, incorrect analysis, or missed data races.

### Unsupported Language Features

The following constructs are not supported:

- `goto` statements
- Non-local jumps (`setjmp` / `longjmp`)
- Function calls via function pointers
- Inline assembly
- Programs exhibiting undefined behaviour

### CFG Construction Assumptions

`switch` statements are only supported when each clause contains thread-local operations only.

A thread-local operation is any operation that is **not**:

- an access to a shared variable
- an operation on a synchronisation primitive
- a `pthread_create` or `pthread_join` call
- a function call

### Supported `for` Loop Forms

`for` loops must match one of the following forms:

```c
for (;;) {}
for (;;) { body }
for (; cond ;) { body }
for (init; cond ;) { body }
for (init; cond; increment) { body }
```

### Concurrency Assumptions

The analysis additionally assumes that:

- Thread creation and joining are balanced
- All threads reach all barriers
- Barrier counts correctly match the number of participating threads
- Shared variables can be identified statically
- Programs use structured synchronisation patterns

### Pointer Analysis Limitations

The static analysis primarily identifies globally shared variables. Pointer-based sharing is approximated using optional LLM-based analysis and may be incomplete.
