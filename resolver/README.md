# Resolver Module (`resolver/`)

This document explains Litecode's resolver stage: the static semantic pass between parsing and runtime interpretation.

Resolver is one of the most important concepts for beginners because it connects lexical scope theory to practical runtime behavior.

For the high-level project map, see [../README.md](../README.md).

---

## 1) Why a Resolver Exists

The parser builds syntax trees, but it does not decide *which declaration each variable use should bind to*.

Resolver solves that by:
- traversing AST before execution,
- tracking lexical scopes,
- assigning scope-depth metadata for variable expressions,
- enforcing static language rules (`return`, `this`, `super`, etc.).

Without resolver, closures and nested scopes can produce incorrect variable lookup behavior.

---

## 2) Files in This Module

```text
resolver/
  inc/
    Resolver.hpp
  src/
    Resolver.cpp
```

`Resolver` collaborates with the interpreter by calling `interpreter.resolve(exprPtr, depth)`.

---

## 3) Core Data Model

Resolver tracks:
- `scopes`: stack of hash maps (`name -> defined?`)
- `currentFunction`: context (`NONE`, `FUNCTION`, `METHOD`, `INITIALIZER`)
- `currentClass`: context (`NONE`, `CLASS`, `SUBCLASS`)
- `hasError`: whether static resolution errors were found

### Meaning of `defined?`
- declared but not defined (`false`) prevents self-initializer reads
- defined (`true`) allows reads

---

## 4) Scope Resolution Mechanism

When resolver sees variable usage:
1. search scope stack from innermost outward,
2. compute distance to matched scope,
3. record `(Expr* -> distance)` in interpreter metadata.

At runtime, interpreter uses this depth with `Environment::getAt` / `assignAt`, avoiding full-chain name search for locals.

---

## 5) Static Checks Implemented

Resolver reports language errors for:
- `return` outside function
- `return <value>` inside initializer (`init`)
- reading local variable in its own initializer
- duplicate local declaration in same scope
- `this` outside class
- `super` outside class
- `super` in class without superclass
- class inheriting from itself

These are caught before interpreter execution of statements.

---

## 6) Class-Specific Resolution Behavior

For class declarations:
- declare/define class name in current scope
- if superclass exists:
  - validate not self-inheritance
  - resolve superclass expression
  - open scope with `super`
- open method scope with `this`
- resolve each method with context:
  - `init` => `INITIALIZER`
  - others => `METHOD`

This ensures `this` and `super` semantics are legal and runtime-bindable.

---

## 7) Function Resolution Behavior

For function declarations:
1. declare and define function name in enclosing scope
2. switch function context type
3. begin inner scope
4. declare/define each parameter
5. resolve function body
6. restore previous function context

This creates proper lexical binding for closures.

---

## 8) Error Style and Pipeline Behavior

Resolver writes diagnostics to `stderr` with line and token lexeme context.

If any resolver error occurs:
- `hasError` is set,
- top-level driver halts before interpretation,
- file mode exits with language error code (`65`).

---

## 9) Beginner Mental Model

Think of resolver as a "symbol binder":
- parser answers "Is syntax valid?"
- resolver answers "Do names and contexts make semantic sense?"
- interpreter answers "Now execute it"

This separation reduces runtime surprises and keeps interpreter logic simpler.

---

## 10) Related Modules

- AST source: [../parser/README.md](../parser/README.md)
- Runtime consumer: [../interpreter/README.md](../interpreter/README.md)
- Integration checks: [../tests/README.md](../tests/README.md)

---

## 11) Extension Ideas

Potential resolver extensions:
- unused variable warnings
- shadowing warnings
- richer context in diagnostics (scope trace)
- optional strict mode for style checks

If you extend resolver rules, add matching tests in both unit and integration suites.
