# litecode lexer

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

## UML diagrams for Lexer

### Class Diagrams
```plaintext
+-------------------+
|    litecode       |
+-------------------+
| - argc: int       |
| - argv: char**    |
| - hadError: bool  |
+-------------------+
| + litecode(argc, argv) |
| + ~litecode()          |
| + start(): void        |
| - runFile(path: string): void |
| - runPrompt(): void    |
| - run(inputSource: string): void |
| - error(line: int, message: string): void |
| - report(line: int, where: string, message: string): void |
+-------------------+

+-------------------+
|    Scanner        |
+-------------------+
| - source: string  |
| - tokens: vector<Token> |
| - start: int      |
| - current: int    |
| - line: int       |
+-------------------+
| + Scanner(source: string) |
| + scanTokens(): vector<Token> |
| - scanToken(): void        |
| - skipWhitespace(): void   |
| - number(): Token          |
| - string(): Token          |
| - identifier(): Token      |
| - match(expected: char): bool |
| - advance(): char          |
| - peek(): char             |
| - peekNext(): char         |
| - isAtEnd(): bool          |
+-------------------+

+-------------------+
|      Token        |
+-------------------+
| - type: TokenType |
| - lexeme: string  |
| - literal: string |
| - line: int       |
+-------------------+
| + Token(type: TokenType, lexeme: string, literal: string, line: int) |
| + getType(): TokenType |
| + toString(): string   |
+-------------------+

+-------------------+
|  ErrorReporter    |
+-------------------+
| - hadError: bool  |
+-------------------+
| + report(line: int, where: string, message: string): void |
| + hadError(): bool |
| + reset(): void    |
+-------------------+
```

### Sequence diagram (File mode)
```plaintext
main() -> litecode::start()
    litecode::start() -> litecode::runFile(path)
        litecode::runFile() -> litecode::run(content)
            litecode::run() -> Scanner::scanTokens()
                Scanner::scanTokens() -> Token Stream
            litecode::run() -> Print Tokens
```

### Sequence diagram (Interactive mode)
```plaintext
main() -> litecode::start()
    litecode::start() -> litecode::runPrompt()
        litecode::runPrompt() -> litecode::run(inputLine)
            litecode::run() -> Scanner::scanTokens()
                Scanner::scanTokens() -> Token Stream
            litecode::run() -> Print Tokens
```

### Activity Diagram
```plaintext
+-------------------+
|       Start       |
+-------------------+
         |
         v
+-------------------+
|  main()           |
+-------------------+
         |
         v
+-------------------+
| litecode::start() |
+-------------------+
         |
         +-----------------------------+
         |                             |
+-------------------+         +-------------------+
| runFile(path)     |         | runPrompt()       |
+-------------------+         +-------------------+
         |                             |
         v                             v
+-------------------+         +-------------------+
| run(content)      |         | run(inputLine)    |
+-------------------+         +-------------------+
         |                             |
         v                             v
+-------------------+         +-------------------+
| scanTokens()      |         | scanTokens()      |
+-------------------+         +-------------------+
         |                             |
         v                             v
+-------------------+         +-------------------+
| Print Tokens      |         | Print Tokens      |
+-------------------+         +-------------------+
         |
         v
+-------------------+
|       End         |
+-------------------+
```

### Component diagram
```plaintext
+-------------------+
|     main.cpp      |
+-------------------+
         |
         v
+-------------------+
| litecode Class    |
+-------------------+
         |
         v
+-------------------+
| Scanner Class     |
+-------------------+
         |
         v
+-------------------+
| Token Class       |
+-------------------+
         |
         v
+-------------------+
| ErrorReporter     |
+-------------------+
```

---

### **Step-by-Step Explanation**

#### **1. `main()`**
- **Input:** Command-line arguments (`argc`, `argv`).
  - Example: `./litecode script.lc` or litecode (no arguments for REPL).
- **Process:**
  - Creates an instance of the litecode class.
  - Calls `litecode::start()` to begin execution.
- **Output:** None (delegates control to `start()`).

---

#### **2. `litecode::start()`**
- **Input:** Command-line arguments (`argc`, `argv`).
  - Example 1: `argc = 2, argv[1] = "script.lc"` (file mode).
  - Example 2: `argc = 1` (interactive mode).
- **Process:**
  - If `argc > 2`, prints usage instructions and exits.
  - If `argc == 2`, calls `runFile()` with the file path.
  - If `argc == 1`, calls `runPrompt()` for interactive mode.
- **Output:** None (delegates control to `runFile()` or `runPrompt()`).

---

#### **3. `litecode::runFile(path)`**
- **Input:** File path (`path`).
  - Example: `"script.lc"`.
- **Process:**
  - Opens the file and reads its content into a string.
  - Calls `run(content)` with the file content.
- **Output:** None (delegates control to `run()`).

---

#### **4. `litecode::runPrompt()`**
- **Input:** User input from the console (`inputLine`).
  - Example: `"> print 42;"`.
