#include "Environment.hpp"

/**
 * @file Environment.cpp
 * @brief Implements runtime lexical environment chain operations.
 */

namespace interpreter {

/**
 * @brief Builds root scope frame.
 */
Environment::Environment() : enclosing(nullptr) {}

/**
 * @brief Builds nested scope frame enclosing parent frame.
 */
Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing(std::move(enclosing)) {}

/**
 * @brief Defines/overwrites variable in current frame.
 */
void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

/**
 * @brief Gets variable value by lexical name walking outward.
 */
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

/**
 * @brief Assigns variable value by lexical name walking outward.
 */
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

/**
 * @brief Internal string-keyed ancestor lookup helper.
 */
Value Environment::getAt(int distance, const std::string& name) const {
    auto env = ancestor(distance);
    auto it = env->values.find(name);
    if (it != env->values.end()) {
        return it->second;
    }
    throw std::runtime_error("Resolver mismatch: variable not found at resolved distance.");
}

/**
 * @brief Token-aware ancestor lookup preserving RuntimeError context.
 */
Value Environment::getAt(int distance, const lexer::Token& name) const {
    auto env = ancestor(distance, &name);
    auto it = env->values.find(name.getLexeme());
    if (it != env->values.end()) {
        return it->second;
    }
    throw RuntimeError(name, "Resolver mismatch: variable not found at resolved distance.");
}

/**
 * @brief Assigns ancestor variable at resolved distance.
 */
void Environment::assignAt(int distance, const lexer::Token& name, const Value& value) {
    auto env = ancestor(distance, &name);
    auto it = env->values.find(name.getLexeme());
    if (it != env->values.end()) {
        it->second = value;
        return;
    }
    throw RuntimeError(name, "Resolver mismatch: variable not found at resolved distance.");
}

/**
 * @brief Returns ancestor environment `distance` hops out.
 */
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
