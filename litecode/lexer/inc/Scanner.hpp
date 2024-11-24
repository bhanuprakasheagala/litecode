#ifndef SCANNER_HPP
#define SCANNER_HPP

#include "Token.hpp"
#include "ErrorReporter.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace lexer{
    class Scanner {
    public:
        explicit Scanner(const std::string& source);
        std::vector<Token> scanTokens(); // Scans and returns all tokens from the source code
    
    private:
        // The source string being scanned
        std::string source;
        
        // Current scanning state
        size_t start, current;
        int line;

        // Helpers for scanning tokens
        char advance();         //   Advances and returns the next character
        char peek() const;      //   Peeks at the current character without consuming it
        char peekNext() const;  //   Peeks at the next character without consuming it

        bool isAtEnd() const;       // Checks if we have reached the end of the source
        bool match(char expected);   // Matches the current character with the expected one

        void skipWhitespace();      // Skips whitespace and comment

        // Token creating and recognition
        Token scanToken();
        Token makeToken(TokenType type, const std::string& literal = "");
        Token stringLiteral();
        Token numberLiteral();
        Token identifier();

        // Keywords map
        static const std::unordered_map<std::string, TokenType> Keywords;
    };
}

#endif