# litecode Lexer

This is the **lexer** module for the **litecode** interpreter, a toy programming language. The lexer (also known as the **scanner**) is responsible for breaking down source code into a sequence of **tokens**, which can then be processed further by the parser and interpreter in later phases of the project.

This module is designed as an independent component, focusing solely on reading a source code file or string and converting it into a list of tokens, skipping over whitespace and comments, and identifying keywords, operators, literals, and symbols.

## Folder Structure

```
lexer/
├── inc/                # Header files for the lexer
│   ├── ErrorReporter.hpp
│   ├── Scanner.hpp
│   ├── Token.hpp
│   └── TokenType.hpp
├── src/                # Source files for the lexer
│   ├── main.cpp        # Main program to test the lexer
│   ├── Scanner.cpp     # Lexer implementation

```

- **`lexer/inc/`**: Contains the header files for the lexer, including error reporting, token definitions, and scanner interface.
- **`lexer/src/`**: Contains the C++ source files. `main.cpp` is the entry point for testing the lexer, and `Scanner.cpp` implements the lexer logic.
- **`CMakeLists.txt`**: CMake configuration file to build the lexer module.

## Lexer Overview

The lexer is responsible for tokenizing the source code into a sequence of tokens. The `Scanner` class performs the scanning and categorizes the input into recognizable **tokens**.

### Key Components

1. **`ErrorReporter.hpp`**:
   - Manages and reports errors during the lexing process, such as invalid characters or unexpected tokens.

2. **`Scanner.hpp`**:
   - Declares the `Scanner` class, which is responsible for scanning the input source code and generating a list of tokens.

3. **`Token.hpp`**:
   - Defines the `Token` class, which stores information about each token, including its type, lexeme, and line number.

4. **`TokenType.hpp`**:
   - Defines the different **token types** in the litecode language, such as keywords (`if`, `while`), operators (`+`, `-`, `*`), and symbols (`(`, `)`, `{`, `}`).

5. **`main.cpp`**:
   - Provides a simple command-line interface to test the lexer by reading a file or an input string and printing the resulting tokens.

6. **`Scanner.cpp`**:
   - Implements the logic for tokenizing the source code. The scanner handles various aspects, including:
     - Skipping over whitespace and comments.
     - Identifying keywords, numbers, strings, operators, and symbols.
     - Reporting unexpected characters or errors.

## How It Works

The lexer works by reading the source code one character at a time, categorizing each sequence of characters into a **token**, and returning a list of tokens.

- **Whitespace and comments**: The scanner skips over whitespace and comments.
- **Identifiers and keywords**: Identifiers (such as variable names) and keywords (like `if`, `while`, etc.) are identified.
- **Literals**: Numeric and string literals are extracted and classified into `NUMBER` and `STRING` tokens.
- **Symbols**: The lexer identifies symbols like parentheses, braces, and operators.

### Tokenization Process

The `Scanner` class is the core of the lexer. It processes the input source code and generates tokens using the following logic:
- It iterates through the characters in the input.
- It identifies and skips over whitespace and comments.
- It generates tokens for recognized characters, such as `+`, `-`, or `*`.
- It handles string and numeric literals by reading until the end of the literal.
- It reports any invalid or unexpected characters encountered in the source code.

### Error Reporting

If an invalid character is encountered, the lexer reports an error with the line number and the unexpected character. This allows for quick identification of mistakes in the source code during the lexing phase.

## Building and Testing the Lexer

The lexer module is designed to be built and tested independently. To build and run the lexer, follow these steps:

### Prerequisites

- C++17 or higher.

### Sample Run

g++ --std=c++17 -Ilexer/inc -Ilexer/src lexer/src/main.cpp lexer/src/Scanner.cpp -o toylang
./toylang

> var x = 10
[VAR] Lexeme: "var" Literal: "" Line: 1
[IDENTIFIER] Lexeme: " x" Literal: "" Line: 1
[EQUAL] Lexeme: " =" Literal: "" Line: 1
[NUMBER] Lexeme: " 10" Literal: " 10" Line: 1
[END_OF_FILE] Lexeme: "" Literal: "" Line: 1

>
