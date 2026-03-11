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
 */

/**
 * @brief Dynamically-typed runtime value used by interpreter evaluation.
 *
 * Supported runtime categories map directly to currently implemented language types.
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
 * The interpreter catches this type to print consistent diagnostics:
 * message + source line reference.
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

class Environment : public std::enable_shared_from_this<Environment> {
public:
    /**
     * @brief Creates root environment with no enclosing scope.
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
     * @brief Reads variable by name walking enclosing chain if needed.
     */
    Value get(const lexer::Token& name) const;
    /**
     * @brief Assigns existing variable by name walking enclosing chain.
     */
    void assign(const lexer::Token& name, const Value& value);
    /**
     * @brief Reads variable from ancestor scope at exact lexical distance.
     *
     * Token form preserves line-aware RuntimeError diagnostics for mismatch cases.
     */
    Value getAt(int distance, const lexer::Token& name) const;
    /**
     * @brief Reads variable from ancestor scope by string key (internal helper use).
     */
    Value getAt(int distance, const std::string& name) const;
    /**
     * @brief Assigns variable in ancestor scope at exact lexical distance.
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
