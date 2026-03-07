#include "AstPrinter.hpp"
#include <sstream>
#include <vector>

namespace parser {

std::string AstPrinter::print(const Expr& expr) {
    return visit(expr);
}

std::string AstPrinter::print(const Stmt& stmt) {
    return visit(stmt);
}

std::string AstPrinter::printProgram(const std::vector<StmtPtr>& statements) {
    std::ostringstream out;
    bool first = true;
    for (const auto& stmt : statements) {
        if (!stmt) continue;
        if (!first) out << '\n';
        out << visit(*stmt);
        first = false;
    }
    return out.str();
}

std::string AstPrinter::visit(const Expr& expr) {
    if (auto binary = dynamic_cast<const Binary*>(&expr)) {
        return parenthesize(binary->op.getLexeme(), {binary->left.get(), binary->right.get()});
    }

    if (auto logical = dynamic_cast<const Logical*>(&expr)) {
        return parenthesize(logical->op.getLexeme(), {logical->left.get(), logical->right.get()});
    }

    if (auto unary = dynamic_cast<const Unary*>(&expr)) {
        return parenthesize(unary->op.getLexeme(), {unary->right.get()});
    }

    if (auto grouping = dynamic_cast<const Grouping*>(&expr)) {
        return parenthesize("group", {grouping->expression.get()});
    }

    if (auto variable = dynamic_cast<const Variable*>(&expr)) {
        return variable->name.getLexeme();
    }

    if (auto assign = dynamic_cast<const Assign*>(&expr)) {
        return parenthesize("assign " + assign->name.getLexeme(), {assign->value.get()});
    }

    if (auto call = dynamic_cast<const Call*>(&expr)) {
        std::ostringstream out;
        out << "(call " << visit(*call->callee);
        for (const auto& arg : call->arguments) {
            out << " " << visit(*arg);
        }
        out << ")";
        return out.str();
    }

    if (auto get = dynamic_cast<const Get*>(&expr)) {
        return parenthesize("get " + get->name.getLexeme(), {get->object.get()});
    }

    if (auto set = dynamic_cast<const Set*>(&expr)) {
        return parenthesize("set " + set->name.getLexeme(), {set->object.get(), set->value.get()});
    }

    if (auto thisExpr = dynamic_cast<const This*>(&expr)) {
        return "(this " + thisExpr->keyword.getLexeme() + ")";
    }

    if (auto superExpr = dynamic_cast<const Super*>(&expr)) {
        return "(super " + superExpr->method.getLexeme() + ")";
    }

    if (auto literal = dynamic_cast<const Literal*>(&expr)) {
        if (std::holds_alternative<std::nullptr_t>(literal->value)) return "nil";
        if (std::holds_alternative<bool>(literal->value)) {
            return std::get<bool>(literal->value) ? "true" : "false";
        }
        if (std::holds_alternative<double>(literal->value)) {
            std::ostringstream out;
            out << std::get<double>(literal->value);
            return out.str();
        }
        if (std::holds_alternative<std::string>(literal->value)) {
            return std::get<std::string>(literal->value);
        }
    }

    return "<unknown expr>";
}

std::string AstPrinter::visit(const Stmt& stmt) {
    if (auto expressionStmt = dynamic_cast<const ExpressionStmt*>(&stmt)) {
        return parenthesize("expr", {expressionStmt->expression.get()});
    }

    if (auto printStmt = dynamic_cast<const PrintStmt*>(&stmt)) {
        return parenthesize("print", {printStmt->expression.get()});
    }

    if (auto varStmt = dynamic_cast<const VarStmt*>(&stmt)) {
        if (varStmt->initializer) {
            return parenthesize("var " + varStmt->name.getLexeme(), {varStmt->initializer.get()});
        }
        return "(var " + varStmt->name.getLexeme() + ")";
    }

    if (auto blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        std::ostringstream out;
        out << "(block";
        for (const auto& inner : blockStmt->statements) {
            if (inner) out << " " << visit(*inner);
        }
        out << ")";
        return out.str();
    }

    if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        std::ostringstream out;
        out << "(if " << visit(*ifStmt->condition) << " " << visit(*ifStmt->thenBranch);
        if (ifStmt->elseBranch) out << " " << visit(*ifStmt->elseBranch);
        out << ")";
        return out.str();
    }

    if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        std::ostringstream out;
        out << "(while " << visit(*whileStmt->condition) << " " << visit(*whileStmt->body) << ")";
        return out.str();
    }

    if (auto functionStmt = dynamic_cast<const FunctionStmt*>(&stmt)) {
        std::ostringstream out;
        out << "(fun " << functionStmt->name.getLexeme() << " (params";
        for (const auto& param : functionStmt->params) {
            out << " " << param.getLexeme();
        }
        out << ") (body";
        for (const auto& inner : functionStmt->body) {
            if (inner) out << " " << visit(*inner);
        }
        out << "))";
        return out.str();
    }

    if (auto returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        if (returnStmt->value) {
            return parenthesize("return", {returnStmt->value.get()});
        }
        return "(return)";
    }

    if (auto classStmt = dynamic_cast<const ClassStmt*>(&stmt)) {
        std::ostringstream out;
        out << "(class " << classStmt->name.getLexeme();
        if (classStmt->superclass) {
            out << " < " << visit(*classStmt->superclass);
        }
        for (const auto& method : classStmt->methods) {
            if (method) out << " " << visit(*method);
        }
        out << ")";
        return out.str();
    }

    return "<unknown stmt>";
}

std::string AstPrinter::parenthesize(const std::string& name, const std::vector<const Expr*>& exprs) {
    std::ostringstream out;
    out << "(" << name;
    for (const Expr* expr : exprs) {
        out << " " << visit(*expr);
    }
    out << ")";
    return out.str();
}

}  // namespace parser
