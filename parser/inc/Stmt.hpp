#ifndef STMT_HPP
#define STMT_HPP

#include <memory>
#include <vector>
#include "Expr.hpp"

namespace parser {

/**
 * @file Stmt.hpp
 * @brief Defines statement AST node types for Litecode programs.
 */

/**
 * @brief Polymorphic base type for all statement nodes.
 */
struct Stmt {
    virtual ~Stmt() = default;
};

/**
 * @brief Owning pointer alias for statements.
 */
using StmtPtr = std::unique_ptr<Stmt>;

/**
 * @brief Expression statement (`expr;`).
 */
struct ExpressionStmt : Stmt {
    ExprPtr expression;

    explicit ExpressionStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

/**
 * @brief Print statement (`print expr;`).
 */
struct PrintStmt : Stmt {
    ExprPtr expression;

    explicit PrintStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
};

/**
 * @brief Variable declaration statement (`var name = initializer;`).
 */
struct VarStmt : Stmt {
    lexer::Token name;
    ExprPtr initializer;

    VarStmt(lexer::Token name, ExprPtr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

/**
 * @brief Block statement containing nested statements and lexical scope.
 */
struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> statements)
        : statements(std::move(statements)) {}
};

/**
 * @brief Conditional statement (`if (...) then ... else ...`).
 */
struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
};

/**
 * @brief While loop statement.
 */
struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

/**
 * @brief Function declaration statement.
 *
 * Holds function name, parameter list, and body block statements.
 */
struct FunctionStmt : Stmt {
    lexer::Token name;
    std::vector<lexer::Token> params;
    std::vector<StmtPtr> body;

    FunctionStmt(lexer::Token name, std::vector<lexer::Token> params, std::vector<StmtPtr> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};

/**
 * @brief Return statement used inside function/method bodies.
 */
struct ReturnStmt : Stmt {
    lexer::Token keyword;
    ExprPtr value;

    ReturnStmt(lexer::Token keyword, ExprPtr value)
        : keyword(std::move(keyword)), value(std::move(value)) {}
};

/**
 * @brief Class declaration statement with optional superclass and methods.
 */
struct ClassStmt : Stmt {
    lexer::Token name;
    ExprPtr superclass;
    std::vector<StmtPtr> methods;

    ClassStmt(lexer::Token name, ExprPtr superclass, std::vector<StmtPtr> methods)
        : name(std::move(name)), superclass(std::move(superclass)), methods(std::move(methods)) {}
};

}  // namespace parser

#endif  // STMT_HPP
