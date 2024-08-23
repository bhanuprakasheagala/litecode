#include "TypesAndData/TokenType.cpp"
#include "TypesAndData/Token.cpp"

namespace toylang {
    class Scanner {
        public:
            Scanner(const std::string& source) : source(source), start(0), current(0), line(1) {}

            std::vector<Token> scanTokens() {
                std::vector<Token> tokens;
                while(!isAtEnd()) {
                    start = current;
                    Token token = scanToken();
                    if(token.type != TokenType::EOF) {
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

            char peek() const{
                return source[current];
            }

            char peekNext() const {
                if(isAtEnd())
                    return '\0';
                return source[current + 1];
            }

            bool match(char expected) {
                if(isAtEnd())
                    return false;
                if(source[current] != expected)
                    return false;
                ++current;

                return true;
            }

            bool isAtEnd() const{
                return current >= source.length();
            }

            void skipWhitespace() {
                while(!isAtEnd()) {
                    char c = peek();
                    switch(c) {
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
                            if(peekNext() == '/') {
                                while(peek() != '\n' && !isAtEnd())
                                    advance();
                            }
                            else {
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
                switch(c) {
                    case '(' : return makeToken(TokenType::LEFT_PARENTH);
                    case ')' : return makeToken(TokenType::RIGHT_PARENTH);
                    case '{' : return makeToken(TokenType::LEFT_BRACE);
                    case '}' : return makeToken(TokenType::RIGHT_BRACE);
                    
                }
            }

    };
}