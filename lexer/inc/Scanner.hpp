#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "Token.hpp"
#include "TokenType.hpp"

namespace lexer {

    /**
     * @file Scanner.hpp
     * @brief Declares the lexical scanner that transforms source text into tokens.
     */

    /**
     * @brief Stateful scanner for Litecode source text.
     *
     * High-level process:
     * 1. Read raw source characters.
     * 2. Group them into lexical units (tokens).
     * 3. Skip trivia (whitespace/comments).
     * 4. Emit token stream ending with END_OF_FILE.
     *
     * This class is intentionally single-pass and left-to-right for clarity.
     */
    class Scanner {
    public:
        /**
         * @brief Creates a scanner for a single source string.
         * @param source Entire source content to scan.
         */
        explicit Scanner(const std::string& source);

        /**
         * @brief Scans the full source and returns all produced tokens.
         * @return Vector of tokens ending with END_OF_FILE.
         */
        std::vector<Token> scanTokens();

    private:
        /**
         * @brief Immutable source text currently being scanned.
         */
        std::string source;

        /**
         * @brief Cursor state for tokenization.
         * `start` marks beginning of current lexeme.
         * `current` marks next unread character.
         * `line` tracks 1-based line number for diagnostics.
         */
        size_t start, current;
        int line;

        /// @name Core character helpers
        /// @{
        char advance();
        char peek() const;
        char peekNext() const;
        bool match(char expected);
        bool isAtEnd() const;
        void skipWhitespace();
        /// @}

        /// @name Token scanners
        /// @{
        /**
         * @brief Scans one logical token from current cursor position.
         * @return Token when recognized; std::nullopt for skipped trivia/error continuation.
         */
        std::optional<Token> scanToken();
        /**
         * @brief Scans numeric literal token.
         * @return NUMBER token with normalized literal payload.
         */
        Token number();
        /**
         * @brief Scans identifier or keyword token.
         * @return IDENTIFIER or keyword token type.
         */
        Token identifier();
        /**
         * @brief Scans string literal content between double quotes.
         * @return STRING token or std::nullopt when unterminated.
         */
        std::optional<Token> string();
        /// @}

        /**
         * @brief Constructs a token from current lexeme range.
         * @param type Token category.
         * @param literal Optional literal payload.
         * @return Fully populated token.
         */
        Token makeToken(TokenType type, const std::string& literal = "");

        /**
         * @brief Reserved word lookup table (`source text` -> `TokenType`).
         */
        static const std::unordered_map<std::string, TokenType> Keywords;
    };

} // namespace lexer

#endif // SCANNER_HPP
