#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "TokenType.hpp"
#include <iostream>
#include <string>
#include <algorithm>
namespace lexer {
    /**
     * @file Token.hpp
     * @brief Defines the Token value object produced by the scanner.
     *
     * A token is a small bundle of lexical metadata:
     * - what category it belongs to (`TokenType`),
     * - what raw text was consumed (`lexeme`),
     * - optional normalized literal payload (`literal`),
     * - where it occurred (`line`).
     */

    /**
     * @brief Immutable lexical token used throughout the frontend pipeline.
     *
     * Tokens are created by the scanner and then consumed by the parser.
     * By keeping token fields immutable, later compiler phases can trust that
     * lexical information remains stable for diagnostics.
     */
    class Token {
        public:
            /**
             * @brief Constructs a token with lexical metadata.
             * @param type Token category.
             * @param lexeme Exact source slice consumed by scanner.
             * @param literal Optional normalized literal payload.
             * @param line 1-based source line number.
             */
            Token(TokenType type, const std::string& lexeme, const std::string& literal, int line)
            : type(type), lexeme(lexeme), literal(literal), line(line) {}

            /**
             * @brief Gets the token category.
             * @return Token kind enum.
             */
            TokenType getType() const { return type; }
            /**
             * @brief Gets the source line where token was recognized.
             * @return 1-based line number.
             */
            int getLine() const { return line; }
            /**
             * @brief Gets raw source text for this token.
             * @return Source lexeme.
             */
            const std::string& getLexeme() const { return lexeme; }
            /**
             * @brief Gets normalized literal content if applicable.
             * @return Literal payload string.
             */
            const std::string& getLiteral() const { return literal; }

            /**
             * @brief Pretty-printer used in debug token dumps.
             * @param os Output stream.
             * @param token Token to format.
             * @return Output stream reference.
             */
            friend std::ostream& operator<<(std::ostream& os, const Token& token) {
                os << "[" << toString(token.type) << "] "
                << "Lexeme: \"" << trim(token.lexeme) << "\" "
                << "Literal: \"" << token.literal << "\" "
                << "Line: " << token.line;
                return os;
            }

        private:
            const TokenType type;
            const std::string lexeme;
            const std::string literal;
            const int line;

            /**
             * @brief Trims leading/trailing spaces from debug-printed lexeme text.
             * @param str Input string.
             * @return Trimmed view copied into std::string.
             *
             * Note for beginners:
             * this is a display helper only; it does not mutate stored token data.
             */
            static std::string trim(const std::string& str) {
                auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
                auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
                return (start < end) ? std::string(start, end) : "";
            }
    };
}  // namespace lexer

#endif
