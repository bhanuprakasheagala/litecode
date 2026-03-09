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

using Value = std::variant<std::nullptr_t,
                           bool,
                           double,
                           std::string,
                           std::shared_ptr<LoxCallable>,
                           std::shared_ptr<LoxInstance>>;

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const lexer::Token& token, const std::string& message)
        : std::runtime_error(message), token(token) {}

    const lexer::Token& getToken() const { return token; }

private:
    lexer::Token token;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment();
    explicit Environment(std::shared_ptr<Environment> enclosing);

    void define(const std::string& name, const Value& value);
    Value get(const lexer::Token& name) const;
    void assign(const lexer::Token& name, const Value& value);
    Value getAt(int distance, const lexer::Token& name) const;
    Value getAt(int distance, const std::string& name) const;
    void assignAt(int distance, const lexer::Token& name, const Value& value);

private:
    std::shared_ptr<Environment> ancestor(int distance, const lexer::Token* token = nullptr) const;

    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
};

}  // namespace interpreter

#endif  // ENVIRONMENT_HPP
