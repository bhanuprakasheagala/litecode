#ifndef TOKEN_H
#define TOKEN_H

namespace toylang{
    
    class Token {
        public:
            Token(TokenType type, const std::string& lexeme, const std::string& literal, int line)
            : type(type), lexeme(lexeme), literal(literal), line(line) {}

            TokenType getType() const { return type; }

            friend std::ostream& operator<<(std::ostream& os, const Token& token) {
                os << static_cast<int>(token.type) << " " << token.lexeme << " " << token.literal;
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