- **Process:**
  - Continuously prompts the user for input.
  - Calls `run(inputLine)` for each non-empty line.
  - Resets the `hadError` flag after each line.
- **Output:** None (delegates control to `run()`).

---

#### **5. `litecode::run(inputSource)`**
- **Input:** Source code as a string (`inputSource`).
  - Example: `"print 42;"` (from file or user input).
- **Process:**
  - Creates a `Scanner` instance with the source code.
  - Calls `Scanner::scanTokens()` to tokenize the input.
  - Iterates through the tokens and prints them.
- **Output:** Prints tokens to the console.

---

#### **6. `Scanner::scanTokens()`**
- **Input:** Source code as a string (`source`).
  - Example: `"print 42;"`.
- **Process:**
  - Tokenizes the source code into a list of `Token` objects.
  - Adds an `END_OF_FILE` token at the end.
- **Output:** A vector of `Token` objects.
  - Example: `[Token(PRINT, "print"), Token(NUMBER, "42"), Token(SEMICOLON, ";"), Token(END_OF_FILE, "")]`.

---

#### **7. Token Stream**
- **Input:** Vector of `Token` objects.
  - Example: `[Token(PRINT, "print"), Token(NUMBER, "42"), Token(SEMICOLON, ";"), Token(END_OF_FILE, "")]`.
- **Process:**
  - Iterates through the tokens and prints them to the console.
- **Output:** Printed tokens.
  - Example:
    ```plaintext
    PRINT print
    NUMBER 42
    SEMICOLON ;
    END_OF_FILE 
    ```

---

### **Example Execution**

#### **File Mode**
1. Command: `./litecode script.lc`
2. File `script.lc` contains:
   ```plaintext
   print 42;
   ```
3. Execution Flow:
   - `main()` → `start()` → `runFile("script.lc")` → `run("print 42;")` → `scanTokens()` → Print tokens.

#### **Interactive Mode**
1. Command: litecode
2. User Input:
   ```plaintext
   > print 42;
   ```
3. Execution Flow:
   - `main()` → `start()` → `runPrompt()` → `run("print 42;")` → `scanTokens()` → Print tokens.

## Building and Testing the Lexer

The lexer module is designed to be built and tested independently. To build and run the lexer, follow these steps:

### Prerequisites

- C++17 or higher.

### Sample Run

cd litecode/lexer <br/>
g++ --std=c++17 -I lexer/inc lexer/src/main.cpp lexer/src/Scanner.cpp -o toylang <br/>

#### Interactive mode
$ ./toylang <br/>

**Input**: <br/>

```
> var x = 10
```

**Output**: <br/>
```
[VAR] Lexeme: "var" Literal: "" Line: 1
[IDENTIFIER] Lexeme: "x" Literal: "" Line: 1
[EQUAL] Lexeme: "=" Literal: "" Line: 1
[NUMBER] Lexeme: "10" Literal: "10" Line: 1
[END_OF_FILE] Lexeme: "" Literal: "" Line: 1
>
```

<br/>

#### File mode
./toylang    <path/to/the/file> <br/>

**Input**: <br/>

```
$ ./toylang lexer/tests/LoxSample1.lox
```

**Output**: <br/>
```
[VAR] Lexeme: "var" Literal: "" Line: 1
[IDENTIFIER] Lexeme: "a" Literal: "" Line: 1
[EQUAL] Lexeme: "=" Literal: "" Line: 1
[NUMBER] Lexeme: "1" Literal: "1" Line: 1
[SEMICOLON] Lexeme: ";" Literal: "" Line: 1
[IDENTIFIER] Lexeme: "while" Literal: "" Line: 2
[LEFT_PARENTH] Lexeme: "(" Literal: "" Line: 2
[IDENTIFIER] Lexeme: "a" Literal: "" Line: 2
[LESS] Lexeme: "<" Literal: "" Line: 2
[NUMBER] Lexeme: "10" Literal: "10" Line: 2
[RIGHT_PARENTH] Lexeme: ")" Literal: "" Line: 2
[LEFT_BRACE] Lexeme: "{" Literal: "" Line: 2
[IDENTIFIER] Lexeme: "print" Literal: "" Line: 3
[IDENTIFIER] Lexeme: "a" Literal: "" Line: 3
[SEMICOLON] Lexeme: ";" Literal: "" Line: 3
[IDENTIFIER] Lexeme: "a" Literal: "" Line: 4
[EQUAL] Lexeme: "=" Literal: "" Line: 4
[IDENTIFIER] Lexeme: "a" Literal: "" Line: 4
[PLUS] Lexeme: "+" Literal: "" Line: 4
[NUMBER] Lexeme: "1" Literal: "1" Line: 4
[SEMICOLON] Lexeme: ";" Literal: "" Line: 4
[RIGHT_BRACE] Lexeme: "}" Literal: "" Line: 5
[END_OF_FILE] Lexeme: "" Literal: "" Line: 5
```


