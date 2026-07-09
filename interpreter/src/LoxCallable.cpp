#include "LoxCallable.hpp"

#include "Interpreter.hpp"

/**
 * @file LoxCallable.cpp
 * @brief Implements runtime callable/object model: functions, classes, instances.
 */

namespace interpreter {

/**
 * @brief Builds function object from declaration and closure.
 */
LoxFunction::LoxFunction(const parser::FunctionStmt* declaration,
                         std::shared_ptr<Environment> closure,
                         bool isInitializer)
    : declaration(declaration), closure(std::move(closure)), isInitializer(isInitializer) {}

/**
 * @brief Returns function parameter count.
 */
int LoxFunction::arity() const {
    return static_cast<int>(declaration->params.size());
}

/**
 * @brief Invokes function with new call frame and handles returns.
 */
Value LoxFunction::call(Interpreter& interpreter, const std::vector<Value>& arguments) {
    auto environment = std::make_shared<Environment>(closure);
    for (size_t i = 0; i < declaration->params.size(); ++i) {
        environment->define(declaration->params[i].getLexeme(), arguments[i]);
    }

    try {
        interpreter.executeBlock(declaration->body, environment);
    } catch (const ReturnSignal& returnSignal) {
        if (isInitializer) {
            return closure->getAt(0, "this");
        }
        return returnSignal.value;
    }

    if (isInitializer) {
        return closure->getAt(0, "this");
    }
    return nullptr;
}

/**
 * @brief Returns debug string for function value.
 */
std::string LoxFunction::toString() const {
    return "<fn " + declaration->name.getLexeme() + ">";
}

/**
 * @brief Binds `this` receiver for method call.
 */
std::shared_ptr<LoxFunction> LoxFunction::bind(const std::shared_ptr<LoxInstance>& instance) const {
    auto environment = std::make_shared<Environment>(closure);
    environment->define("this", instance);
    return std::make_shared<LoxFunction>(declaration, environment, isInitializer);
}

/**
 * @brief Builds class object with optional superclass and method map.
 */
LoxClass::LoxClass(std::string name,
                   std::shared_ptr<LoxClass> superclass,
                   std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods)
    : name(std::move(name)), superclass(std::move(superclass)), methods(std::move(methods)) {}

/**
 * @brief Returns constructor arity based on `init` method if present.
 */
int LoxClass::arity() const {
    auto initializer = findMethod("init");
    if (!initializer) return 0;
    return initializer->arity();
}

/**
 * @brief Calls class as constructor: creates instance and runs initializer.
 */
Value LoxClass::call(Interpreter& interpreter, const std::vector<Value>& arguments) {
    auto instance = std::make_shared<LoxInstance>(shared_from_this());
    auto initializer = findMethod("init");
    if (initializer) {
        initializer->bind(instance)->call(interpreter, arguments);
    }
    return instance;
}

/**
 * @brief Returns class display name.
 */
std::string LoxClass::toString() const {
    return name;
}

/**
 * @brief Resolves method from class then superclass chain.
 */
std::shared_ptr<LoxFunction> LoxClass::findMethod(const std::string& methodName) const {
    auto it = methods.find(methodName);
    if (it != methods.end()) return it->second;
    if (superclass) return superclass->findMethod(methodName);
    return nullptr;
}

/**
 * @brief Creates new instance bound to class metadata.
 */
LoxInstance::LoxInstance(std::shared_ptr<LoxClass> klass) : klass(std::move(klass)) {}

/**
 * @brief Gets instance field or bound method.
 */
Value LoxInstance::get(const lexer::Token& name) {
    auto field = fields.find(name.getLexeme());
    if (field != fields.end()) {
        return field->second;
    }

    auto method = klass->findMethod(name.getLexeme());
    if (method) {
        return method->bind(shared_from_this());
    }

    throw RuntimeError(name, "Undefined property '" + name.getLexeme() + "'.");
}

/**
 * @brief Sets instance field value.
 */
void LoxInstance::set(const lexer::Token& name, const Value& value) {
    fields[name.getLexeme()] = value;
}

/**
 * @brief Returns display text for instance values.
 */
std::string LoxInstance::toString() const {
    return klass->toString() + " instance";
}

}  // namespace interpreter
