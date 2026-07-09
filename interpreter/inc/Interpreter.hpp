#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Environment.hpp"
#include "Expr.hpp"
#include "Stmt.hpp"

namespace interpreter {

/**
 * @file Interpreter.hpp
 * @brief Declares the Litecode tree-walk interpreter runtime engine.
 */

/**
 * @brief Internal non-local control-flow signal for `return` statements.
 *
 * `return` needs to unwind nested execution contexts until function call boundary.
 * This class carries the return value and token context during that unwind.
 */
class ReturnSignal {
public:
    /**
     * @brief Constructs return signal.
     * @param keyword `return` token.
     * @param value Value being returned.
     */
    ReturnSignal(lexer::Token keyword, Value value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    lexer::Token keyword;
    Value value;
};

/**
 * @brief Executes resolved AST statements using dynamic runtime semantics.
 */
class Interpreter {
public:
    /**
     * @brief Creates interpreter with fresh global environment and native functions.
     */
    Interpreter();

    /**
     * @brief Executes a full program statement list.
     * @param statements Parsed and resolved statement list.
     */
    void interpret(const std::vector<parser::StmtPtr>& statements);
    /**
     * @brief Returns whether latest `interpret()` run encountered runtime error.
     */
    bool hadRuntimeError() const { return hadError; }
    /**
     * @brief Clears expression->distance metadata cache.
     */
    void clearResolutionCache();
    /**
     * @brief Executes statements under a temporary block environment.
     */
    void executeBlock(const std::vector<parser::StmtPtr>& statements,
                      std::shared_ptr<Environment> blockEnvironment);
    /**
     * @brief Records lexical distance for one expression node (resolver output).
     */
    void resolve(const parser::Expr* expr, int depth);

private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::unordered_map<const parser::Expr*, int> locals;
    bool hadError;

    /// @name Statement/expression execution internals
    /// @{
    void execute(const parser::Stmt& stmt);

    Value evaluate(const parser::Expr& expr);
    Value lookUpVariable(const lexer::Token& name, const parser::Expr& expr);
    /// @}

    /// @name Runtime semantic helpers
    /// @{
    bool isTruthy(const Value& value) const;
    bool isEqual(const Value& left, const Value& right) const;
    std::string stringify(const Value& value) const;
    /// @}

    /// @name Runtime validation and diagnostics
    /// @{
    void checkNumberOperand(const lexer::Token& op, const Value& operand) const;
    void checkNumberOperands(const lexer::Token& op, const Value& left, const Value& right) const;
    void runtimeError(const RuntimeError& error);
    /// @}
};

}  // namespace interpreter

#endif  // INTERPRETER_HPP
