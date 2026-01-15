#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "TokenType.hpp"
#include <iostream>
#include <string>
#include <algorithm>
namespace lexer {  
    class Token {
        public:
            Token(TokenType type, const std::string& lexeme, const std::string& literal, int line)
            : type(type), lexeme(lexeme), literal(literal), line(line) {}

            TokenType getType() const { return type; }
            int getLine() const { return line; }
            const std::string& getLexeme() const { return lexeme; }
            const std::string& getLiteral() const { return literal; }

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

            // Helper function to trim leading and trailing whitespace
            static std::string trim(const std::string& str) {
                auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
                auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
                return (start < end) ? std::string(start, end) : "";
            }
    };
}

#endif