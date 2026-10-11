#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Chunk.hpp"
#include "Expr.hpp"
#include "Stmt.hpp"

namespace bytecode {

/**
 * @brief Compiles supported AST forms for Litecode's alternate VM backend.
 *
 * The driver shares scanning, parsing, and AST construction between backends.
 * Without `--vm`, it resolves and evaluates that AST in the tree-walk
 * interpreter; with `--vm`, this compiler translates it into a `Chunk` for
 * `VirtualMachine`. This is a separate execution route, not a stage after the
 * tree-walk interpreter.
 *
 * The compiler is intentionally incomplete: it tracks block-local slots and
 * treats other names as globals, but does not use tree-walk resolver metadata.
 * Logical expressions, classes/properties/`super`, and closure capture are not
 * implemented; unsupported AST nodes may emit no instructions.
 */
class Compiler {
public:
    Compiler();

    /**
    * @brief Compiles a supported expression into a chunk terminated by Return.
     * @param expr Parsed expression tree.
    * @return Bytecode chunk whose execution result is the expression value.
    * @details Support is limited to the expression node forms handled by `emitExpression`.
     */
    Chunk compileExpression(const parser::Expr& expr);

    /**
    * @brief Compiles supported statements into one program chunk without a synthetic final Return.
    * @param statements Parsed statements; this compiler performs its own limited local-name tracking.
    * @return Bytecode representing the statement forms handled by `emitStatement`.
     */
    Chunk compileProgram(const std::vector<parser::StmtPtr>& statements);

private:
    std::vector<std::unordered_map<std::string, std::size_t>> scopes_;
    std::unordered_set<std::string> globals_;

    /** @brief Adds an empty local-name-to-slot frame for a block or function body. */
    void beginScope();
    /** @brief Drops the current compile-time name frame after its block is emitted. */
    void endScope();
    /** @brief Searches active local frames from innermost outward and returns the slot. */
    bool resolveLocal(const std::string& name, std::size_t& slot) const;
    /** @brief Assigns the next slot in the current frame to a newly declared local. */
    std::size_t declareLocal(const std::string& name);
    /** @brief Emits one supported statement and its stack cleanup/control-flow operands. */
    void emitStatement(Chunk& chunk, const parser::Stmt& stmt);
    /** @brief Emits one supported expression with its result left on the operand stack. */
    void emitExpression(Chunk& chunk, const parser::Expr& expr);
    /** @brief Writes a jump target into its operand byte; targets are currently truncated to 8 bits. */
    static void patchJump(Chunk& chunk, std::size_t jumpIndex, std::size_t target);
};

}  // namespace bytecode

#endif  // COMPILER_HPP
