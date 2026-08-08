#include "Interpreter.hpp"
#include "HostConfig.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include "LoxCallable.hpp"

/**
 * @file Interpreter.cpp
 * @brief Implements tree-walk execution/evaluation runtime for Litecode AST.
 */

namespace interpreter {

namespace {

class ClockCallable : public LoxCallable {
public:
    int arity() const override { return 0; }

    Value call(Interpreter&, const std::vector<Value>&) override {
        using clock = std::chrono::system_clock;
        auto now = clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        return static_cast<double>(millis) / 1000.0;
    }

    std::string toString() const override { return "<native fn>"; }
};

}  // namespace

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals), hadError(false) {
    globals->define("clock", std::make_shared<ClockCallable>());
}

/**
 * @brief Clears expression-distance cache produced by resolver.
 */
void Interpreter::clearResolutionCache() {
    locals.clear();
}

/**
 * @brief Stores lexical distance metadata for expression lookup optimization.
 */
void Interpreter::resolve(const parser::Expr* expr, int depth) {
    locals[expr] = depth;
}

/**
 * @brief Interprets top-level statement list and handles runtime diagnostics.
 */
void Interpreter::interpret(const std::vector<parser::StmtPtr>& statements) {
    hadError = false;
    try {
        for (const auto& stmt : statements) {
            if (stmt) execute(*stmt);
        }
    } catch (const ReturnSignal& signal) {
        runtimeError(RuntimeError(signal.keyword, "Can't return from top-level code."));
    } catch (const RuntimeError& error) {
        runtimeError(error);
    } catch (const std::exception& error) {
        hadError = true;
        std::cerr << error.what() << '\n';
    }
}

/**
 * @brief Executes one statement node.
 */
void Interpreter::execute(const parser::Stmt& stmt) {
    if (auto expressionStmt = dynamic_cast<const parser::ExpressionStmt*>(&stmt)) {
        (void)evaluate(*expressionStmt->expression);
        return;
    }

    if (auto printStmt = dynamic_cast<const parser::PrintStmt*>(&stmt)) {
        Value value = evaluate(*printStmt->expression);
        std::cout << stringify(value) << '\n';
        return;
    }

    if (auto varStmt = dynamic_cast<const parser::VarStmt*>(&stmt)) {
        Value value = nullptr;
        if (varStmt->initializer) {
            value = evaluate(*varStmt->initializer);
        }
        environment->define(varStmt->name.getLexeme(), value);
        return;
    }

    if (auto blockStmt = dynamic_cast<const parser::BlockStmt*>(&stmt)) {
        executeBlock(blockStmt->statements, std::make_shared<Environment>(environment));
        return;
    }

    if (auto ifStmt = dynamic_cast<const parser::IfStmt*>(&stmt)) {
        if (isTruthy(evaluate(*ifStmt->condition))) {
            execute(*ifStmt->thenBranch);
        } else if (ifStmt->elseBranch) {
            execute(*ifStmt->elseBranch);
        }
        return;
    }

    if (auto whileStmt = dynamic_cast<const parser::WhileStmt*>(&stmt)) {
        while (isTruthy(evaluate(*whileStmt->condition))) {
            execute(*whileStmt->body);
        }
        return;
    }

    if (auto functionStmt = dynamic_cast<const parser::FunctionStmt*>(&stmt)) {
        auto function = std::make_shared<LoxFunction>(functionStmt, environment);
        environment->define(functionStmt->name.getLexeme(), function);
        return;
    }

    if (auto returnStmt = dynamic_cast<const parser::ReturnStmt*>(&stmt)) {
        Value value = nullptr;
        if (returnStmt->value) {
            value = evaluate(*returnStmt->value);
        }
        throw ReturnSignal(returnStmt->keyword, value);
    }

    if (auto classStmt = dynamic_cast<const parser::ClassStmt*>(&stmt)) {
        std::shared_ptr<LoxClass> superclass = nullptr;
        if (classStmt->superclass) {
            Value superValue = evaluate(*classStmt->superclass);
            if (!std::holds_alternative<std::shared_ptr<LoxCallable>>(superValue)) {
                throw RuntimeError(classStmt->name, "Superclass must be a class.");
            }
            auto callable = std::get<std::shared_ptr<LoxCallable>>(superValue);
            superclass = std::dynamic_pointer_cast<LoxClass>(callable);
            if (!superclass) {
                throw RuntimeError(classStmt->name, "Superclass must be a class.");
            }
        }

        environment->define(classStmt->name.getLexeme(), nullptr);

        std::shared_ptr<Environment> previousEnvironment = environment;
        if (superclass) {
            environment = std::make_shared<Environment>(environment);
            environment->define("super", superclass);
        }

        std::unordered_map<std::string, std::shared_ptr<LoxFunction>> methods;
        for (const auto& method : classStmt->methods) {
            auto* functionStmt = dynamic_cast<parser::FunctionStmt*>(method.get());
            if (!functionStmt) continue;
            bool isInitializer = functionStmt->name.getLexeme() == "init";
            auto function = std::make_shared<LoxFunction>(functionStmt, environment, isInitializer);
            methods[functionStmt->name.getLexeme()] = function;
        }

        auto klass = std::make_shared<LoxClass>(classStmt->name.getLexeme(), superclass, std::move(methods));
        if (superclass) {
            environment = previousEnvironment;
        }
        environment->assign(classStmt->name, klass);
        return;
    }
}

