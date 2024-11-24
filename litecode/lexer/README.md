## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Directory Structure](#directory-structure)
- [Setup and Installation](#setup-and-installation)
- [Usage](#usage)
- [Code Explanation](#code-explanation)

## Overview

The lexer is the first phase in the interpretation or compilation of source code. It reads the source code character by character and groups them into meaningful sequences called tokens. These tokens are then passed to the parser for syntactic analysis.

This lexer supports:
- Single-character tokens (e.g., `(`, `)`, `{`, `}`).
- Multi-character tokens (e.g., `==`, `!=`, `<=`, `>=`).
- Literals (e.g., strings, numbers).
- Keywords (e.g., `if`, `else`, `for`, `while`, `return`).
- Ignoring whitespaces and comments.

## Features

- **Tokenization**: Converts source code into a list of tokens.
- **Error Handling**: Reports errors encountered during lexing.
- **Interactive and Script Mode**: Supports both REPL (interactive mode) and executing from a script file.

## Directory Structure
```
lox-lexer/
├── include/             # All header files
│   ├── Scanner.hpp
│   ├── Token.hpp
│   ├── TokenType.hpp
│   ├── ErrorReporter.hpp
├── src/                 # All source files
│   ├── main.cpp
│   ├── Scanner.cpp
│   ├── Token.cpp
│   ├── ErrorReporter.cpp
├── tests/               # Unit tests
│   ├── ScannerTests.cpp
│   ├── TokenTests.cpp
├── tools/               # Optional tools (e.g., benchmarking, script examples)
│   ├── example_script.lox
├── CMakeLists.txt       # CMake build file
└── README.md            # Project overview and documentation
```

## Setup and Installation

### Prerequisites

- **C++17 or higher**: Ensure your compiler supports C++17 or a later standard.
- **Make** (optional): If you prefer using a `Makefile`.

### Compiling the Lexer

To compile the lexer, run the following command:

```bash
g++ -std=c++17 -o toylang main.cpp
```

Alternatively, if you use `make`, you can create a `Makefile` with appropriate targets.

### Running the Lexer

To run the lexer, you can use two modes:

1. **Interactive Mode** (REPL):
   
   ```bash
   ./toylang
   ```

   This will start the interpreter in interactive mode where you can type Lox statements directly.

2. **Script Mode**:

   ```bash
   ./toylang path/to/your/script.lox
   ```

   This will run the specified Lox script.

## Code Explanation

### 1. `TokenType` and `Token` Classes

The `TokenType` enum defines all possible token types, including single-character tokens, multi-character tokens, literals, and keywords.

The `Token` class represents a token with a type, lexeme (the text that the token represents), a literal value (for string or numeric literals), and the line number from which it was parsed.

### 2. `Scanner` Class

The `Scanner` class is responsible for converting the input source code into a list of tokens. It supports methods to:

- **Advance** through the source code.
- **Peek** at the current and next characters without advancing.
- **Match** expected characters.
- **Skip whitespaces and comments**.
- **Identify literals and keywords**.
  
The scanner also handles errors gracefully and adds tokens to the output list.

### 3. `litecode` Class (Driver)

The `litecode` class is the driver of the program that manages input and output. It reads source code from files or the standard input (for interactive mode) and invokes the `Scanner` to tokenize the input.

### Error Handling

Errors are reported directly to the console with the line number and an error message. If errors are found during scanning, the interpreter exits with an appropriate error code.
