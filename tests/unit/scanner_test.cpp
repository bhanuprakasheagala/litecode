#include <gtest/gtest.h>

#include <vector>
#include "ErrorReporter.hpp"
#include "Scanner.hpp"

/**
 * @file scanner_test.cpp
 * @brief Unit tests for lexer scanner behavior and safety invariants.
 */

TEST(ScannerTest, RecognizesKeywordsAndDecimalNumbers) {
    lexer::Scanner scanner("while (a < 10) print a + 1.5;");
    std::vector<lexer::Token> tokens = scanner.scanTokens();

    std::vector<lexer::TokenType> types;
    types.reserve(tokens.size());
    for (const auto& token : tokens) {
        types.push_back(token.getType());
    }

    std::vector<lexer::TokenType> expected = {
        lexer::TokenType::WHILE,
        lexer::TokenType::LEFT_PARENTH,
        lexer::TokenType::IDENTIFIER,
        lexer::TokenType::LESS,
        lexer::TokenType::NUMBER,
        lexer::TokenType::RIGHT_PARENTH,
        lexer::TokenType::PRINT,
        lexer::TokenType::IDENTIFIER,
        lexer::TokenType::PLUS,
        lexer::TokenType::NUMBER,
        lexer::TokenType::SEMICOLON,
        lexer::TokenType::END_OF_FILE,
    };

    ASSERT_EQ(types, expected);
    EXPECT_EQ(tokens[9].getLiteral(), "1.5");
}

TEST(ScannerTest, NonAsciiByteDoesNotCrashAndReportsError) {
    lexer::ErrorReporter::reset();

    std::string source;
    source.push_back(static_cast<char>(0xFF));
    lexer::Scanner scanner(source);
    std::vector<lexer::Token> tokens = scanner.scanTokens();

    EXPECT_TRUE(lexer::ErrorReporter::hadError());
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens.back().getType(), lexer::TokenType::END_OF_FILE);
}
