#include "AstPrinter.hpp"
#include <sstream>

namespace parser {

std::string AstPrinter::print(const Expr& expr) {
    return visit(expr);
}

std::string AstPrinter::visit(const Expr& expr) {
    if (auto binary = dynamic_cast<const Binary*>(&expr)) {
        return parenthesize(binary->op.getLexeme(),
                            { binary->left.get(), binary->right.get() });
    }

    if (auto unary = dynamic_cast<const Unary*>(&expr)) {
        return parenthesize(unary->op.getLexeme(),
                            { unary->right.get() });
    }

    if (auto grouping = dynamic_cast<const Grouping*>(&expr)) {
        return parenthesize("group", { grouping->expression.get() });
    }

    if (auto literal = dynamic_cast<const Literal*>(&expr)) {
        if (std::holds_alternative<std::nullptr_t>(literal->value)) return "nil";
        if (std::holds_alternative<bool>(literal->value))
            return std::get<bool>(literal->value) ? "true" : "false";
        if (std::holds_alternative<double>(literal->value)){
            std::ostringstream out;
            out << std::get<double>(literal->value);
            return out.str();
        }
        if (std::holds_alternative<std::string>(literal->value))
            return std::get<std::string>(literal->value);
    }

    return "<unknown expr>";
}

std::string AstPrinter::parenthesize(
    const std::string& name,
    const std::vector<const Expr*>& exprs) {

    std::ostringstream out;
    out << "(" << name;
    for (const Expr* expr : exprs) {
        out << " " << visit(*expr);
    }
    out << ")";
    return out.str();
}

} // namespace parser