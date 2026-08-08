# Tests Guide (`tests/`)

This document explains how Litecode testing is organized and how to add new test coverage safely.

For project overview, see [../README.md](../README.md).

---

## 1) Testing Goals

The test setup targets three concerns:
- correctness of language behavior,
- stability of error handling and exit codes,
- regression protection for future parser/runtime changes.

---

## 2) Test Layers

Litecode currently uses:

1. **Unit tests (GTest)**
- focused checks for scanner, parser/resolver, interpreter, environment helpers.

2. **Case-based integration tests (CTest + fixtures)**
- black-box program execution against expected stdout/stderr/exit code.

3. **Smoke script**
- script-level scenario validation for important flows and toggles.

---

## 3) Folder Structure

```text
tests/
  run_case.sh            # case runner used by CTest entries
  unit/
    scanner_test.cpp
    parser_resolver_test.cpp
    interpreter_test.cpp
    environment_test.cpp
  cases/
    <case_name>/
      input.lox
      expected.out
      expected.err
      expected.exit
```

---

## 4) Running Tests

From project root after build:

```bash
ctest --test-dir build --output-on-failure
```

To run the smoke suite:

```bash
./scripts/regression_smoke.sh ./build/litecode
```

For file-based program validation, the practical workflow is:

```bash
./build/litecode path/to/program.lox
```

This is the normal product path for executing a saved `.lox` script and is the most direct validation mode for user-facing behavior.

---

## 5) How Integration Cases Work

Each case directory in `tests/cases/*` must include:
- `input.lox`
- `expected.out`
- `expected.err`
- `expected.exit`

`run_case.sh` executes interpreter on `input.lox` and diffs actual outputs/exit against expected artifacts.

This gives deterministic end-to-end validation.

---

## 6) What Is Covered Today

Examples of existing integration coverage:
- lex errors and parser errors
- resolver semantic errors (`return` top-level, `this/super` misuse)
- arithmetic and type errors
- functions, closures, recursion
- classes, fields, methods, inheritance, `super`
- native function behavior

---

## 7) Adding a New Integration Test Case

1. Create new folder in `tests/cases/`.
2. Add required files (`input.lox`, `expected.*`).
3. Reconfigure/build so CMake test discovery picks up folder.
4. Run `ctest --test-dir build --output-on-failure`.

Tips:
- keep case names descriptive, e.g. `runtime_divide_by_zero`.
- test one main behavior per case.
- include precise expected stderr messages for error-path regression safety.

---

## 8) Unit Tests Overview

Current unit suites target:
- scanner tokenization behavior
- parser/resolver constraints
- interpreter semantics
- environment depth-based operations

Use unit tests for targeted logic and fast feedback.
Use integration cases for full-language behavior and process-level outputs.

---

## 9) CI Integration

GitHub Actions workflow (`.github/workflows/ci.yml`) runs:
- configure
- build
- ctest

Matrix:
- `ubuntu-latest`
- `macos-latest`

This helps detect portability issues early.

---

## 10) Testing Best Practices for This Repo

- Add tests with every semantic behavior change.
- Prefer deterministic outputs.
- Keep error text stable unless intentionally changed.
- For parser/resolver/interpreter changes, add both:
  - a focused unit test,
  - at least one integration case.

---

## 11) Related Documentation

- Lexer details: [../lexer/README.md](../lexer/README.md)
- Parser details: [../parser/README.md](../parser/README.md)
- Resolver details: [../resolver/README.md](../resolver/README.md)
- Interpreter details: [../interpreter/README.md](../interpreter/README.md)
