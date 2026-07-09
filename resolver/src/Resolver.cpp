#include "Resolver.hpp"

#include <iostream>

/**
 * @file Resolver.cpp
 * @brief Implements static lexical resolution and semantic rule checks.
 */

namespace resolver {

/**
 * @brief Constructs resolver with interpreter callback target.
 */
Resolver::Resolver(interpreter::Interpreter& interpreter)
    : interpreter(interpreter),
      hasError(false),
      currentFunction(FunctionType::NONE),
      currentClass(ClassType::NONE) {}

/**
 * @brief Resolves each top-level statement in order.
 */
void Resolver::resolve(const std::vector<parser::StmtPtr>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) resolve(*stmt);
    }
}

/**
 * @brief Resolves one statement node and nested children.
 */
void Resolver::resolve(const parser::Stmt& stmt) {
    if (auto expressionStmt = dynamic_cast<const parser::ExpressionStmt*>(&stmt)) {
        resolve(*expressionStmt->expression);
        return;
    }

    if (auto printStmt = dynamic_cast<const parser::PrintStmt*>(&stmt)) {
        resolve(*printStmt->expression);
        return;
    }

    if (auto varStmt = dynamic_cast<const parser::VarStmt*>(&stmt)) {
        declare(varStmt->name);
        if (varStmt->initializer) {
            resolve(*varStmt->initializer);
        }
        define(varStmt->name);
        return;
    }

    if (auto blockStmt = dynamic_cast<const parser::BlockStmt*>(&stmt)) {
        beginScope();
        resolve(blockStmt->statements);
        endScope();
        return;
    }

    if (auto ifStmt = dynamic_cast<const parser::IfStmt*>(&stmt)) {
        resolve(*ifStmt->condition);
        resolve(*ifStmt->thenBranch);
        if (ifStmt->elseBranch) resolve(*ifStmt->elseBranch);
        return;
    }

    if (auto whileStmt = dynamic_cast<const parser::WhileStmt*>(&stmt)) {
        resolve(*whileStmt->condition);
        resolve(*whileStmt->body);
        return;
    }

    if (auto functionStmt = dynamic_cast<const parser::FunctionStmt*>(&stmt)) {
        declare(functionStmt->name);
        define(functionStmt->name);
        resolveFunction(*functionStmt, FunctionType::FUNCTION);
        return;
    }

    if (auto returnStmt = dynamic_cast<const parser::ReturnStmt*>(&stmt)) {
        if (currentFunction == FunctionType::NONE) {
            error(returnStmt->keyword, "Can't return from top-level code.");
        }
        if (currentFunction == FunctionType::INITIALIZER && returnStmt->value) {
            error(returnStmt->keyword, "Can't return a value from an initializer.");
        }
        if (returnStmt->value) resolve(*returnStmt->value);
        return;
    }

    if (auto classStmt = dynamic_cast<const parser::ClassStmt*>(&stmt)) {
        ClassType enclosingClass = currentClass;
        currentClass = ClassType::CLASS;

        declare(classStmt->name);
        define(classStmt->name);

        if (classStmt->superclass) {
            auto* superVar = dynamic_cast<parser::Variable*>(classStmt->superclass.get());
            if (superVar && classStmt->name.getLexeme() == superVar->name.getLexeme()) {
                error(superVar->name, "A class can't inherit from itself.");
            }
            currentClass = ClassType::SUBCLASS;
            resolve(*classStmt->superclass);
            beginScope();
            scopes.back()["super"] = true;
        }

        beginScope();
        scopes.back()["this"] = true;
        for (const auto& method : classStmt->methods) {
            auto* function = dynamic_cast<parser::FunctionStmt*>(method.get());
            if (!function) continue;
            FunctionType declaration = FunctionType::METHOD;
            if (function->name.getLexeme() == "init") {
                declaration = FunctionType::INITIALIZER;
            }
            resolveFunction(*function, declaration);
        }
        endScope();
        if (classStmt->superclass) {
            endScope();
        }
        currentClass = enclosingClass;
        return;
    }
}

/**
 * @brief Resolves one expression node and nested children.
 */
