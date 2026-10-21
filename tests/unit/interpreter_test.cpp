#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ErrorReporter.hpp"
#include "HostConfig.hpp"
#include "Interpreter.hpp"
#include "Parser.hpp"
#include "Resolver.hpp"
#include "Scanner.hpp"

/**
 * @file interpreter_test.cpp
 * @brief Unit tests for tree-walk runtime behavior and error paths.
 */

namespace {

std::string findLitecodeBinary() {
    const auto cwd = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        cwd / "litecode",
        cwd / "build" / "litecode",
        cwd / "build-asan" / "litecode",
        cwd / "build-debug" / "litecode",
        cwd.parent_path() / "litecode",
        cwd.parent_path() / "build" / "litecode",
        cwd.parent_path() / "build-asan" / "litecode",
        cwd.parent_path() / "build-debug" / "litecode",
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            return candidate.string();
        }
    }

    return (cwd / "build" / "litecode").string();
}

struct RunResult {
    std::string out;
    std::string err;
    bool parseError = false;
    bool resolveError = false;
    bool runtimeError = false;
};

/**
 * @brief Runs full frontend/runtime pipeline against inline source snippet.
 * @param source Program text.
 * @return Captured stdout/stderr and stage error flags.
 */
RunResult runProgram(const std::string& source) {
    RunResult result;

    std::ostringstream outBuffer;
    std::ostringstream errBuffer;
    auto* oldOut = std::cout.rdbuf(outBuffer.rdbuf());
    auto* oldErr = std::cerr.rdbuf(errBuffer.rdbuf());

    lexer::ErrorReporter::reset();
    lexer::Scanner scanner(source);
    auto tokens = scanner.scanTokens();
    if (lexer::ErrorReporter::hadError()) {
        result.parseError = true;
        result.err = errBuffer.str();
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);
        return result;
    }

    parser::Parser parser(tokens);
    auto statements = parser.parse();
    result.parseError = parser.hadError();
    if (!result.parseError) {
        interpreter::Interpreter interpreter;
        resolver::Resolver resolver(interpreter);
        resolver.resolve(statements);
        result.resolveError = resolver.hadError();
        if (!result.resolveError) {
            interpreter.interpret(statements);
            result.runtimeError = interpreter.hadRuntimeError();
        }
    }

    result.out = outBuffer.str();
    result.err = errBuffer.str();
    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);
    return result;
}

}  // namespace

TEST(InterpreterTest, ExecutesClosure) {
    RunResult result = runProgram(
        "fun makeAdder(x) { "
        "  fun inner(y) { return x + y; } "
        "  return inner; "
        "} "
        "var add10 = makeAdder(10); "
        "print add10(5);");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_FALSE(result.runtimeError);
    EXPECT_EQ(result.out, "15\n");
}

TEST(InterpreterTest, ExecutesClassInheritanceAndSuper) {
    RunResult result = runProgram(
        "class A { speak() { return \"A\"; } } "
        "class B < A { speak() { return super.speak() + \"B\"; } } "
        "print B().speak();");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_FALSE(result.runtimeError);
    EXPECT_EQ(result.out, "AB\n");
}

TEST(InterpreterTest, ReportsRuntimeErrorForNonCallable) {
    RunResult result = runProgram("var a = 1; a();");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_TRUE(result.runtimeError);
    EXPECT_NE(result.err.find("Can only call functions."), std::string::npos);
}

TEST(InterpreterTest, InitializerReturnStillReturnsInstance) {
    RunResult result = runProgram(
        "class C { "
        "  init() { return; } "
        "  ok() { return 7; } "
        "} "
        "print C().ok();");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_FALSE(result.runtimeError);
    EXPECT_EQ(result.out, "7\n");
}

TEST(InterpreterTest, SuperDispatchFromOverrideWorks) {
    RunResult result = runProgram(
        "class A { "
        "  speak() { return \"A\"; } "
        "} "
        "class B < A { "
        "  speak() { return super.speak() + \"B\"; } "
        "} "
        "print B().speak();");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_FALSE(result.runtimeError);
    EXPECT_EQ(result.out, "AB\n");
}

TEST(InterpreterTest, ResolverMismatchIsReportedAsRuntimeError) {
    lexer::ErrorReporter::reset();
    lexer::Scanner scanner("var a = 1; print a;");
    auto tokens = scanner.scanTokens();
    ASSERT_FALSE(lexer::ErrorReporter::hadError());

    parser::Parser parser(tokens);
    auto statements = parser.parse();
    ASSERT_FALSE(parser.hadError());
    ASSERT_GE(statements.size(), 2u);

    interpreter::Interpreter interpreter;
    resolver::Resolver resolver(interpreter);
    resolver.resolve(statements);
    ASSERT_FALSE(resolver.hadError());

    auto* printStmt = dynamic_cast<parser::PrintStmt*>(statements[1].get());
    ASSERT_NE(printStmt, nullptr);

    // Inject an invalid lexical depth to emulate resolver/runtime metadata mismatch.
    interpreter.resolve(printStmt->expression.get(), 9999);

    std::ostringstream errBuffer;
    auto* oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
    interpreter.interpret(statements);
    std::cerr.rdbuf(oldErr);

    EXPECT_TRUE(interpreter.hadRuntimeError());
    EXPECT_NE(errBuffer.str().find("Resolver mismatch"), std::string::npos);
}

