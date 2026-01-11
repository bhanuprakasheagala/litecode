#include "Scanner.hpp"
#include <iostream>
#include <cctype>
namespace lexer {

    Scanner::Scanner(const std::string& source)
        : source(source), start(0), current(0), line(1) {}

    std::vector<Token> Scanner::scanTokens() {
        std::vector<Token> tokens;

        while (!isAtEnd()) {
            start = current;
            auto token = scanToken();
            if (token.has_value()) {
                tokens.push_back(*token);
            }
        }

        tokens.emplace_back(TokenType::END_OF_FILE, "", "", line);
        return tokens;
    }

    char Scanner::advance() {
        return source[current++];
    }

    char Scanner::peek() const {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    char Scanner::peekNext() const {
        if (current + 1 >= source.length()) return '\0';
        return source[current + 1];
    }

    bool Scanner::match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }

    bool Scanner::isAtEnd() const {
        return current >= source.length();
    }

    void Scanner::skipWhitespace() {
        while (!isAtEnd()) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    advance();
                    break;
                case '\n':
                    line++;
                    advance();
                    break;
                case '/':
                    if (peekNext() == '/') {
                        while (peek() != '\n' && !isAtEnd()) advance();
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
        }
    }

    std::optional<Token> Scanner::scanToken() {
        skipWhitespace();
        if (isAtEnd()) return std::nullopt;

        char c = advance();

        switch (c) {
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

            case '/':
                if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                    return std::nullopt;
                }
                return makeToken(TokenType::SLASH);

            case '"':
                return string();

            default:
                if (std::isdigit(c)) return number();
                if (std::isalpha(c) || c == '_') return identifier();

                std::cerr << "[Line " << line << "] Unexpected character: " << c << '\n';
                return std::nullopt;
        }
    }
    std::optional<Token> Scanner::number() {
        size_t literalStart = current - 1; // first digit was already consumed
        while (std::isdigit(peek()))
            advance();

        std::string literal = source.substr(literalStart, current - literalStart);
        return makeToken(TokenType::NUMBER, literal);
    }
    std::optional<Token> Scanner::string() {
        size_t literalStart = current; // skip opening quote
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            std::cerr << "[Line " << line << "] Unterminated string.\n";
            return std::nullopt;
        }

        advance(); // consume closing quote
        std::string literal = source.substr(literalStart, current - literalStart - 1);
        return makeToken(TokenType::STRING, literal);
    }
    std::optional<Token> Scanner::identifier() {
        while (std::isalnum(peek()) || peek() == '_') advance();
        std::string text = source.substr(start, current - start);
        TokenType type = Keywords.count(text) ? Keywords.at(text) : TokenType::IDENTIFIER;
        return makeToken(type);
    }
    Token Scanner::makeToken(TokenType type, const std::string& literal) {
        std::string lexeme = source.substr(start, current - start); // full token text
        return Token(type, lexeme, literal, line);
    }
    const std::unordered_map<std::string, TokenType> Scanner::Keywords = {
        {"and", TokenType::AND}, {"class", TokenType::CLASS}, {"else", TokenType::ELSE},
        {"false", TokenType::FALSE}, {"for", TokenType::FOR}, {"fun", TokenType::FUN},
        {"if", TokenType::IF}, {"nil", TokenType::NILL}, {"or", TokenType::OR},
        {"print", TokenType::PRINT}, {"super", TokenType::SUPER}, {"return", TokenType::RETURN},
        {"this", TokenType::THIS}, {"var", TokenType::VAR}, {"true", TokenType::TRUE},
        {"while", TokenType::WHILE}
    };
}