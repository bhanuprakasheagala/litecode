#include "Environment.hpp"

namespace interpreter {

Environment::Environment() : enclosing(nullptr) {}

Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing(std::move(enclosing)) {}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

Value Environment::get(const lexer::Token& name) const {
    auto it = values.find(name.getLexeme());
    if (it != values.end()) {
        return it->second;
    }

    if (enclosing) {
        return enclosing->get(name);
    }

    throw RuntimeError(name, "Undefined variable '" + name.getLexeme() + "'.");
}

void Environment::assign(const lexer::Token& name, const Value& value) {
    auto it = values.find(name.getLexeme());
    if (it != values.end()) {
        it->second = value;
        return;
    }

    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }

    throw RuntimeError(name, "Undefined variable '" + name.getLexeme() + "'.");
}

Value Environment::getAt(int distance, const std::string& name) const {
    auto env = ancestor(distance);
    auto it = env->values.find(name);
    if (it != env->values.end()) {
        return it->second;
    }
    throw std::runtime_error("Resolver mismatch: variable not found at resolved distance.");
}

Value Environment::getAt(int distance, const lexer::Token& name) const {
    auto env = ancestor(distance, &name);
    auto it = env->values.find(name.getLexeme());
    if (it != env->values.end()) {
        return it->second;
    }
    throw RuntimeError(name, "Resolver mismatch: variable not found at resolved distance.");
}

void Environment::assignAt(int distance, const lexer::Token& name, const Value& value) {
    auto env = ancestor(distance, &name);
    auto it = env->values.find(name.getLexeme());
    if (it != env->values.end()) {
        it->second = value;
        return;
    }
    throw RuntimeError(name, "Resolver mismatch: variable not found at resolved distance.");
}

std::shared_ptr<Environment> Environment::ancestor(int distance, const lexer::Token* token) const {
    std::shared_ptr<const Environment> environment = shared_from_this();
    for (int i = 0; i < distance; ++i) {
        if (!environment->enclosing) {
            if (token != nullptr) {
                throw RuntimeError(*token, "Resolver mismatch: invalid environment distance.");
            }
            throw std::runtime_error("Resolver mismatch: invalid environment distance.");
        }
        environment = environment->enclosing;
    }
    return std::const_pointer_cast<Environment>(environment);
}

}  // namespace interpreter
