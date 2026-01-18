#ifndef AST_PRINTER_HPP
#define AST_PRINTER_HPP

#include <string>
#include "Expr.hpp"

namespace parser {

class AstPrinter {
public:
    std::string print(const Expr& expr);

private:
    std::string parenthesize(const std::string& name,
                             const std::vector<const Expr*>& exprs);

    std::string visit(const Expr& expr);
};

} // namespace parser

#endif