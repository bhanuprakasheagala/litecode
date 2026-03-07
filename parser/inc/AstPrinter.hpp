#ifndef AST_PRINTER_HPP
#define AST_PRINTER_HPP

#include <string>
#include <vector>
#include "Expr.hpp"
#include "Stmt.hpp"

namespace parser {

class AstPrinter {
public:
    std::string print(const Expr& expr);
    std::string print(const Stmt& stmt);
    std::string printProgram(const std::vector<StmtPtr>& statements);

private:
    std::string parenthesize(const std::string& name,
                             const std::vector<const Expr*>& exprs);

    std::string visit(const Expr& expr);
    std::string visit(const Stmt& stmt);
};

} // namespace parser

#endif
