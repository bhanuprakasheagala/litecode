#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
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

        // Current scanning state
        size_t start, current;
        int line;

        // Core helpers
        char advance();
        char peek() const;
        char peekNext() const;
        bool match(char expected);
        bool isAtEnd() const;
        void skipWhitespace();

        // Scans a single token, returns nullopt on error or comment
        std::optional<Token> scanToken();
        std::optional<Token> number();
        std::optional<Token> string();
        std::optional<Token> identifier();

        // Constructs a token
        Token makeToken(TokenType type, const std::string& literal = "");

        // Keywords map
        static const std::unordered_map<std::string, TokenType> Keywords;
    };

} // namespace lexer

#endif // SCANNER_HPP