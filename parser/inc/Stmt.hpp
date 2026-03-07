#ifndef STMT_HPP
#define STMT_HPP

#include <memory>
#include <vector>
#include "Expr.hpp"

namespace parser {

struct Stmt {
    virtual ~Stmt() = default;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct ExpressionStmt : Stmt {
    ExprPtr expression;

    explicit ExpressionStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

struct PrintStmt : Stmt {
    ExprPtr expression;

    explicit PrintStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

struct VarStmt : Stmt {
    lexer::Token name;
    ExprPtr initializer;

    VarStmt(lexer::Token name, ExprPtr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> statements)
        : statements(std::move(statements)) {}
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

struct FunctionStmt : Stmt {
    lexer::Token name;
    std::vector<lexer::Token> params;
    std::vector<StmtPtr> body;

    FunctionStmt(lexer::Token name, std::vector<lexer::Token> params, std::vector<StmtPtr> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};

struct ReturnStmt : Stmt {
    lexer::Token keyword;
    ExprPtr value;

    ReturnStmt(lexer::Token keyword, ExprPtr value)
        : keyword(std::move(keyword)), value(std::move(value)) {}
};

struct ClassStmt : Stmt {
    lexer::Token name;
    ExprPtr superclass;
    std::vector<StmtPtr> methods;

    ClassStmt(lexer::Token name, ExprPtr superclass, std::vector<StmtPtr> methods)
        : name(std::move(name)), superclass(std::move(superclass)), methods(std::move(methods)) {}
};

}  // namespace parser

#endif  // STMT_HPP
