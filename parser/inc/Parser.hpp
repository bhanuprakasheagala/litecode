#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <memory>
#include <initializer_list>
#include "Token.hpp"
#include "Expr.hpp"
#include "Stmt.hpp"

namespace parser {

    /**
     * @file Parser.hpp
     * @brief Declares the recursive-descent parser for Litecode tokens.
     */

    /**
     * @brief Converts scanner tokens into statement/expression AST nodes.
     *
     * This parser follows precedence-based recursive descent.
     * Each method corresponds to a grammar production.
     */
    class Parser {
    public:
        /**
         * @brief Creates parser over an immutable token stream.
         * @param tokens Token vector produced by scanner.
         */
        explicit Parser(const std::vector<lexer::Token>& tokens);

        /**
         * @brief Entry point that parses a whole program.
         * @return List of top-level statements/declarations.
         */
        std::vector<StmtPtr> parse();
        /**
         * @brief Indicates whether any parse error occurred.
         * @return True if parser reported an error.
         */
        bool hadError() const { return hadParseError; }

    private:
        /// @name Parser state
        /// @{
        const std::vector<lexer::Token>& tokens;
        size_t current;
        bool hadParseError;
        /// @}

        /// @name Statement/declaration grammar rules
        /// @{
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
        /// @}

        /// @name Expression precedence rules
        /// @{
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
        /// @}

        /// @name Token navigation helpers
        /// @{
        bool match(std::initializer_list<lexer::TokenType> types);
        bool check(lexer::TokenType type) const;
        const lexer::Token& advance();
        bool isAtEnd() const;
        const lexer::Token& peek() const;
        const lexer::Token& previous() const;
        const lexer::Token& consume(lexer::TokenType type, const std::string& message);
        /// @}

        /// @name Error handling and recovery
        /// @{
        void synchronize();
        /**
         * @brief Internal parse error marker type used for unwinding.
         */
        struct ParseError {};
        ParseError error(const lexer::Token& token, const std::string& message);
        /// @}
    };

} // namespace parser

#endif // PARSER_HPP
