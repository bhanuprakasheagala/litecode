#ifndef LOX_CALLABLE_HPP
#define LOX_CALLABLE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Environment.hpp"
#include "Stmt.hpp"

namespace interpreter {

class Interpreter;
class ReturnSignal;

/**
 * @file LoxCallable.hpp
 * @brief Declares callable/object runtime model: functions, classes, instances.
 */

/**
 * @brief Runtime callable interface for anything that can be invoked with `()`.
 *
 * Both user-defined functions and classes implement this interface.
 */
class LoxCallable {
public:
    virtual ~LoxCallable() = default;
    /**
     * @brief Returns required argument count.
     */
    virtual int arity() const = 0;
    /**
     * @brief Invokes callable with evaluated argument values.
     */
    virtual Value call(Interpreter& interpreter, const std::vector<Value>& arguments) = 0;
    /**
     * @brief Returns debug/user-facing textual form of callable.
     */
    virtual std::string toString() const = 0;
};

/**
 * @brief Runtime wrapper for user-defined function or bound method.
 */
class LoxFunction : public LoxCallable {
public:
    /**
     * @brief Creates runtime function object.
     * @param declaration Function AST declaration pointer.
     * @param closure Lexical closure environment captured at declaration time.
     * @param isInitializer Whether this function is class initializer (`init`).
     */
    LoxFunction(const parser::FunctionStmt* declaration,
                std::shared_ptr<Environment> closure,
                bool isInitializer = false);

    int arity() const override;
    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override;
    std::string toString() const override;
    /**
     * @brief Binds method receiver (`this`) and returns bound callable.
     */
    std::shared_ptr<LoxFunction> bind(const std::shared_ptr<class LoxInstance>& instance) const;

private:
    const parser::FunctionStmt* declaration;
    std::shared_ptr<Environment> closure;
    bool isInitializer;
};

/**
 * @brief Runtime class object, also callable to construct instances.
 */
class LoxClass : public LoxCallable, public std::enable_shared_from_this<LoxClass> {
public:
    /**
     * @brief Creates class object with optional superclass and method table.
     */
    LoxClass(std::string name,
             std::shared_ptr<LoxClass> superclass,
             std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods);

    int arity() const override;
    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override;
    std::string toString() const override;

    /**
     * @brief Resolves method by name with superclass fallback.
     */
    std::shared_ptr<LoxFunction> findMethod(const std::string& name) const;

private:
    std::string name;
    std::shared_ptr<LoxClass> superclass;
    std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods;
};

/**
 * @brief Runtime instance object holding fields and providing method binding.
 */
class LoxInstance : public std::enable_shared_from_this<LoxInstance> {
public:
    /**
     * @brief Creates instance of class.
     */
    explicit LoxInstance(std::shared_ptr<LoxClass> klass);

    /**
     * @brief Reads field or resolves/binds method.
     */
    Value get(const lexer::Token& name);
    /**
     * @brief Writes/creates field on this instance.
     */
    void set(const lexer::Token& name, const Value& value);
    /**
     * @brief Returns debug/user string form.
     */
    std::string toString() const;

private:
    std::shared_ptr<LoxClass> klass;
    std::unordered_map<std::string, Value> fields;
};

}  // namespace interpreter

#endif  // LOX_CALLABLE_HPP
