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

class Compiler {
public:
    Compiler() {
        scopes_.emplace_back();
    }

    Chunk compileExpression(const parser::Expr& expr) {
        Chunk chunk;
        emitExpression(chunk, expr);
        chunk.writeOp(OpCode::Return);
        return chunk;
    }

    Chunk compileProgram(const std::vector<parser::StmtPtr>& statements) {
        Chunk chunk;
        for (const auto& statement : statements) {
            emitStatement(chunk, *statement);
        }
        chunk.writeOp(OpCode::Return);
        return chunk;
    }

private:
    std::vector<std::unordered_map<std::string, std::size_t>> scopes_;
    std::unordered_set<std::string> globals_;

    void beginScope() {
        scopes_.emplace_back();
    }

    void endScope() {
        if (!scopes_.empty()) {
            scopes_.pop_back();
        }
    }

    bool resolveLocal(const std::string& name, std::size_t& slot) const {
        for (int depth = static_cast<int>(scopes_.size()) - 1; depth >= 0; --depth) {
            const auto& scope = scopes_[static_cast<std::size_t>(depth)];
            const auto it = scope.find(name);
            if (it != scope.end()) {
                slot = it->second;
                return true;
            }
        }
        return false;
    }

    std::size_t declareLocal(const std::string& name) {
        auto& scope = scopes_.back();
        const auto slot = scope.size();
        scope[name] = slot;
        return slot;
    }

    void emitStatement(Chunk& chunk, const parser::Stmt& stmt) {
        if (const auto* expression = dynamic_cast<const parser::ExpressionStmt*>(&stmt)) {
            emitExpression(chunk, *expression->expression);
            chunk.writeOp(OpCode::Pop);
            return;
        }

        if (const auto* print = dynamic_cast<const parser::PrintStmt*>(&stmt)) {
            emitExpression(chunk, *print->expression);
            chunk.writeOp(OpCode::Print);
            return;
        }

        if (const auto* var = dynamic_cast<const parser::VarStmt*>(&stmt)) {
            if (var->initializer) {
                emitExpression(chunk, *var->initializer);
            } else {
                chunk.writeOp(OpCode::Nil);
            }

            if (scopes_.size() == 1) {
                globals_.insert(var->name.getLexeme());
                chunk.writeOp(OpCode::DefineGlobal);
                chunk.writeConstant(var->name.getLexeme());
            } else {
                const auto slot = declareLocal(var->name.getLexeme());
                chunk.writeOp(OpCode::SetLocal);
                chunk.write(static_cast<uint8_t>(slot));
            }
            chunk.writeOp(OpCode::Pop);
            return;
        }

        if (const auto* block = dynamic_cast<const parser::BlockStmt*>(&stmt)) {
            beginScope();
            for (const auto& statement : block->statements) {
                emitStatement(chunk, *statement);
            }
            endScope();
            return;
        }

        if (const auto* ifStmt = dynamic_cast<const parser::IfStmt*>(&stmt)) {
            emitExpression(chunk, *ifStmt->condition);
            chunk.writeOp(OpCode::JumpIfFalse);
            const std::size_t falseJumpIndex = chunk.code().size();
            chunk.write(0);

            emitStatement(chunk, *ifStmt->thenBranch);
            if (ifStmt->elseBranch) {
                chunk.writeOp(OpCode::Jump);
                const std::size_t endJumpIndex = chunk.code().size();
                chunk.write(0);

                patchJump(chunk, falseJumpIndex, chunk.code().size());
                emitStatement(chunk, *ifStmt->elseBranch);
                patchJump(chunk, endJumpIndex, chunk.code().size());
            } else {
                patchJump(chunk, falseJumpIndex, chunk.code().size());
            }
            return;
        }

        if (const auto* whileStmt = dynamic_cast<const parser::WhileStmt*>(&stmt)) {
            const std::size_t loopStart = chunk.code().size();
            emitExpression(chunk, *whileStmt->condition);
            chunk.writeOp(OpCode::JumpIfFalse);
            const std::size_t exitJumpIndex = chunk.code().size();
            chunk.write(0);

            emitStatement(chunk, *whileStmt->body);
            chunk.writeOp(OpCode::Jump);
            chunk.write(static_cast<uint8_t>(loopStart));
            patchJump(chunk, exitJumpIndex, chunk.code().size());
            return;
        }
    }

