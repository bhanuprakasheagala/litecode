#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <memory>
#include <initializer_list>
#include "Token.hpp"
#include "Expr.hpp"
#include "Stmt.hpp"

namespace parser {

    class Parser {
    public:
        explicit Parser(const std::vector<lexer::Token>& tokens);

        // Entry point
        std::vector<StmtPtr> parse();
        bool hadError() const { return hadParseError; }

    private:
        // ===== State =====
        const std::vector<lexer::Token>& tokens;
        size_t current;
        bool hadParseError;

        // ===== Grammar rules =====
        StmtPtr declaration();
        StmtPtr classDeclaration();
        StmtPtr function(const std::string& kind);
        StmtPtr varDeclaration();
        StmtPtr statement();
        StmtPtr forStatement();
        StmtPtr ifStatement();
        StmtPtr whileStatement();
        StmtPtr returnStatement();
        StmtPtr printStatement();
        StmtPtr expressionStatement();
        std::vector<StmtPtr> block();

        ExprPtr expression();
        ExprPtr assignment();
        ExprPtr logicalOr();
        ExprPtr logicalAnd();
        ExprPtr equality();
        ExprPtr comparison();
        ExprPtr term();
        ExprPtr factor();
        ExprPtr unary();
        ExprPtr call();
        ExprPtr finishCall(ExprPtr callee);
        ExprPtr primary();

        // ===== Helpers =====
        bool match(std::initializer_list<lexer::TokenType> types);
        bool check(lexer::TokenType type) const;
        const lexer::Token& advance();
        bool isAtEnd() const;
        const lexer::Token& peek() const;
        const lexer::Token& previous() const;
        const lexer::Token& consume(lexer::TokenType type, const std::string& message);

        // Error handling
        void synchronize();
        struct ParseError {};
        ParseError error(const lexer::Token& token, const std::string& message);
    };

} // namespace parser

#endif // PARSER_HPP
