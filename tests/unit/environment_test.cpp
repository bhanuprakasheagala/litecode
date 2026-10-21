#include <gtest/gtest.h>

#include <memory>
#include "Environment.hpp"
#include "Token.hpp"
#include "TokenType.hpp"

/**
 * @file environment_test.cpp
 * @brief Unit tests for runtime lexical environment distance operations.
 */

TEST(EnvironmentTest, AssignAtAndGetAtAcrossNestedScopes) {
    auto global = std::make_shared<interpreter::Environment>();
    global->define("a", 1.0);

    auto mid = std::make_shared<interpreter::Environment>(global);
    auto local = std::make_shared<interpreter::Environment>(mid);

    lexer::Token name(lexer::TokenType::IDENTIFIER, "a", "", 1);
    local->assignAt(2, name, 42.0);

    auto globalValue = global->get(name);
    ASSERT_TRUE(std::holds_alternative<double>(globalValue));
    EXPECT_EQ(std::get<double>(globalValue), 42.0);

    auto viaDistance = local->getAt(2, "a");
    ASSERT_TRUE(std::holds_alternative<double>(viaDistance));
    EXPECT_EQ(std::get<double>(viaDistance), 42.0);
}
