#include "Compiler.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace bytecode {

/**
 * @brief Creates the compiler with a single global scope frame.
 */
Compiler::Compiler() {
    scopes_.emplace_back();
}

/**
 * @brief Compiles a single expression into a chunk that leaves a result on the
 * stack.
 */
/** @brief Emits the expression and a Return opcode so the VM exposes its resulting value. */
Chunk Compiler::compileExpression(const parser::Expr& expr) {
    Chunk chunk;
    emitExpression(chunk, expr);
    chunk.writeOp(OpCode::Return);
    return chunk;
}

/**
 * @brief Compiles a program into a chunk that leaves a result on the stack.
 */
/**
 * @brief Emits top-level statements in source order without a final Return instruction.
 *
 * Program statements perform their own stack cleanup. A synthetic Return here
 * would incorrectly turn the final stack item into expression-result output.
 */
Chunk Compiler::compileProgram(const std::vector<parser::StmtPtr>& statements) {
    Chunk chunk;
    for (const auto& statement : statements) {
        emitStatement(chunk, *statement);
    }
    return chunk;
}

/** @brief Starts a slot namespace whose local declarations shadow outer names. */
void Compiler::beginScope() {
    scopes_.emplace_back();
}

/** @brief Removes compile-time name visibility when leaving the current block. */
void Compiler::endScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

/** @brief Finds the nearest active local; a miss causes the emitter to use global bytecode. */
bool Compiler::resolveLocal(const std::string& name, std::size_t& slot) const {
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

/** @brief Allocates the next slot in the current frame; slot lifetime follows emitted scope nesting. */
std::size_t Compiler::declareLocal(const std::string& name) {
    auto& scope = scopes_.back();
    const auto slot = scope.size();
    scope[name] = slot;
    return slot;
}

/**
 * @brief Lowers supported statements while maintaining the VM stack contract.
 *
 * Expression statements and declarations explicitly discard their temporary
 * results. Function declarations are installed as globals in this bootstrap
 * compiler; they do not capture local environments.
 */
void Compiler::emitStatement(Chunk& chunk, const parser::Stmt& stmt) {
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

    if (const auto* function = dynamic_cast<const parser::FunctionStmt*>(&stmt)) {
        Chunk functionChunk;
        beginScope();
        for (const auto& param : function->params) {
            declareLocal(param.getLexeme());
        }
        for (const auto& bodyStmt : function->body) {
            emitStatement(functionChunk, *bodyStmt);
        }
        if (functionChunk.code().empty() || functionChunk.code().back() != static_cast<uint8_t>(OpCode::Return)) {
            functionChunk.writeOp(OpCode::Nil);
            functionChunk.writeOp(OpCode::Return);
        }
        endScope();

        auto fn = std::make_shared<Function>();
        fn->name = function->name.getLexeme();
        fn->params = std::vector<std::string>();
        for (const auto& param : function->params) {
            fn->params.push_back(param.getLexeme());
        }
        fn->code = functionChunk.code();
        fn->constants = functionChunk.constants();

        globals_.insert(function->name.getLexeme());
        chunk.writeOp(OpCode::Constant);
        chunk.writeConstant(fn);
        chunk.writeOp(OpCode::DefineGlobal);
        chunk.writeConstant(function->name.getLexeme());
        chunk.writeOp(OpCode::Pop);
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const parser::ReturnStmt*>(&stmt)) {
        if (returnStmt->value) {
            emitExpression(chunk, *returnStmt->value);
        } else {
            chunk.writeOp(OpCode::Nil);
        }
        chunk.writeOp(OpCode::Return);
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

/**
 * @brief Lowers supported expressions so each successful path leaves one value.
 *
 * Names are encoded as local-slot operations when found in active compile-time
 * scopes and otherwise as global-name constants. Unhandled AST node types have
 * no fallback diagnostic in this experimental compiler.
 */
void Compiler::emitExpression(Chunk& chunk, const parser::Expr& expr) {
    if (const auto* literal = dynamic_cast<const parser::Literal*>(&expr)) {
        if (std::holds_alternative<std::nullptr_t>(literal->value)) {
            chunk.writeOp(OpCode::Nil);
            return;
        }
        if (std::holds_alternative<bool>(literal->value)) {
            chunk.writeOp(std::get<bool>(literal->value) ? OpCode::True : OpCode::False);
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

    if (const auto* call = dynamic_cast<const parser::Call*>(&expr)) {
        emitExpression(chunk, *call->callee);
        for (const auto& argument : call->arguments) {
            emitExpression(chunk, *argument);
        }
        chunk.writeOp(OpCode::Call);
        chunk.write(static_cast<uint8_t>(call->arguments.size()));
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

/** @brief Patches a previously reserved operand byte with the absolute target offset. */
void Compiler::patchJump(Chunk& chunk, std::size_t jumpIndex, std::size_t target) {
    if (jumpIndex >= chunk.code().size()) {
        return;
    }
    chunk.writeAt(jumpIndex, static_cast<uint8_t>(target));
}

}  // namespace bytecode
