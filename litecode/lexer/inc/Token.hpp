#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "TokenType.hpp"
#include <iostream>
#include <string>
namespace lexer {  
    class Token {
        public:
            Token(TokenType type, const std::string& lexeme, const std::string& literal, int line)
            : type(type), lexeme(lexeme), literal(literal), line(line) {}

            TokenType getType() const { return type; }

            friend std::ostream& operator<<(std::ostream& os, const Token& token) {
                os << "[" << toString(token.type) << "] "
                << "Lexeme: \"" << token.lexeme << "\" "
                << "Literal: \"" << token.literal << "\" "
                << "Line: " << token.line;
                return os;
            }

        private:
            const TokenType type;
            const std::string lexeme;
            const std::string literal;
            const int line;
    };
}

#endif