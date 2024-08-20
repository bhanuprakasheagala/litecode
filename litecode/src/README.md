## Lexical analyzer in C++ for Lox toy language

### Overview

The Lox language is a small language with various types of tokens, including keywords, operators, and literals. Our lexer will need to handle:

1. **Keywords**: `and`, `class`, `else`, `false`, `for`, `fun`, `if`, `nil`, `or`, `print`, `return`, `super`, `this`, `true`, `var`, `while`.
2. **Operators**: `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `>`, `>=`, `<`, `<=`, `!`, `&&`, `||`.
3. **Punctuation**: `(`, `)`, `{`, `}`, `[`, `]`, `,`, `.`.
4. **Literals**: Numbers and strings.
5. **Identifiers**: Names for variables and functions.
6. **Whitespace and Comments**: Should be ignored.

### Token Types

We'll define different token types using an enumeration:

```cpp
enum class TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    AND, CLASS, ELSE, FALSE, FOR, FUN, IF, NIL, OR, PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    EOF
};
```

### Token Structure

Each token will have a type and, in some cases, a lexeme (the text that matches the token). We can define a `Token` class:

```cpp
#include <string>

class Token {
public:
    Token(TokenType type, std::string lexeme, std::string literal, int line)
        : type(type), lexeme(lexeme), literal(literal), line(line) {}

    TokenType type;
    std::string lexeme;
    std::string literal;
    int line;
};
```

### Lexical Analyzer Class

Now, let's write the `Lexer` class to handle tokenization:

```cpp
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class Lexer {
public:
    Lexer(const std::string& source) : source(source), start(0), current(0), line(1) {}

    std::vector<Token> scanTokens() {
        std::vector<Token> tokens;
        while (!isAtEnd()) {
            start = current;
            Token token = scanToken();
            if (token.type != TokenType::EOF) {
                tokens.push_back(token);
            }
        }
        tokens.push_back(Token(TokenType::EOF, "", "", line));
        return tokens;
    }

private:
    std::string source;
    size_t start, current;
    int line;

    char advance() {
        return source[current++];
    }

    char peek() const {
        return source[current];
    }

    char peekNext() const {
        if (isAtEnd()) return '\0';
        return source[current + 1];
    }

    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        ++current;
        return true;
    }

    bool isAtEnd() const {
        return current >= source.length();
    }

    void skipWhitespace() {
        while (!isAtEnd()) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    advance();
                    break;
                case '\n':
                    ++line;
                    advance();
                    break;
                case '/':
                    if (peekNext() == '/') {
                        while (peek() != '\n' && !isAtEnd()) advance();
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
        }
    }

    Token scanToken() {
        char c = advance();
        switch (c) {
            case '(': return makeToken(TokenType::LEFT_PAREN);
            case ')': return makeToken(TokenType::RIGHT_PAREN);
            case '{': return makeToken(TokenType::LEFT_BRACE);
            case '}': return makeToken(TokenType::RIGHT_BRACE);
            case ',': return makeToken(TokenType::COMMA);
            case '.': return makeToken(TokenType::DOT);
            case '-': return makeToken(TokenType::MINUS);
            case '+': return makeToken(TokenType::PLUS);
            case ';': return makeToken(TokenType::SEMICOLON);
            case '*': return makeToken(TokenType::STAR);
            case '!': return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
            case '=': return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            case '<': return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            case '>': return makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            case '/': // Handle comments separately
                if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                    return scanToken();
                }
                return makeToken(TokenType::SLASH);
            case '"': return string();
            default:
                if (std::isdigit(c)) return number();
                if (std::isalpha(c) || c == '_') return identifier();
                std::cerr << "Unexpected character: " << c << std::endl;
                return makeToken(TokenType::EOF); // Error case
        }
    }

    Token number() {
        while (std::isdigit(peek())) advance();
        return makeToken(TokenType::NUMBER, source.substr(start, current - start));
    }

    Token string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') ++line;
            advance();
        }
        if (isAtEnd()) {
            std::cerr << "Unterminated string." << std::endl;
            return makeToken(TokenType::EOF); // Error case
        }
        advance(); // Closing "
        return makeToken(TokenType::STRING, source.substr(start + 1, current - start - 2));
    }

    Token identifier() {
        while (std::isalnum(peek()) || peek() == '_') advance();
        std::string text = source.substr(start, current - start);
        TokenType type = keywords.count(text) ? keywords.at(text) : TokenType::IDENTIFIER;
        return makeToken(type);
    }

    Token makeToken(TokenType type, const std::string& literal = "") {
        return Token(type, source.substr(start, current - start), literal, line);
    }

    static const std::unordered_map<std::string, TokenType> keywords;
};

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"and", TokenType::AND}, {"class", TokenType::CLASS}, {"else", TokenType::ELSE},
    {"false", TokenType::FALSE}, {"for", TokenType::FOR}, {"fun", TokenType::FUN},
    {"if", TokenType::IF}, {"nil", TokenType::NIL}, {"or", TokenType::OR},
    {"print", TokenType::PRINT}, {"return", TokenType::RETURN}, {"super", TokenType::SUPER},
    {"this", TokenType::THIS}, {"true", TokenType::TRUE}, {"var", TokenType::VAR},
    {"while", TokenType::WHILE}
};
```

### Key Points

- **Skipping Whitespace and Comments**: The `skipWhitespace` function handles whitespace and comments. It allows us to ignore unnecessary characters.
- **Tokenization**: Each token type is processed in the `scanToken` function. We handle single-character tokens, operators, and complex tokens like numbers and strings.
- **Error Handling**: Basic error handling is implemented for unexpected characters and unterminated strings.

### Running the Lexer

To test the lexer, you can write a small main function:

```cpp
int main() {
    std::string source = R"(var x = 5 + 3;
                            if (x > 10) {
                                print "x is large!";
                            } else {
                                print "x is small.";
                            })";

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();

    for (const Token& token : tokens) {
        std::cout << "Type: " << static_cast<int>(token.type)
                  << ", Lexeme: " << token.lexeme
                  << ", Literal: " << token.literal
                  << ", Line: " << token.line << std::endl;
    }

    return 0;
}
```
