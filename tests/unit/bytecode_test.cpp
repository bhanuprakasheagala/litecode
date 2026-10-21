#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <variant>

#include "Compiler.hpp"
#include "VM.hpp"

TEST(BytecodeTest, CompilesSimpleLiteralAndBinaryExpression) {
    auto literal = std::make_unique<parser::Literal>(42.0);
    bytecode::Compiler compiler;
    bytecode::Chunk chunk = compiler.compileExpression(*literal);

    EXPECT_FALSE(chunk.code().empty());
    EXPECT_EQ(chunk.constants().size(), 1u);
    EXPECT_TRUE(std::holds_alternative<double>(chunk.constants()[0]));
    EXPECT_DOUBLE_EQ(std::get<double>(chunk.constants()[0]), 42.0);

    auto left = std::make_unique<parser::Literal>(10.0);
    auto right = std::make_unique<parser::Literal>(5.0);
    lexer::Token op(lexer::TokenType::PLUS, "+", "", 1);
    auto binary = std::make_unique<parser::Binary>(std::move(left), std::move(op), std::move(right));

    bytecode::Chunk binaryChunk = compiler.compileExpression(*binary);
    EXPECT_GT(binaryChunk.code().size(), 0u);
    EXPECT_GE(binaryChunk.constants().size(), 2u);
}

TEST(BytecodeTest, ExecutesCompiledArithmeticChunk) {
    bytecode::Compiler compiler;
    auto left = std::make_unique<parser::Literal>(12.0);
    auto right = std::make_unique<parser::Literal>(30.0);
    lexer::Token op(lexer::TokenType::PLUS, "+", "", 1);
    auto expr = std::make_unique<parser::Binary>(std::move(left), std::move(op), std::move(right));

    bytecode::Chunk chunk = compiler.compileExpression(*expr);
    std::ostringstream output;
    bytecode::VirtualMachine vm;
    auto status = vm.interpret(chunk, output);

    EXPECT_EQ(status, bytecode::InterpretStatus::Ok);
    EXPECT_EQ(output.str(), "42\n");
}

