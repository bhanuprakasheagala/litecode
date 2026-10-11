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
    /** @brief Recursively visits one statement and manages scopes introduced by it. */
    void resolve(const parser::Stmt& stmt);
    /** @brief Recursively visits expressions and records lexical bindings on variable nodes. */
    void resolve(const parser::Expr& expr);
    /** @brief Resolves parameters/body under a function context, restoring the enclosing context afterward. */
    void resolveFunction(const parser::FunctionStmt& function, FunctionType type);
    /// @}

    /// @name Scope-stack management helpers
    /// @{
    /** @brief Adds a local scope frame; its position determines runtime lexical distance. */
    void beginScope();
    /** @brief Removes the innermost frame after all expressions in that scope are resolved. */
    void endScope();
    /** @brief Marks a local name present but unreadable during its own initializer. */
    void declare(const lexer::Token& name);
    /** @brief Marks a declared local readable after its initializer has been resolved. */
    void define(const lexer::Token& name);
    /** @brief Records an expression's lexical depth, leaving absent names for global lookup. */
    void resolveLocal(const parser::Expr& expr, const lexer::Token& name);
    /** @brief Reports a semantic error while allowing resolution to continue. */
    void error(const lexer::Token& token, const std::string& message);
    /// @}
};

}  // namespace resolver

#endif  // RESOLVER_HPP
