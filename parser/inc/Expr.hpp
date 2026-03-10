#ifndef EXPR_HPP
#define EXPR_HPP

#include <memory>
#include <vector>
#include <variant>
#include <string>
#include "Token.hpp"

namespace parser {

    /**
     * @file Expr.hpp
     * @brief Defines expression AST node types used by parser/resolver/interpreter.
     *
     * Each struct models one grammar expression form. The interpreter later
     * performs runtime behavior by pattern-matching these concrete node types.
     */

    /**
     * @brief Polymorphic base for all expression nodes.
     */
    struct Expr {
        virtual ~Expr() = default;
    };

    /**
     * @brief Owning pointer alias for expression nodes.
     *
     * `std::unique_ptr` keeps AST ownership explicit and avoids accidental sharing.
     */
    using ExprPtr = std::unique_ptr<Expr>;

    // ===== Concrete Expression Types =====

    /**
     * @brief Binary operator expression (`left op right`).
     */
    struct Binary : Expr {
        ExprPtr left;
        lexer::Token op;
        ExprPtr right;

        Binary(ExprPtr left, lexer::Token op, ExprPtr right)
            : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    };

    /**
     * @brief Prefix unary expression (`op right`).
     */
    struct Unary : Expr {
        lexer::Token op;
        ExprPtr right;

        Unary(lexer::Token op, ExprPtr right)
            : op(std::move(op)), right(std::move(right)) {}
    };

    /**
     * @brief Literal expression node (`nil`, boolean, number, string).
     */
    struct Literal : Expr {
        std::variant<std::nullptr_t, bool, double, std::string> value;

        explicit Literal(std::variant<std::nullptr_t, bool, double, std::string> value)
            : value(std::move(value)) {}
    };

    /**
     * @brief Parenthesized expression grouping.
     *
     * Preserves explicit source grouping when parsing precedence.
     */
    struct Grouping : Expr {
        ExprPtr expression;

        explicit Grouping(ExprPtr expression)
            : expression(std::move(expression)) {}
    };

    /**
     * @brief Variable read expression (`name`).
     */
    struct Variable : Expr {
        lexer::Token name;

        explicit Variable(lexer::Token name)
            : name(std::move(name)) {}
    };

    /**
     * @brief Variable assignment expression (`name = value`).
     */
    struct Assign : Expr {
        lexer::Token name;
        ExprPtr value;

        Assign(lexer::Token name, ExprPtr value)
            : name(std::move(name)), value(std::move(value)) {}
    };

    /**
     * @brief Short-circuit logical expression (`and` / `or`).
     */
    struct Logical : Expr {
        ExprPtr left;
        lexer::Token op;
        ExprPtr right;

        Logical(ExprPtr left, lexer::Token op, ExprPtr right)
            : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    };

    /**
     * @brief Callable invocation expression (`callee(arguments...)`).
     */
    struct Call : Expr {
        ExprPtr callee;
        lexer::Token paren;
        std::vector<ExprPtr> arguments;

        Call(ExprPtr callee, lexer::Token paren, std::vector<ExprPtr> arguments)
            : callee(std::move(callee)),
              paren(std::move(paren)),
              arguments(std::move(arguments)) {}
    };

    /**
     * @brief Property read on object (`object.name`).
     */
    struct Get : Expr {
        ExprPtr object;
        lexer::Token name;

        Get(ExprPtr object, lexer::Token name)
            : object(std::move(object)), name(std::move(name)) {}
    };

    /**
     * @brief Property write on object (`object.name = value`).
     */
    struct Set : Expr {
        ExprPtr object;
        lexer::Token name;
        ExprPtr value;

        Set(ExprPtr object, lexer::Token name, ExprPtr value)
            : object(std::move(object)), name(std::move(name)), value(std::move(value)) {}
    };

    /**
     * @brief `this` receiver expression inside class methods.
     */
    struct This : Expr {
        lexer::Token keyword;

        explicit This(lexer::Token keyword)
            : keyword(std::move(keyword)) {}
    };

    /**
     * @brief `super.method` expression for superclass dispatch.
     */
    struct Super : Expr {
        lexer::Token keyword;
        lexer::Token method;

        Super(lexer::Token keyword, lexer::Token method)
            : keyword(std::move(keyword)), method(std::move(method)) {}
    };

} // namespace parser

#endif // EXPR_HPP
