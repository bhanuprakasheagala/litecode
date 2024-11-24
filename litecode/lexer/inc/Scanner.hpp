#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.hpp"
#include "TokenType.hpp"

namespace lexer {

    class Scanner {
    public:
        // Constructor
        explicit Scanner(const std::string& source);

        // Scans all tokens and returns them
        std::vector<Token> scanTokens();

    private:
        // Source code to scan
        std::string source;

        // Current state in the scanner
        size_t start, current;
        int line;

        // Helper methods
        char advance();
        char peek() const;
        char peekNext() const;
        bool match(char expected);
        bool isAtEnd() const;
        void skipWhitespace();

        // Scans a single token
        Token scanToken();
        Token number();
        Token string();
        Token identifier();
        Token makeToken(TokenType type, const std::string& literal = "");

        // Keywords map
        static const std::unordered_map<std::string, TokenType> Keywords;
    };

} // namespace lexer

#endif // SCANNER_HPP