/**
 * @brief Executes statement block in temporary nested environment.
 */
void Interpreter::executeBlock(const std::vector<parser::StmtPtr>& statements,
                               std::shared_ptr<Environment> blockEnvironment) {
    std::shared_ptr<Environment> previous = environment;
    try {
        environment = std::move(blockEnvironment);
        for (const auto& stmt : statements) {
            if (stmt) execute(*stmt);
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    environment = previous;
}

/**
 * @brief Evaluates one expression subtree and returns runtime value.
 */
Value Interpreter::evaluate(const parser::Expr& expr) {
    if (auto literal = dynamic_cast<const parser::Literal*>(&expr)) {
        if (std::holds_alternative<std::nullptr_t>(literal->value)) return nullptr;
        if (std::holds_alternative<bool>(literal->value)) return std::get<bool>(literal->value);
        if (std::holds_alternative<double>(literal->value)) return std::get<double>(literal->value);
        return std::get<std::string>(literal->value);
    }

    if (auto grouping = dynamic_cast<const parser::Grouping*>(&expr)) {
        return evaluate(*grouping->expression);
    }

    if (auto unary = dynamic_cast<const parser::Unary*>(&expr)) {
        Value right = evaluate(*unary->right);

        switch (unary->op.getType()) {
            case lexer::TokenType::MINUS:
                checkNumberOperand(unary->op, right);
                return -std::get<double>(right);
            case lexer::TokenType::BANG:
                return !isTruthy(right);
            default:
                break;
        }
    }

    if (auto binary = dynamic_cast<const parser::Binary*>(&expr)) {
        Value left = evaluate(*binary->left);
        Value right = evaluate(*binary->right);

        switch (binary->op.getType()) {
            case lexer::TokenType::GREATER:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) > std::get<double>(right);
            case lexer::TokenType::GREATER_EQUAL:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) >= std::get<double>(right);
            case lexer::TokenType::LESS:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) < std::get<double>(right);
            case lexer::TokenType::LESS_EQUAL:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) <= std::get<double>(right);
            case lexer::TokenType::MINUS:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) - std::get<double>(right);
            case lexer::TokenType::SLASH:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) / std::get<double>(right);
            case lexer::TokenType::STAR:
                checkNumberOperands(binary->op, left, right);
                return std::get<double>(left) * std::get<double>(right);
            case lexer::TokenType::PLUS:
                if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) + std::get<double>(right);
                }
                if (std::holds_alternative<std::string>(left) &&
                    std::holds_alternative<std::string>(right)) {
                    return std::get<std::string>(left) + std::get<std::string>(right);
                }
                throw RuntimeError(binary->op, "Operands must be two numbers or two strings.");
            case lexer::TokenType::BANG_EQUAL:
                return !isEqual(left, right);
            case lexer::TokenType::EQUAL_EQUAL:
                return isEqual(left, right);
            default:
                break;
        }
    }

    if (auto variable = dynamic_cast<const parser::Variable*>(&expr)) {
        return lookUpVariable(variable->name, expr);
    }

    if (auto assign = dynamic_cast<const parser::Assign*>(&expr)) {
        Value value = evaluate(*assign->value);
        auto local = locals.find(&expr);
        if (local != locals.end()) {
            environment->assignAt(local->second, assign->name, value);
        } else {
            globals->assign(assign->name, value);
        }
        return value;
    }

    if (auto logical = dynamic_cast<const parser::Logical*>(&expr)) {
        Value left = evaluate(*logical->left);
        if (logical->op.getType() == lexer::TokenType::OR) {
            if (isTruthy(left)) return left;
        } else {
            if (!isTruthy(left)) return left;
        }

        return evaluate(*logical->right);
    }

    if (auto call = dynamic_cast<const parser::Call*>(&expr)) {
        Value callee = evaluate(*call->callee);

        std::vector<Value> arguments;
        arguments.reserve(call->arguments.size());
        for (const auto& argument : call->arguments) {
            arguments.push_back(evaluate(*argument));
        }

        if (!std::holds_alternative<std::shared_ptr<LoxCallable>>(callee)) {
            throw RuntimeError(call->paren, "Can only call functions.");
        }

        auto callable = std::get<std::shared_ptr<LoxCallable>>(callee);
        if (static_cast<int>(arguments.size()) != callable->arity()) {
            throw RuntimeError(call->paren,
                               "Expected " + std::to_string(callable->arity()) +
                                   " arguments but got " + std::to_string(arguments.size()) + ".");
        }

        return callable->call(*this, arguments);
    }

    if (auto get = dynamic_cast<const parser::Get*>(&expr)) {
        Value object = evaluate(*get->object);
        if (std::holds_alternative<std::shared_ptr<LoxInstance>>(object)) {
            return std::get<std::shared_ptr<LoxInstance>>(object)->get(get->name);
        }
        throw RuntimeError(get->name, "Only instances have properties.");
    }

    if (auto set = dynamic_cast<const parser::Set*>(&expr)) {
        Value object = evaluate(*set->object);
        if (!std::holds_alternative<std::shared_ptr<LoxInstance>>(object)) {
            throw RuntimeError(set->name, "Only instances have fields.");
        }

        Value value = evaluate(*set->value);
        std::get<std::shared_ptr<LoxInstance>>(object)->set(set->name, value);
        return value;
    }

    if (auto thisExpr = dynamic_cast<const parser::This*>(&expr)) {
        return lookUpVariable(thisExpr->keyword, expr);
    }

    if (auto superExpr = dynamic_cast<const parser::Super*>(&expr)) {
        auto local = locals.find(&expr);
        if (local == locals.end()) {
            throw RuntimeError(superExpr->keyword, "Resolver metadata missing for 'super'.");
        }
        int distance = local->second;
        if (distance <= 0) {
            throw RuntimeError(superExpr->keyword, "Invalid resolver depth for 'super'.");
        }
        Value superclassValue = environment->getAt(distance, superExpr->keyword);
        if (!std::holds_alternative<std::shared_ptr<LoxCallable>>(superclassValue)) {
            throw RuntimeError(superExpr->keyword, "Superclass binding is invalid.");
        }
        auto superclassCallable = std::get<std::shared_ptr<LoxCallable>>(superclassValue);
        auto superclass = std::dynamic_pointer_cast<LoxClass>(superclassCallable);
        if (!superclass) {
            throw RuntimeError(superExpr->keyword, "Superclass binding is not a class.");
        }

        lexer::Token thisToken(lexer::TokenType::THIS, "this", "", superExpr->keyword.getLine());
        Value objectValue = environment->getAt(distance - 1, thisToken);
        if (!std::holds_alternative<std::shared_ptr<LoxInstance>>(objectValue)) {
            throw RuntimeError(superExpr->keyword, "'this' binding is invalid.");
        }
        auto object = std::get<std::shared_ptr<LoxInstance>>(objectValue);

        auto method = superclass->findMethod(superExpr->method.getLexeme());
        if (!method) {
            throw RuntimeError(superExpr->method,
                               "Undefined property '" + superExpr->method.getLexeme() + "'.");
        }

        return method->bind(object);
    }

    return nullptr;
}

