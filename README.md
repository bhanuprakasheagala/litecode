# Litecode

Litecode is a from-scratch interpreter project inspired by [Crafting Interpreters](https://craftinginterpreters.com/) by Robert Nystrom.

Current implementation is a **C++17 tree-walk interpreter** with scanner, parser, resolver, and runtime fully connected. The long-term goal is to continue into a **C-based bytecode VM path**.

This root README is the **high-level guide** for the entire project.
Detailed deep-dives are split into per-module READMEs.

---

## What This Project Is

Litecode is a learning-focused language implementation project that helps you understand:
- lexical analysis (scanner)
- parsing and AST construction
- static scope resolution
- runtime interpretation
- functions, closures, classes, inheritance

If you are a beginner, this repo is meant to be read layer-by-layer.

---

## Documentation Map

Read in this order:

1. **Project overview (this file)**
2. [1-overview.md](docs/1-overview.md)
3. [2-architecture.md](docs/2-architecture.md)
4. [3-language-reference.md](docs/3-language-reference.md)
5. [4-runtime-and-execution.md](docs/4-runtime-and-execution.md)
6. [5-developer-guide.md](docs/5-developer-guide.md)
7. [6-testing-validation.md](docs/6-testing-validation.md)
8. [7-roadmap-status.md](docs/7-roadmap-status.md)

Module-specific deep dives are still available in the per-module READMEs for source-level reference:

- [Lexer Deep Dive](lexer/README.md)
- [Parser + AST Deep Dive](parser/README.md)
- [Resolver Deep Dive](resolver/README.md)
- [Interpreter Runtime Deep Dive](interpreter/README.md)
- [Testing Guide](tests/README.md)

---

## High-Level Architecture

```mermaid
flowchart TD
  A["Source (.lox)"] --> B["Scanner / Lexer"]
  B --> C["Token stream"]
  C --> D["Parser"]
  D --> E["AST (Expr + Stmt)"]
  E --> F["Resolver (scope binding)"]
  F --> G["Interpreter (tree-walk runtime)"]
  G --> H["stdout / stderr / exit code"]
```

Core execution order in `main.cpp`:
1. Scan source into tokens.
2. Parse tokens into statements AST.
3. Resolve lexical scope/static constraints.
4. Execute with interpreter.

---

## Repository Layout

```text
litecode/
  lexer/         # scanner/tokenization module + docs
  parser/        # AST + recursive descent parser + docs
  resolver/      # lexical scope resolver + docs
  interpreter/   # runtime/evaluator + docs
  tests/         # unit and integration tests + docs
  scripts/       # smoke/regression scripts
  docs/          # status documents
  main.cpp       # entrypoint (file mode + REPL)
```

---

## Implemented Language Features

- Literals: `nil`, booleans, numbers, strings
- Expressions: arithmetic, comparison, equality, unary, logical operators
- Statements: `print`, `var`, block, `if/else`, `while`, `for`, `return`
- Functions: declarations, calls, closures, recursion
- Classes: instances, fields, methods, `this`, inheritance, `super`
- Native function: `clock()`

For detailed semantics and examples, see [interpreter/README.md](interpreter/README.md).

---

## Build and Run

### Prerequisites
- CMake 3.16+
- C++17 compiler

### Quick build commands

Use the project wrapper script to avoid repeating the configure/build/test steps manually:

```bash
./scripts/build.sh default
./scripts/build.sh default test
./scripts/build.sh asan
./scripts/build.sh asan test
./scripts/build.sh leak
./scripts/build.sh clean
```

The standard build writes to `build`, and the sanitizer build writes to `build-asan` in a separate directory so address-sanitizer artifacts do not mix with the normal build.

`./scripts/build.sh leak` is an optional, tool-dependent memory check. On Linux it prefers `valgrind`; on macOS it prefers the native `leaks` tool. These are advisory diagnostics rather than a required product gate, and on this codebase they may report reference cycles in the runtime object graph rather than outright leaks. For routine development, the recommended cross-platform option is still the ASan build: `./scripts/build.sh asan test`.

You can also use the equivalent CMake presets directly:

```bash
cmake --preset default
cmake --build --preset default
cmake --preset asan
cmake --build --preset asan
```

### Run a script file

Use a `.lox` source file when you want to execute a complete program in one shot:

```bash
./build/litecode path/to/file.lox
```

This is the recommended way to run finished programs and is the default product workflow for file-based validation.

### Run the REPL

Use the interactive prompt when you want to type Lox code line by line:

```bash
./build/litecode
```

REPL commands:
- `.help` shows available commands
- `.reset` clears the current interpreter state
- `.quit` exits the prompt

REPL helper command:
- `.reset` clears current REPL state (variables/functions/classes) and frees retained session AST state.
- optional env knob: `LITECODE_REPL_AUTO_RESET_EVERY=<N>` auto-resets REPL state after every `N` successful inputs (default disabled).

### Debug toggles

```bash
LITECODE_DUMP_TOKENS=1 ./build/litecode sample.lox
LITECODE_DUMP_AST=1 ./build/litecode sample.lox
```

---

## Testing and Validation

### Full test suite

```bash
ctest --test-dir build --output-on-failure
```

### Smoke checks

```bash
./scripts/regression_smoke.sh ./build/litecode
```

Detailed test documentation: [tests/README.md](tests/README.md)

---

## Exit Codes and Error Model

- `64`: command usage error
- `65`: language error (lex/parse/resolve/runtime)
- `66`: file input error (for example missing script)

---

## Progress vs Crafting Interpreters

Current state aligns approximately through late `jlox` chapters:
- scanning, parsing, expressions/statements, control flow
- functions and closures
- resolver and lexical binding
- classes and inheritance (`this`/`super`)

Progress details: [docs/7-roadmap-status.md](docs/7-roadmap-status.md)

---

## Known Tradeoffs

- REPL keeps AST batches alive for safe function/class pointer lifetimes; long sessions can grow memory.
- No garbage collector yet (smart pointers currently handle ownership).
- Internal naming has a few intentional differences (for example `NILL`, `LEFT_PARENTH`) but is consistent.

---

## Roadmap Direction

### Short-term
- tighten diagnostics and polish docs/examples
- expand targeted edge-case tests

### Mid-term
- improve REPL memory strategy while preserving correctness
- continue optional VM bootstrap work as a secondary implementation path

### Long-term
- evaluate whether the bytecode VM should remain a bootstrap branch or become a future alternative runtime

---

## External References

- [Crafting Interpreters](https://craftinginterpreters.com/)
- [The Lox language (book chapter)](https://craftinginterpreters.com/the-lox-language.html)
- [A Tree-Walk Interpreter (book chapter)](https://craftinginterpreters.com/a-tree-walk-interpreter.html)
- [CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
- [GoogleTest docs](https://google.github.io/googletest/)

---

## License

See [LICENSE](LICENSE).
