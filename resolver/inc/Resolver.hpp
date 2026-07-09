#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "Expr.hpp"
#include "Interpreter.hpp"
#include "Stmt.hpp"

namespace resolver {

/**
 * @file Resolver.hpp
 * @brief Declares semantic resolver pass for lexical scope binding.
 */

/**
 * @brief Performs static scope analysis between parse and runtime stages.
 *
 * Resolver responsibilities:
 * - track variable declarations/definitions by lexical scope,
 * - compute scope-distance metadata for variable expressions,
 * - enforce static semantic constraints (`return`, `this`, `super`, etc.).
 */
class Resolver {
public:
    /**
     * @brief Creates resolver bound to interpreter for distance metadata output.
     * @param interpreter Interpreter receiving resolved lexical distances.
     */
    explicit Resolver(interpreter::Interpreter& interpreter);

    /**
     * @brief Resolves all top-level statements in a parsed program.
     * @param statements Program AST.
     */
    void resolve(const std::vector<parser::StmtPtr>& statements);
    /**
     * @brief Indicates if semantic resolution errors occurred.
     * @return True when resolver emitted at least one diagnostic.
     */
    bool hadError() const { return hasError; }

private:
    /**
     * @brief Tracks current function-context semantics while resolving.
     */
    enum class FunctionType {
        NONE,
        FUNCTION,
        METHOD,
        INITIALIZER
    };

    /**
     * @brief Tracks class context for validating `this` and `super`.
     */
    enum class ClassType {
        NONE,
        CLASS,
        SUBCLASS
    };

    interpreter::Interpreter& interpreter;
    std::vector<std::unordered_map<std::string, bool>> scopes;
    bool hasError;
    FunctionType currentFunction;
    ClassType currentClass;

    /// @name Recursive resolve entry points
    /// @{
    void resolve(const parser::Stmt& stmt);
    void resolve(const parser::Expr& expr);
    void resolveFunction(const parser::FunctionStmt& function, FunctionType type);
    /// @}

    /// @name Scope-stack management helpers
    /// @{
    void beginScope();
    void endScope();
    void declare(const lexer::Token& name);
    void define(const lexer::Token& name);
    void resolveLocal(const parser::Expr& expr, const lexer::Token& name);
    void error(const lexer::Token& token, const std::string& message);
    /// @}
};

}  // namespace resolver

#endif  // RESOLVER_HPP
