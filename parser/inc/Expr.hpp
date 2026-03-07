#ifndef EXPR_HPP
#define EXPR_HPP

#include <memory>
#include <vector>
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

    struct Logical : Expr {
        ExprPtr left;
        lexer::Token op;
        ExprPtr right;

        Logical(ExprPtr left, lexer::Token op, ExprPtr right)
            : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    };

    struct Call : Expr {
        ExprPtr callee;
        lexer::Token paren;
        std::vector<ExprPtr> arguments;

        Call(ExprPtr callee, lexer::Token paren, std::vector<ExprPtr> arguments)
            : callee(std::move(callee)),
              paren(std::move(paren)),
              arguments(std::move(arguments)) {}
    };

    struct Get : Expr {
        ExprPtr object;
        lexer::Token name;

        Get(ExprPtr object, lexer::Token name)
            : object(std::move(object)), name(std::move(name)) {}
    };

    struct Set : Expr {
        ExprPtr object;
        lexer::Token name;
        ExprPtr value;

        Set(ExprPtr object, lexer::Token name, ExprPtr value)
            : object(std::move(object)), name(std::move(name)), value(std::move(value)) {}
    };

    struct This : Expr {
        lexer::Token keyword;

        explicit This(lexer::Token keyword)
            : keyword(std::move(keyword)) {}
    };

    struct Super : Expr {
        lexer::Token keyword;
        lexer::Token method;

        Super(lexer::Token keyword, lexer::Token method)
            : keyword(std::move(keyword)), method(std::move(method)) {}
    };

} // namespace parser

#endif // EXPR_HPP