TEST(BytecodeTest, StoresAndReadsGlobalsAcrossInterpretCalls) {
    bytecode::Compiler compiler;
    bytecode::VirtualMachine vm;

    lexer::Token name(lexer::TokenType::IDENTIFIER, "x", "", 1);
    auto assign = std::make_unique<parser::Assign>(name, std::make_unique<parser::Literal>(21.0));
    auto assignChunk = compiler.compileExpression(*assign);

    std::ostringstream assignOutput;
    EXPECT_EQ(vm.interpret(assignChunk, assignOutput), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(assignOutput.str(), "21\n");

    lexer::Token readName(lexer::TokenType::IDENTIFIER, "x", "", 1);
    auto variable = std::make_unique<parser::Variable>(readName);
    auto readChunk = compiler.compileExpression(*variable);

    std::ostringstream readOutput;
    EXPECT_EQ(vm.interpret(readChunk, readOutput), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(readOutput.str(), "21\n");
}

TEST(BytecodeTest, SupportsBlockScopedLocalVariables) {
    bytecode::Compiler compiler;
    bytecode::VirtualMachine vm;

    lexer::Token xName(lexer::TokenType::IDENTIFIER, "x", "", 1);
    auto xDecl = std::make_unique<parser::VarStmt>(xName, std::make_unique<parser::Literal>(10.0));

    lexer::Token yName(lexer::TokenType::IDENTIFIER, "y", "", 1);
    auto xRead = std::make_unique<parser::Variable>(xName);
    auto five = std::make_unique<parser::Literal>(5.0);
    lexer::Token plus(lexer::TokenType::PLUS, "+", "", 1);
    auto sum = std::make_unique<parser::Binary>(std::move(xRead), std::move(plus), std::move(five));
    auto yDecl = std::make_unique<parser::VarStmt>(yName, std::move(sum));
    auto yPrint = std::make_unique<parser::PrintStmt>(std::make_unique<parser::Variable>(yName));

    std::vector<parser::StmtPtr> blockStatements;
    blockStatements.push_back(std::move(yDecl));
    blockStatements.push_back(std::move(yPrint));
    auto block = std::make_unique<parser::BlockStmt>(std::move(blockStatements));

    std::vector<parser::StmtPtr> program;
    program.push_back(std::move(xDecl));
    program.push_back(std::move(block));

    auto chunk = compiler.compileProgram(program);
    std::ostringstream output;
    EXPECT_EQ(vm.interpret(chunk, output), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(output.str(), "15\n");
}

TEST(BytecodeTest, SupportsConditionalAndLoopExecution) {
    bytecode::Compiler compiler;
    bytecode::VirtualMachine vm;

    lexer::Token xName(lexer::TokenType::IDENTIFIER, "x", "", 1);
    auto xInit = std::make_unique<parser::VarStmt>(xName, std::make_unique<parser::Literal>(0.0));

    std::unique_ptr<parser::Expr> condition = std::make_unique<parser::Binary>(
        std::make_unique<parser::Variable>(xName),
        lexer::Token(lexer::TokenType::LESS, "<", "", 1),
        std::make_unique<parser::Literal>(3.0));

    auto loopBody = std::make_unique<parser::BlockStmt>(std::vector<parser::StmtPtr>{});
    auto& loopStatements = loopBody->statements;
    loopStatements.emplace_back(std::make_unique<parser::ExpressionStmt>(std::make_unique<parser::Assign>(
        xName,
        std::make_unique<parser::Binary>(
            std::make_unique<parser::Variable>(xName),
            lexer::Token(lexer::TokenType::PLUS, "+", "", 1),
            std::make_unique<parser::Literal>(1.0)))));

    auto whileStmt = std::make_unique<parser::WhileStmt>(std::move(condition), std::move(loopBody));
    auto ifCond = std::make_unique<parser::Binary>(
        std::make_unique<parser::Variable>(xName),
        lexer::Token(lexer::TokenType::GREATER, ">", "", 1),
        std::make_unique<parser::Literal>(2.0));
    auto ifStmt = std::make_unique<parser::IfStmt>(
        std::move(ifCond),
        std::make_unique<parser::PrintStmt>(std::make_unique<parser::Literal>(42.0)),
        std::make_unique<parser::PrintStmt>(std::make_unique<parser::Literal>(99.0)));

    auto finalPrint = std::make_unique<parser::PrintStmt>(std::make_unique<parser::Variable>(xName));

    std::vector<parser::StmtPtr> program;
    program.push_back(std::move(xInit));
    program.push_back(std::move(whileStmt));
    program.push_back(std::move(ifStmt));
    program.push_back(std::move(finalPrint));

    auto chunk = compiler.compileProgram(program);
    std::ostringstream output;
    EXPECT_EQ(vm.interpret(chunk, output), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(output.str(), "42\n3\n");
}

TEST(BytecodeTest, PreservesBlockLocalShadowingAcrossNestedScopes) {
    bytecode::Compiler compiler;
    bytecode::VirtualMachine vm;

    std::vector<parser::StmtPtr> program;
    lexer::Token outerName(lexer::TokenType::IDENTIFIER, "value", "", 1);
    program.push_back(std::make_unique<parser::VarStmt>(outerName, std::make_unique<parser::Literal>(1.0)));

    std::vector<parser::StmtPtr> innerBlock;
    innerBlock.push_back(std::make_unique<parser::VarStmt>(outerName, std::make_unique<parser::Literal>(2.0)));
    innerBlock.push_back(std::make_unique<parser::PrintStmt>(std::make_unique<parser::Variable>(outerName)));
    program.push_back(std::make_unique<parser::BlockStmt>(std::move(innerBlock)));
    program.push_back(std::make_unique<parser::PrintStmt>(std::make_unique<parser::Variable>(outerName)));

    auto chunk = compiler.compileProgram(program);
    std::ostringstream output;
    EXPECT_EQ(vm.interpret(chunk, output), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(output.str(), "2\n1\n");
}

TEST(BytecodeTest, SupportsFunctionDeclarationAndInvocation) {
    bytecode::Compiler compiler;
    bytecode::VirtualMachine vm;

    lexer::Token addName(lexer::TokenType::IDENTIFIER, "add", "", 1);
    lexer::Token leftParam(lexer::TokenType::IDENTIFIER, "a", "", 1);
    lexer::Token rightParam(lexer::TokenType::IDENTIFIER, "b", "", 1);
    std::vector<lexer::Token> params{leftParam, rightParam};

    std::vector<parser::StmtPtr> functionBody;
    auto addExpression = std::make_unique<parser::Binary>(
        std::make_unique<parser::Variable>(leftParam),
        lexer::Token(lexer::TokenType::PLUS, "+", "", 1),
        std::make_unique<parser::Variable>(rightParam));
    functionBody.push_back(std::make_unique<parser::ReturnStmt>(
        lexer::Token(lexer::TokenType::RETURN, "return", "", 1),
        std::move(addExpression)));

    std::vector<parser::StmtPtr> program;
    program.push_back(std::make_unique<parser::FunctionStmt>(addName, params, std::move(functionBody)));

    lexer::Token callParen(lexer::TokenType::RIGHT_PARENTH, ")", "", 1);
    std::vector<parser::ExprPtr> callArgs;
    callArgs.push_back(std::make_unique<parser::Literal>(2.0));
    callArgs.push_back(std::make_unique<parser::Literal>(3.0));
    program.push_back(std::make_unique<parser::PrintStmt>(std::make_unique<parser::Call>(
        std::make_unique<parser::Variable>(addName),
        callParen,
        std::move(callArgs))));

    auto chunk = compiler.compileProgram(program);
    std::ostringstream output;
    EXPECT_EQ(vm.interpret(chunk, output), bytecode::InterpretStatus::Ok);
    EXPECT_EQ(output.str(), "5\n");
}
