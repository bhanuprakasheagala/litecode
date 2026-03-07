# Parser Module (`parser/`)

This document explains Litecode's parser and AST layer in detail.

The parser converts token streams from the lexer into structured AST nodes.
This module also includes an AST printer used for debugging.

For project-level context, see [../README.md](../README.md).

---

## 1) What the Parser Does

Input:
- `std::vector<lexer::Token>`

Output:
- `std::vector<StmtPtr>` (program as a list of declarations/statements)

Responsibilities:
- enforce grammar and precedence,
- build AST objects,
- report parse errors with synchronization,
- produce AST form that resolver/interpreter can consume.

---

## 2) Files in This Module

```text
parser/
  inc/
    Expr.hpp
    Stmt.hpp
    Parser.hpp
    AstPrinter.hpp
  src/
    Parser.cpp
    AstPrinter.cpp
```

### `Expr.hpp`
Expression AST node hierarchy:
- `Binary`, `Unary`, `Grouping`, `Literal`
- `Variable`, `Assign`, `Logical`
- `Call`, `Get`, `Set`
- `This`, `Super`

### `Stmt.hpp`
Statement AST node hierarchy:
- `ExpressionStmt`, `PrintStmt`, `VarStmt`, `BlockStmt`
- `IfStmt`, `WhileStmt`
- `FunctionStmt`, `ReturnStmt`
- `ClassStmt`

### `Parser.hpp` / `Parser.cpp`
Recursive-descent parser implementation.

### `AstPrinter.*`
Debug-only representation utility for visualizing AST shape.

---

## 3) Parser Strategy: Recursive Descent

Litecode parser is hand-written recursive descent.
Each grammar rule is implemented as a C++ method.

Benefits for learning:
- grammar and code map directly,
- precedence is explicit by call layering,
- easy to debug and extend incrementally.

---

## 4) Grammar Shape (Implemented)

At a high level:

- Program -> many declarations
- Declarations -> class/fun/var/statement
- Statements -> block/if/while/for/return/print/expression
- Expressions -> assignment -> logical -> equality -> comparison -> term -> factor -> unary -> call -> primary

`for` loops are desugared into `while` + optional initializer/increment blocks while parsing.

---

## 5) Expression Precedence Mapping

From lowest to highest:
1. Assignment (`=`)
2. Logical OR (`or`)
3. Logical AND (`and`)
4. Equality (`==`, `!=`)
5. Comparison (`>`, `>=`, `<`, `<=`)
6. Term (`+`, `-`)
7. Factor (`*`, `/`)
8. Unary (`!`, unary `-`)
9. Call/property access (`()`, `.`)
10. Primary literals/grouping/identifiers/`this`/`super`

This avoids ambiguity and preserves expected arithmetic behavior.

---

## 6) Class and Function Parsing

### Function declarations
- `fun name(params) { body }`
- max params: 255

### Function calls
- `callee(args...)`
- max args: 255

### Class declarations
- `class Name { methods... }`
- subclass: `class Child < Parent { ... }`
- `super.method()` parsed as dedicated `Super` expression node

---

## 7) Parse Errors and Recovery

Parser uses a local `ParseError` signaling pattern and `synchronize()` to continue parsing after an error.

Common parse diagnostics include:
- missing `)` / `}`
- invalid assignment target
- too many arguments/parameters

If parse errors occur, the driver marks language error state and stops before resolver/interpreter phases.

---

## 8) AST Ownership Model

- Expressions and statements use `std::unique_ptr` (`ExprPtr`, `StmtPtr`).
- Program is represented as `std::vector<StmtPtr>`.
- In REPL mode, top-level stores AST batches to preserve pointer lifetimes needed by closures/method metadata.

---

## 9) Parser Example

Source:

```lox
print 1 + 2 * 3;
```

Structural interpretation:

```text
PrintStmt
  Binary(+)
    Literal(1)
    Binary(*)
      Literal(2)
      Literal(3)
```

Debug AST dump:

```bash
LITECODE_DUMP_AST=1 ./build/litecode sample.lox
```

---

## 10) Interfaces with Other Modules

- Consumes tokens from [../lexer/README.md](../lexer/README.md)
- Produces AST for [../resolver/README.md](../resolver/README.md)
- Final execution handled by [../interpreter/README.md](../interpreter/README.md)

Parser correctness is central: resolver and interpreter assume AST invariants are valid.

---

## 11) Extension Ideas

Safe parser-side feature additions:
- ternary operator (`?:`)
- comma expressions
- richer literal syntax
- improved parse diagnostics with context snippets

If you extend grammar, remember to update:
- AST node definitions
- parser precedence chain
- resolver behavior (if semantic impact)
- interpreter evaluation logic
- tests in `tests/cases` and `tests/unit`