TEST(InterpreterTest, ReplConfigHelpersUseSafeDefaultsAndHelpText) {
    setenv("LITECODE_REPL_AUTO_RESET_EVERY", "0", 1);
    EXPECT_EQ(litecode::parsePositiveSizeEnv("LITECODE_REPL_AUTO_RESET_EVERY", 0, 32), 0u);

    setenv("LITECODE_REPL_AUTO_RESET_EVERY", "99", 1);
    EXPECT_EQ(litecode::parsePositiveSizeEnv("LITECODE_REPL_AUTO_RESET_EVERY", 0, 32), 32u);

    setenv("LITECODE_REPL_MAX_BATCHES", "abc", 1);
    EXPECT_EQ(litecode::parsePositiveSizeEnv("LITECODE_REPL_MAX_BATCHES", 32, 64), 32u);

    unsetenv("LITECODE_REPL_AUTO_RESET_EVERY");
    unsetenv("LITECODE_REPL_MAX_BATCHES");

    std::string help = litecode::replHelpText();
    EXPECT_NE(help.find(".help"), std::string::npos);
    EXPECT_NE(help.find(".reset"), std::string::npos);
    EXPECT_NE(help.find(".quit"), std::string::npos);

    std::string usage = litecode::usageText();
    EXPECT_NE(usage.find("Usage:"), std::string::npos);
    EXPECT_NE(usage.find("--help"), std::string::npos);
    EXPECT_NE(usage.find("REPL"), std::string::npos);

    std::string version = litecode::versionText();
    EXPECT_NE(version.find("Litecode"), std::string::npos);
    EXPECT_NE(version.find("version"), std::string::npos);
}

TEST(InterpreterTest, CliOptionsSupportDebugAndDiagnosticsFlags) {
    const char* argv[] = {
        "litecode",
        "--dump-tokens",
        "--dump-ast",
        "--diagnostics=structured",
        "--repl-auto-reset=7",
        "--repl-max-batches=9",
        "script.lox"
    };

    litecode::CliOptions options = litecode::parseCliArguments(7, const_cast<char**>(argv));
    EXPECT_TRUE(options.dumpTokens);
    EXPECT_TRUE(options.dumpAst);
    EXPECT_TRUE(options.structuredDiagnostics);
    EXPECT_EQ(options.replAutoResetEvery, 7u);
    EXPECT_EQ(options.replMaxBatches, 9u);
    EXPECT_EQ(options.scriptPath, "script.lox");
}

TEST(InterpreterTest, CliOptionsSupportVmFlag) {
    const char* argv[] = {
        "litecode",
        "--vm",
        "script.lox"
    };

    litecode::CliOptions options = litecode::parseCliArguments(3, const_cast<char**>(argv));
    EXPECT_TRUE(options.useBytecode);
    EXPECT_EQ(options.scriptPath, "script.lox");
}

TEST(InterpreterTest, StructuredDiagnosticsUseConsistentFormatting) {
    setenv("LITECODE_STRUCTURED_DIAGNOSTICS", "1", 1);

    auto parse = litecode::formatDiagnostic(litecode::DiagnosticStage::Parse, 12, "foo", "Unexpected token");
    EXPECT_NE(parse.find("[parse]"), std::string::npos);
    EXPECT_NE(parse.find("[line 12]"), std::string::npos);
    EXPECT_NE(parse.find("Unexpected token"), std::string::npos);

    auto runtime = litecode::formatDiagnostic(litecode::DiagnosticStage::Runtime, 8, "x", "Undefined variable");
    EXPECT_NE(runtime.find("[runtime]"), std::string::npos);
    EXPECT_NE(runtime.find("[line 8]"), std::string::npos);
    EXPECT_NE(runtime.find("Undefined variable"), std::string::npos);

    EXPECT_TRUE(litecode::structuredDiagnosticsEnabled());
    unsetenv("LITECODE_STRUCTURED_DIAGNOSTICS");
}

TEST(InterpreterTest, BlockScopedShadowingPreservesOuterBinding) {
    RunResult result = runProgram(
        "var value = 1; "
        "{ "
        "  var value = 2; "
        "  print value; "
        "} "
        "print value;");

    EXPECT_FALSE(result.parseError);
    EXPECT_FALSE(result.resolveError);
    EXPECT_FALSE(result.runtimeError);
    EXPECT_EQ(result.out, "2\n1\n");
}

TEST(InterpreterTest, ReplAutoResetClearsStateBetweenSuccessfulInputs) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    const std::string suffix = std::to_string(now);
    const std::string outputPath = (tempDir / ("litecode_repl_out_" + suffix + ".txt")).string();
    const std::string errorPath = (tempDir / ("litecode_repl_err_" + suffix + ".txt")).string();

    const std::string binary = findLitecodeBinary();
    const std::string command =
        "env LITECODE_REPL_AUTO_RESET_EVERY=1 \"" + binary + "\" >\"" + outputPath + "\" 2>\"" + errorPath + "\" <<'EOF'\n"
        "var x = 10;\n"
        "print x;\n"
        ".quit\n"
        "EOF";

    EXPECT_EQ(std::system(command.c_str()), 0);

    std::ifstream output(outputPath);
    ASSERT_TRUE(output.is_open());
    std::stringstream outputBuffer;
    outputBuffer << output.rdbuf();
    const std::string outputText = outputBuffer.str();

    std::ifstream error(errorPath);
    ASSERT_TRUE(error.is_open());
    std::stringstream errorBuffer;
    errorBuffer << error.rdbuf();
    const std::string errorText = errorBuffer.str();

    EXPECT_NE(outputText.find("State reset."), std::string::npos);
    EXPECT_NE(errorText.find("Undefined variable 'x'."), std::string::npos);
    EXPECT_NE(errorText.find("[line 1]"), std::string::npos);

    std::filesystem::remove(outputPath);
    std::filesystem::remove(errorPath);
}
