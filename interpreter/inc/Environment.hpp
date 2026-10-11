#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include "Token.hpp"

namespace interpreter {

class LoxCallable;
class LoxInstance;

/**
 * @file Environment.hpp
 * @brief Runtime value model, runtime error type, and lexical environment chain.
 *
 * The interpreter does not evaluate raw syntax directly. It converts the AST into
 * a small runtime value system and then executes statements against a nested
 * environment chain. This keeps evaluation semantics explicit and easy to follow.
 */

/**
 * @brief Dynamically-typed runtime value used by interpreter evaluation.
 *
 * The value model is intentionally compact: it covers the language features the
 * project currently implements without introducing a larger object graph or a
 * custom GC. This is sufficient for numbers, strings, booleans, functions,
 * classes, and instance objects while keeping the runtime easy to inspect.
 */
using Value = std::variant<std::nullptr_t,
                           bool,
                           double,
                           std::string,
                           std::shared_ptr<LoxCallable>,
                           std::shared_ptr<LoxInstance>>;

/**
 * @brief Runtime exception carrying source-token context.
 *
 * The interpreter catches this type to print consistent diagnostics: a message
 * plus the offending token location. This makes runtime failures easier to trace
 * to a specific expression or statement in user code.
 */
class RuntimeError : public std::runtime_error {
public:
    /**
     * @brief Constructs runtime error with token context.
     * @param token Source token associated with failure.
     * @param message Human-readable runtime failure message.
     */
    RuntimeError(const lexer::Token& token, const std::string& message)
        : std::runtime_error(message), token(token) {}

    /**
     * @brief Gets token associated with this runtime error.
     * @return Source token used for line diagnostics.
     */
    const lexer::Token& getToken() const { return token; }

private:
    lexer::Token token;
};

/**
 * @brief Lexical environment chain used to model variable scope and closure capture.
 *
 * Each environment represents one scope frame. A nested block or function keeps a
 * reference to its parent environment, so lookups fall back outward until the
 * value is found. This is the mechanism behind closures and variable shadowing.
 */
class Environment : public std::enable_shared_from_this<Environment> {
public:
    /**
     * @brief Creates root environment with no enclosing scope.
     *
     * The global environment is the base frame for all top-level declarations.
     */
    Environment();
    /**
     * @brief Creates nested environment with enclosing parent scope.
     * @param enclosing Parent lexical environment.
     */
    explicit Environment(std::shared_ptr<Environment> enclosing);

    /**
     * @brief Defines (or overwrites) a variable in current scope frame.
     */
    void define(const std::string& name, const Value& value);
    /**
    * @brief Reads the nearest binding by name, searching enclosing scopes outward.
     */
    Value get(const lexer::Token& name) const;
    /**
    * @brief Updates the nearest existing binding; assignment never creates a name.
     */
    void assign(const lexer::Token& name, const Value& value);
    /**
     * @brief Reads variable from ancestor scope at exact lexical distance.
     *
     * Token form preserves line-aware RuntimeError diagnostics for mismatch cases.
     */
    Value getAt(int distance, const lexer::Token& name) const;
    /**
    * @brief Reads a binding at an exact scope distance using an internal string key.
    * @details Resolver/environment mismatches throw `std::runtime_error` because no source token is available.
     */
    Value getAt(int distance, const std::string& name) const;
    /**
    * @brief Updates only the binding at the resolver-specified scope distance.
     */
    void assignAt(int distance, const lexer::Token& name, const Value& value);

private:
    /**
     * @brief Finds ancestor environment `distance` hops outward.
     * @param distance Lexical hop count.
     * @param token Optional token for RuntimeError context in mismatch cases.
     */
    std::shared_ptr<Environment> ancestor(int distance, const lexer::Token* token = nullptr) const;

    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
};

}  // namespace interpreter

#endif  // ENVIRONMENT_HPP
