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
        /** @brief Parses one declaration and synchronizes after a malformed declaration. */
        StmtPtr declaration();
        /** @brief Parses a class header, optional superclass, and method declarations. */
        StmtPtr classDeclaration();
        /** @brief Parses a function or method; `kind` selects diagnostics and declaration role. */
        StmtPtr function(const std::string& kind);
        /** @brief Parses a variable binding and optional initializer, terminated by `;`. */
        StmtPtr varDeclaration();
        /** @brief Selects the statement production beginning at the current token. */
        StmtPtr statement();
        /** @brief Parses `for` and lowers its clauses into block/while AST nodes. */
        StmtPtr forStatement();
        /** @brief Parses a conditional and its optional else branch. */
        StmtPtr ifStatement();
        /** @brief Parses a condition-controlled loop and its body. */
        StmtPtr whileStatement();
        /** @brief Parses a return with optional value; semantic legality is checked by Resolver. */
        StmtPtr returnStatement();
        /** @brief Parses a print expression and its terminating semicolon. */
        StmtPtr printStatement();
        /** @brief Parses an expression in statement position and discards its eventual value. */
        StmtPtr expressionStatement();
        /** @brief Parses declarations until the matching `}`; caller consumed the opening brace. */
        std::vector<StmtPtr> block();
        /// @}

        /// @name Expression precedence rules
        /// @{
        ExprPtr expression();
        /** @brief Parses right-associative assignment, accepting variables and properties as targets. */
        ExprPtr assignment();
        /** @brief Parses left-associative `or`, represented separately for runtime short-circuiting. */
        ExprPtr logicalOr();
        /** @brief Parses left-associative `and`, represented separately for runtime short-circuiting. */
        ExprPtr logicalAnd();
        /** @brief Parses left-associative equality operators over comparison expressions. */
        ExprPtr equality();
        /** @brief Parses comparison operators over additive expressions. */
        ExprPtr comparison();
        /** @brief Parses left-associative addition and subtraction. */
        ExprPtr term();
        /** @brief Parses left-associative multiplication and division. */
        ExprPtr factor();
        /** @brief Parses prefix `!` and `-`, recursively binding them tighter than binary operators. */
        ExprPtr unary();
        /** @brief Parses a primary followed by any chain of calls or property accesses. */
        ExprPtr call();
        /** @brief Completes a call suffix and enforces the language's 255-argument limit. */
        ExprPtr finishCall(ExprPtr callee);
        /** @brief Parses literals, identifiers, grouping, `this`, and `super.method`. */
        ExprPtr primary();
        /// @}

        /// @name Token navigation helpers
        /// @{
        /** @brief Consumes the current token only if its type is in the supplied set. */
        bool match(std::initializer_list<lexer::TokenType> types);
        /** @brief Tests current token type without consuming; EOF is handled by `isAtEnd`. */
        bool check(lexer::TokenType type) const;
        /** @brief Advances unless at EOF and returns the token just consumed. */
        const lexer::Token& advance();
        /** @brief Detects the explicit EOF sentinel in the token vector. */
        bool isAtEnd() const;
        /** @brief Returns the current token without advancing. */
        const lexer::Token& peek() const;
        /** @brief Returns the most recently consumed token. */
        const lexer::Token& previous() const;
        /** @brief Consumes a required token or reports a recoverable parse error. */
        const lexer::Token& consume(lexer::TokenType type, const std::string& message);
        /// @}

        /// @name Error handling and recovery
        /// @{
        /** @brief Skips malformed input to a semicolon or likely declaration boundary. */
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
