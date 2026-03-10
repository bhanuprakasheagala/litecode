#ifndef AST_PRINTER_HPP
#define AST_PRINTER_HPP

#include <string>
#include <vector>
#include "Expr.hpp"
#include "Stmt.hpp"

namespace parser {

/**
 * @file AstPrinter.hpp
 * @brief Declares debug utility for rendering AST nodes as text.
 */

/**
 * @brief Pretty-printer for expressions/statements/programs.
 *
 * This class is used for debugging parser output and learning how source
 * syntax maps to AST structure.
 */
class AstPrinter {
public:
    /**
     * @brief Renders one expression node.
     * @param expr Expression root.
     * @return Text form of expression subtree.
     */
    std::string print(const Expr& expr);
    /**
     * @brief Renders one statement node.
     * @param stmt Statement root.
     * @return Text form of statement subtree.
     */
    std::string print(const Stmt& stmt);
    /**
     * @brief Renders a full parsed program.
     * @param statements Top-level statements.
     * @return Multi-line textual AST program representation.
     */
    std::string printProgram(const std::vector<StmtPtr>& statements);

private:
    /**
     * @brief Shared helper for Lisp-like parenthesized expression output.
     */
    std::string parenthesize(const std::string& name,
                             const std::vector<const Expr*>& exprs);

    /**
     * @brief Internal dynamic dispatch helper for expression nodes.
     */
    std::string visit(const Expr& expr);
    /**
     * @brief Internal dynamic dispatch helper for statement nodes.
     */
    std::string visit(const Stmt& stmt);
};

} // namespace parser

#endif