    void emitExpression(Chunk& chunk, const parser::Expr& expr) {
        if (const auto* literal = dynamic_cast<const parser::Literal*>(&expr)) {
            if (std::holds_alternative<std::nullptr_t>(literal->value)) {
                chunk.writeOp(OpCode::Nil);
                return;
            }
            if (std::holds_alternative<bool>(literal->value)) {
                if (std::get<bool>(literal->value)) {
                    chunk.writeOp(OpCode::True);
                } else {
                    chunk.writeOp(OpCode::False);
                }
                return;
            }
            if (std::holds_alternative<double>(literal->value)) {
                chunk.writeOp(OpCode::Constant);
                chunk.writeConstant(std::get<double>(literal->value));
                return;
            }
            if (std::holds_alternative<std::string>(literal->value)) {
                chunk.writeOp(OpCode::Constant);
                chunk.writeConstant(std::get<std::string>(literal->value));
                return;
            }
            return;
        }

        if (const auto* binary = dynamic_cast<const parser::Binary*>(&expr)) {
            emitExpression(chunk, *binary->left);
            emitExpression(chunk, *binary->right);

            switch (binary->op.getType()) {
                case lexer::TokenType::PLUS:
                    chunk.writeOp(OpCode::Add);
                    break;
                case lexer::TokenType::MINUS:
                    chunk.writeOp(OpCode::Subtract);
                    break;
                case lexer::TokenType::STAR:
                    chunk.writeOp(OpCode::Multiply);
                    break;
                case lexer::TokenType::SLASH:
                    chunk.writeOp(OpCode::Divide);
                    break;
                case lexer::TokenType::GREATER:
                    chunk.writeOp(OpCode::Greater);
                    break;
                case lexer::TokenType::LESS:
                    chunk.writeOp(OpCode::Less);
                    break;
                case lexer::TokenType::EQUAL_EQUAL:
                    chunk.writeOp(OpCode::Equal);
                    break;
                default:
                    break;
            }
            return;
        }

        if (const auto* unary = dynamic_cast<const parser::Unary*>(&expr)) {
            emitExpression(chunk, *unary->right);
            if (unary->op.getType() == lexer::TokenType::MINUS) {
                chunk.writeOp(OpCode::Negate);
            } else if (unary->op.getType() == lexer::TokenType::BANG) {
                chunk.writeOp(OpCode::Not);
            }
            return;
        }

        if (const auto* grouping = dynamic_cast<const parser::Grouping*>(&expr)) {
            emitExpression(chunk, *grouping->expression);
            return;
        }

        if (const auto* variable = dynamic_cast<const parser::Variable*>(&expr)) {
            std::size_t slot = 0;
            if (resolveLocal(variable->name.getLexeme(), slot)) {
                chunk.writeOp(OpCode::GetLocal);
                chunk.write(static_cast<uint8_t>(slot));
            } else {
                chunk.writeOp(OpCode::GetGlobal);
                chunk.writeConstant(variable->name.getLexeme());
            }
            return;
        }

        if (const auto* assign = dynamic_cast<const parser::Assign*>(&expr)) {
            emitExpression(chunk, *assign->value);
            std::size_t slot = 0;
            if (resolveLocal(assign->name.getLexeme(), slot)) {
                chunk.writeOp(OpCode::SetLocal);
                chunk.write(static_cast<uint8_t>(slot));
            } else if (globals_.find(assign->name.getLexeme()) != globals_.end()) {
                chunk.writeOp(OpCode::SetGlobal);
                chunk.writeConstant(assign->name.getLexeme());
            } else {
                globals_.insert(assign->name.getLexeme());
                chunk.writeOp(OpCode::DefineGlobal);
                chunk.writeConstant(assign->name.getLexeme());
            }
            return;
        }
    }

    static void patchJump(Chunk& chunk, std::size_t jumpIndex, std::size_t target) {
        chunk.writeAt(jumpIndex, static_cast<uint8_t>(target));
    }
};

}  // namespace bytecode

#endif  // COMPILER_HPP