void Resolver::resolve(const parser::Expr& expr) {
    if (auto variable = dynamic_cast<const parser::Variable*>(&expr)) {
        if (!scopes.empty()) {
            auto it = scopes.back().find(variable->name.getLexeme());
            if (it != scopes.back().end() && !it->second) {
                error(variable->name, "Can't read local variable in its own initializer.");
            }
        }
        resolveLocal(expr, variable->name);
        return;
    }

    if (auto assign = dynamic_cast<const parser::Assign*>(&expr)) {
        resolve(*assign->value);
        resolveLocal(expr, assign->name);
        return;
    }

    if (auto binary = dynamic_cast<const parser::Binary*>(&expr)) {
        resolve(*binary->left);
        resolve(*binary->right);
        return;
    }

    if (auto logical = dynamic_cast<const parser::Logical*>(&expr)) {
        resolve(*logical->left);
        resolve(*logical->right);
        return;
    }

    if (auto unary = dynamic_cast<const parser::Unary*>(&expr)) {
        resolve(*unary->right);
        return;
    }

    if (auto grouping = dynamic_cast<const parser::Grouping*>(&expr)) {
        resolve(*grouping->expression);
        return;
    }

    if (auto call = dynamic_cast<const parser::Call*>(&expr)) {
        resolve(*call->callee);
        for (const auto& arg : call->arguments) {
            resolve(*arg);
        }
        return;
    }

    if (auto get = dynamic_cast<const parser::Get*>(&expr)) {
        resolve(*get->object);
        return;
    }

    if (auto set = dynamic_cast<const parser::Set*>(&expr)) {
        resolve(*set->value);
        resolve(*set->object);
        return;
    }

    if (auto thisExpr = dynamic_cast<const parser::This*>(&expr)) {
        if (currentClass == ClassType::NONE) {
            error(thisExpr->keyword, "Can't use 'this' outside of a class.");
            return;
        }
        resolveLocal(expr, thisExpr->keyword);
        return;
    }

    if (auto superExpr = dynamic_cast<const parser::Super*>(&expr)) {
        if (currentClass == ClassType::NONE) {
            error(superExpr->keyword, "Can't use 'super' outside of a class.");
        } else if (currentClass != ClassType::SUBCLASS) {
            error(superExpr->keyword, "Can't use 'super' in a class with no superclass.");
        }
        resolveLocal(expr, superExpr->keyword);
        return;
    }
}

/**
 * @brief Resolves function body in isolated scope and function context.
 */
void Resolver::resolveFunction(const parser::FunctionStmt& function, FunctionType type) {
    FunctionType enclosingFunction = currentFunction;
    currentFunction = type;

    beginScope();
    for (const auto& param : function.params) {
        declare(param);
        define(param);
    }
    resolve(function.body);
    endScope();

    currentFunction = enclosingFunction;
}

/**
 * @brief Pushes a new lexical scope frame.
 */
void Resolver::beginScope() {
    scopes.push_back({});
}

/**
 * @brief Pops current lexical scope frame.
 */
void Resolver::endScope() {
    scopes.pop_back();
}

/**
 * @brief Marks variable name as declared but not yet defined.
 */
void Resolver::declare(const lexer::Token& name) {
    if (scopes.empty()) return;

    auto& scope = scopes.back();
    if (scope.count(name.getLexeme()) != 0) {
        error(name, "Already a variable with this name in this scope.");
    }

    scope[name.getLexeme()] = false;
}

/**
 * @brief Marks previously declared variable as defined and readable.
 */
void Resolver::define(const lexer::Token& name) {
    if (scopes.empty()) return;
    scopes.back()[name.getLexeme()] = true;
}

/**
 * @brief Emits lexical distance metadata for variable expression use.
 */
void Resolver::resolveLocal(const parser::Expr& expr, const lexer::Token& name) {
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        if (scopes[i].count(name.getLexeme()) != 0) {
            int distance = static_cast<int>(scopes.size()) - 1 - i;
            interpreter.resolve(&expr, distance);
            return;
        }
    }
}

/**
 * @brief Reports resolver semantic diagnostic.
 */
void Resolver::error(const lexer::Token& token, const std::string& message) {
    hasError = true;
    std::cerr << "[Line " << token.getLine() << "] Error at '" << token.getLexeme()
              << "': " << message << '\n';
}

}  // namespace resolver
