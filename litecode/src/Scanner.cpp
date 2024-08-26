#ifndef SCANNER_H
#define SCANNER_H
#include <unordered_map>

namespace toylang {
    class Scanner {
        public:
            Scanner(const std::string& source) : source(source), start(0), current(0), line(1) {}

            std::vector<Token> scanTokens() {
                std::vector<Token> tokens;
                while(!isAtEnd()) {
                    start = current;
                    Token token = scanToken();
                    if(token.getType() != TokenType::END_OF_FILE) {
                        tokens.push_back(token);
                    }
                }
                tokens.push_back(Token(TokenType::END_OF_FILE, "", "", line));

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
                    case ',' : return makeToken(TokenType::COMMA);
                    case '.' : return makeToken(TokenType::DOT);
                    case '-' : return makeToken(TokenType::MINUS);
                    case '+' : return makeToken(TokenType::PLUS);
                    case ';' : return makeToken(TokenType::SEMICOLON);
                    case '*' : return makeToken(TokenType::STAR);
                    case '!' : return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
                    case '=' : return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
                    case '<' : return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
                    case '>' : return makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
                    case '/' : 
                        if(match('/')) {
                            while(peek() != '\n' && !isAtEnd())
                                advance();
                            return scanToken();
                        }
                        return makeToken(TokenType::SLASH);
                    case '"' : return string();
                    default  :
                        if(std::isdigit(c))
                            return number();
                        if(std::isalpha(c) || c == '_')
                            return identifier();
                        std::cerr << "Unexpected character: " << c << '\n';
                        return makeToken(TokenType::END_OF_FILE);
                }
            }

            Token number() {
                while(std::isdigit(peek()))
                    advance();
                return makeToken(TokenType::NUMBER, source.substr(start, current - start));
            }

            Token string() {
                while(peek() != '"' && !isAtEnd()) {
                    if(peek() == '\n')
                        line++;
                    advance();
                }
                if(isAtEnd()) {
                    std::cerr << "Unexpected string!\n";
                    return makeToken(TokenType::END_OF_FILE);
                }
                advance();
                return makeToken(TokenType::STRING, source.substr(start+1, current - start - 2));
            }

            Token identifier() {
                while(std::isalnum(peek()) || peek() == '_')
                    advance();
                std::string text = source.substr(start, current - start);
                TokenType type = Keywords.count(text) ? Keywords.at(text) : TokenType::IDENTIFIER;

                return makeToken(type);
            }

            Token makeToken(TokenType type, const std::string& literal = "") {
                return Token(type, source.substr(start, current - start), literal, line);
            }

            static const std::unordered_map<std::string, TokenType> Keywords;
    };

    const std::unordered_map<std::string, TokenType> Scanner::Keywords = {
        {"and", TokenType::AND}, {"class", TokenType::CLASS}, {"else", TokenType::ELSE},
        {"false", TokenType::FALSE}, {"for", TokenType::FOR}, {"fun", TokenType::FUN},
        {"if", TokenType::IF}, {"nil", TokenType::NILL}, {"or", TokenType::OR},
        {"print", TokenType::PRINT}, {"super", TokenType::SUPER}, {"return", TokenType::RETURN},
        {"this", TokenType::THIS}, {"var", TokenType::VAR}, {"true", TokenType::TRUE},
        {"while", TokenType::WHILE}
    };
}

#endif