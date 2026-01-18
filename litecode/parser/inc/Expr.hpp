#ifndef EXPR_HPP
#define EXPR_HPP

#include <memory>
#include <variant>
#include <string>
#include "Token.hpp"

namespace parser {

    struct Expr {
        virtual ~Expr() = default;
    };

    using ExprPtr = std::unique_ptr<Expr>;

    // ===== Expression Types =====

    struct Binary : Expr {
        ExprPtr left;
        lexer::Token op;
        ExprPtr right;

        Binary(ExprPtr left, lexer::Token op, ExprPtr right)
            : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    };

    struct Unary : Expr {
        lexer::Token op;
        ExprPtr right;

        Unary(lexer::Token op, ExprPtr right)
            : op(std::move(op)), right(std::move(right)) {}
    };

    struct Literal : Expr {
        std::variant<std::nullptr_t, bool, double, std::string> value;

        explicit Literal(std::variant<std::nullptr_t, bool, double, std::string> value)
            : value(std::move(value)) {}
    };

    struct Grouping : Expr {
        ExprPtr expression;

        explicit Grouping(ExprPtr expression)
            : expression(std::move(expression)) {}
    };

    struct Variable : Expr {
        lexer::Token name;

        explicit Variable(lexer::Token name)
            : name(std::move(name)) {}
    };

    struct Assign : Expr {
        lexer::Token name;
        ExprPtr value;

        Assign(lexer::Token name, ExprPtr value)
            : name(std::move(name)), value(std::move(value)) {}
    };

} // namespace parser

#endif // EXPR_HPP