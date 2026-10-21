#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include "ErrorReporter.hpp"
#include "Interpreter.hpp"
#include "Parser.hpp"
#include "Resolver.hpp"
#include "Scanner.hpp"

/**
 * @file parser_resolver_test.cpp
 * @brief Unit tests for parser shape and resolver semantic validations.
 */

namespace {

/**
 * @brief Runs the scanner/parser front end and reports either phase's error state.
 *
 * Scanning errors stop the helper before parsing because the token stream is
 * incomplete; otherwise parser recovery may still return partial statements.
 */
std::vector<parser::StmtPtr> parseProgram(const std::string& source, bool* parseError) {
    lexer::ErrorReporter::reset();
    lexer::Scanner scanner(source);
    auto tokens = scanner.scanTokens();
    if (lexer::ErrorReporter::hadError()) {
        *parseError = true;
        return {};
    }

    parser::Parser parser(tokens);
    auto statements = parser.parse();
    *parseError = parser.hadError();
    return statements;
}

}  // namespace

TEST(ParserResolverTest, ParsesClassInheritanceAndSuper) {
    bool parseError = false;
    auto statements = parseProgram(
        "class A { speak() { return \"A\"; } } "
        "class B < A { speak() { return super.speak() + \"B\"; } } "
        "print B().speak();",
        &parseError);

    EXPECT_FALSE(parseError);
    EXPECT_FALSE(statements.empty());
}

TEST(ParserResolverTest, ResolverRejectsTopLevelReturn) {
    bool parseError = false;
    auto statements = parseProgram("return 1;", &parseError);
    ASSERT_FALSE(parseError);

    interpreter::Interpreter interpreter;
    resolver::Resolver resolver(interpreter);
    resolver.resolve(statements);
    EXPECT_TRUE(resolver.hadError());
}

TEST(ParserResolverTest, ResolverAllowsThisInsideClassMethod) {
    bool parseError = false;
    auto statements = parseProgram(
        "class C { method() { print this; } }",
        &parseError);
    ASSERT_FALSE(parseError);

    interpreter::Interpreter interpreter;
    resolver::Resolver resolver(interpreter);
    resolver.resolve(statements);
    EXPECT_FALSE(resolver.hadError());
}

TEST(ParserResolverTest, ResolverAllowsSuperInsideSubclass) {
    bool parseError = false;
    auto statements = parseProgram(
        "class A { method() { return 1; } } "
        "class B < A { method() { return super.method(); } }",
        &parseError);
    ASSERT_FALSE(parseError);

    interpreter::Interpreter interpreter;
    resolver::Resolver resolver(interpreter);
    resolver.resolve(statements);
    EXPECT_FALSE(resolver.hadError());
}