/**
 * @brief Resolves variable by lexical distance when available, global fallback otherwise.
 */
Value Interpreter::lookUpVariable(const lexer::Token& name, const parser::Expr& expr) {
    auto local = locals.find(&expr);
    if (local != locals.end()) {
        return environment->getAt(local->second, name);
    }
    return globals->get(name);
}

/**
 * @brief Computes language truthiness semantics.
 */
bool Interpreter::isTruthy(const Value& value) const {
    if (std::holds_alternative<std::nullptr_t>(value)) return false;
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    return true;
}

/**
 * @brief Computes language equality semantics.
 */
bool Interpreter::isEqual(const Value& left, const Value& right) const {
    if (left.index() != right.index()) return false;

    if (std::holds_alternative<std::nullptr_t>(left)) return true;
    if (std::holds_alternative<bool>(left)) return std::get<bool>(left) == std::get<bool>(right);
    if (std::holds_alternative<double>(left)) return std::get<double>(left) == std::get<double>(right);
    return std::get<std::string>(left) == std::get<std::string>(right);
}

/**
 * @brief Converts runtime value to user-visible string representation.
 */
std::string Interpreter::stringify(const Value& value) const {
    if (std::holds_alternative<std::nullptr_t>(value)) return "nil";
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
    if (std::holds_alternative<double>(value)) {
        std::ostringstream out;
        out << std::setprecision(15) << std::get<double>(value);
        std::string text = out.str();
        if (text.find('.') != std::string::npos) {
            while (!text.empty() && text.back() == '0') text.pop_back();
            if (!text.empty() && text.back() == '.') text.pop_back();
        }
        return text;
    }
    if (std::holds_alternative<std::shared_ptr<LoxCallable>>(value)) {
        return std::get<std::shared_ptr<LoxCallable>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<LoxInstance>>(value)) {
        return std::get<std::shared_ptr<LoxInstance>>(value)->toString();
    }
    return std::get<std::string>(value);
}

/**
 * @brief Verifies unary numeric operand requirement.
 */
void Interpreter::checkNumberOperand(const lexer::Token& op, const Value& operand) const {
    if (std::holds_alternative<double>(operand)) return;
    throw RuntimeError(op, "Operand must be a number.");
}

/**
 * @brief Verifies binary numeric operand requirement.
 */
void Interpreter::checkNumberOperands(const lexer::Token& op,
                                      const Value& left,
                                      const Value& right) const {
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) return;
    throw RuntimeError(op, "Operands must be numbers.");
}

/**
 * @brief Emits runtime error in consistent user-facing format.
 */
void Interpreter::runtimeError(const RuntimeError& error) {
    hadError = true;
    if (::litecode::structuredDiagnosticsEnabled()) {
        std::cerr << ::litecode::formatDiagnostic(::litecode::DiagnosticStage::Runtime,
                                                 error.getToken().getLine(),
                                                 error.getToken().getLexeme(),
                                                 error.what())
                  << '\n';
    } else {
        std::cerr << error.what() << "\n[line " << error.getToken().getLine() << "]\n";
    }
}

}  // namespace interpreter
