#include "Parser.hpp"
#include <iostream>

namespace parser {

    Parser::Parser(const std::vector<lexer::Token>& tokens)
    : tokens(tokens), current(0) {}

    ExprPtr Parser::expression() {
        return equality();
    }
    
    ExprPtr Parser::parse() {
        return expression();
    }

    bool Parser::match(std::initializer_list<lexer::TokenType> types) {
        for (auto type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    bool Parser::check(lexer::TokenType type) const {
        if (isAtEnd()) return false;
        return peek().getType() == type;
    }

    const lexer::Token& Parser::advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool Parser::isAtEnd() const {
        return peek().getType() == lexer::TokenType::END_OF_FILE;
    }

    const lexer::Token& Parser::peek() const {
        return tokens[current];
    }

    const lexer::Token& Parser::previous() const {
        return tokens[current - 1];
    }

    void Parser::error(const lexer::Token& token, const std::string& message) const {
        std::cerr << "[Line " << /* token.line */ "] Error at '"
                << /* token.lexeme */ "': "
                << message << '\n';
    }


    ExprPtr Parser::equality() {
        ExprPtr expr = comparison();

        while (match({lexer::TokenType::BANG_EQUAL,
                    lexer::TokenType::EQUAL_EQUAL})) {
            lexer::Token op = previous();
            ExprPtr right = comparison();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    ExprPtr Parser::comparison() {
        ExprPtr expr = term();

        while (match({lexer::TokenType::GREATER,
                    lexer::TokenType::GREATER_EQUAL,
                    lexer::TokenType::LESS,
                    lexer::TokenType::LESS_EQUAL})) {
            lexer::Token op = previous();
            ExprPtr right = term();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    ExprPtr Parser::term() {
        ExprPtr expr = factor();

        while (match({lexer::TokenType::MINUS,
                    lexer::TokenType::PLUS})) {
            lexer::Token op = previous();
            ExprPtr right = factor();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    ExprPtr Parser::factor() {
        ExprPtr expr = unary();

        while (match({lexer::TokenType::SLASH,
                    lexer::TokenType::STAR})) {
            lexer::Token op = previous();
            ExprPtr right = unary();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    ExprPtr Parser::unary() {
        if (match({lexer::TokenType::BANG,
                lexer::TokenType::MINUS})) {
            lexer::Token op = previous();
            ExprPtr right = unary();
            return std::make_unique<Unary>(op, std::move(right));
        }

        return primary();
    }

    ExprPtr Parser::primary() {
        if (match({lexer::TokenType::FALSE})) return std::make_unique<Literal>(false);
        if (match({lexer::TokenType::TRUE}))  return std::make_unique<Literal>(true);
        if (match({lexer::TokenType::NILL}))  return std::make_unique<Literal>(nullptr);

        if (match({lexer::TokenType::NUMBER})) {
            return std::make_unique<Literal>(std::stod(previous().getLiteral()));
        }

        if (match({lexer::TokenType::STRING})) {
            return std::make_unique<Literal>(previous().getLiteral());
        }

        if (match({lexer::TokenType::IDENTIFIER})) {
            return std::make_unique<Variable>(previous());
        }

        if (match({lexer::TokenType::LEFT_PARENTH})) {
            ExprPtr expr = expression();
            // consume RIGHT_PARENTH (you’ll add consume() later)
            advance();
            return std::make_unique<Grouping>(std::move(expr));
        }

        error(peek(), "Expected expression.");
        return nullptr;
    }
}