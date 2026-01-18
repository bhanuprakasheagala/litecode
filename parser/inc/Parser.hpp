#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <memory>
#include <initializer_list>
#include "Token.hpp"
#include "Expr.hpp"

namespace parser {

    class Parser {
    public:
        explicit Parser(const std::vector<lexer::Token>& tokens);

        // Entry point
        ExprPtr parse();

    private:
        // ===== State =====
        const std::vector<lexer::Token>& tokens;
        size_t current;

        // ===== Grammar rules =====
        ExprPtr expression();
        ExprPtr equality();
        ExprPtr comparison();
        ExprPtr term();
        ExprPtr factor();
        ExprPtr unary();
        ExprPtr primary();

        // ===== Helpers =====
        bool match(std::initializer_list<lexer::TokenType> types);
        bool check(lexer::TokenType type) const;
        const lexer::Token& advance();
        bool isAtEnd() const;
        const lexer::Token& peek() const;
        const lexer::Token& previous() const;

        // const lexer::Token& consume(lexer::TokenType type, const char* message);

        // Error handling
        void synchronize();
        void error(const lexer::Token& token, const std::string& message) const;
    };

} // namespace parser

#endif // PARSER_HPP
