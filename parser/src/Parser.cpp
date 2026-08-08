#include "Parser.hpp"
#include "HostConfig.hpp"
#include <iostream>
#include <utility>

/**
 * @file Parser.cpp
 * @brief Implements recursive-descent parser for Litecode grammar.
 */

namespace parser {

/**
 * @brief Initializes parser over scanner token sequence.
 */
Parser::Parser(const std::vector<lexer::Token>& tokens)
    : tokens(tokens), current(0), hadParseError(false) {}

/**
 * @brief Parses entire token stream into top-level statement list.
 */
std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) {
        StmtPtr stmt = declaration();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    return statements;
}

/**
 * @brief Parses declaration production with error synchronization boundary.
 */
StmtPtr Parser::declaration() {
    try {
        if (match({lexer::TokenType::CLASS})) return classDeclaration();
        if (match({lexer::TokenType::FUN})) return function("function");
        if (match({lexer::TokenType::VAR})) return varDeclaration();
        return statement();
    } catch (const ParseError&) {
        synchronize();
        return nullptr;
    }
}

/**
 * @brief Parses class declaration including optional superclass and methods.
 */
StmtPtr Parser::classDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expect class name.");

    ExprPtr superclass = nullptr;
    if (match({lexer::TokenType::LESS})) {
        consume(lexer::TokenType::IDENTIFIER, "Expect superclass name.");
        superclass = std::make_unique<Variable>(previous());
    }
    consume(lexer::TokenType::LEFT_BRACE, "Expect '{' before class body.");

    std::vector<StmtPtr> methods;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        methods.push_back(function("method"));
    }

    consume(lexer::TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    return std::make_unique<ClassStmt>(std::move(name), std::move(superclass), std::move(methods));
}

/**
 * @brief Parses function or method declaration.
 */
StmtPtr Parser::function(const std::string& kind) {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expect " + kind + " name.");
    consume(lexer::TokenType::LEFT_PARENTH, "Expect '(' after " + kind + " name.");

    std::vector<lexer::Token> parameters;
    if (!check(lexer::TokenType::RIGHT_PARENTH)) {
        do {
            if (parameters.size() >= 255) {
                throw error(peek(), "Can't have more than 255 parameters.");
            }
            parameters.push_back(
                consume(lexer::TokenType::IDENTIFIER, "Expect parameter name."));
        } while (match({lexer::TokenType::COMMA}));
    }
    consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after parameters.");
    consume(lexer::TokenType::LEFT_BRACE, "Expect '{' before " + kind + " body.");
    std::vector<StmtPtr> body = block();
    return std::make_unique<FunctionStmt>(std::move(name), std::move(parameters), std::move(body));
}

/**
 * @brief Parses variable declaration statement.
 */
StmtPtr Parser::varDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expect variable name.");

    ExprPtr initializer = nullptr;
    if (match({lexer::TokenType::EQUAL})) {
        initializer = expression();
    }

    consume(lexer::TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(std::move(name), std::move(initializer));
}

/**
 * @brief Parses generic statement production.
 */
StmtPtr Parser::statement() {
    if (match({lexer::TokenType::FOR})) return forStatement();
    if (match({lexer::TokenType::IF})) return ifStatement();
    if (match({lexer::TokenType::PRINT})) return printStatement();
    if (match({lexer::TokenType::RETURN})) return returnStatement();
    if (match({lexer::TokenType::WHILE})) return whileStatement();
    if (match({lexer::TokenType::LEFT_BRACE})) return std::make_unique<BlockStmt>(block());
    return expressionStatement();
}

/**
 * @brief Parses `for` and desugars it into `while` + block AST.
 */
StmtPtr Parser::forStatement() {
    consume(lexer::TokenType::LEFT_PARENTH, "Expect '(' after 'for'.");

    StmtPtr initializer;
    if (match({lexer::TokenType::SEMICOLON})) {
        initializer = nullptr;
    } else if (match({lexer::TokenType::VAR})) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }

    ExprPtr condition;
    if (!check(lexer::TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(lexer::TokenType::SEMICOLON, "Expect ';' after loop condition.");

    ExprPtr increment;
    if (!check(lexer::TokenType::RIGHT_PARENTH)) {
        increment = expression();
    }
    consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after for clauses.");

    StmtPtr body = statement();

    if (increment) {
        std::vector<StmtPtr> statements;
        statements.push_back(std::move(body));
        statements.push_back(std::make_unique<ExpressionStmt>(std::move(increment)));
        body = std::make_unique<BlockStmt>(std::move(statements));
    }

    if (!condition) {
        condition = std::make_unique<Literal>(true);
    }
    body = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

    if (initializer) {
        std::vector<StmtPtr> statements;
        statements.push_back(std::move(initializer));
        statements.push_back(std::move(body));
        body = std::make_unique<BlockStmt>(std::move(statements));
    }

    return body;
}

/**
 * @brief Parses `if`/`else` statement.
 */
StmtPtr Parser::ifStatement() {
    consume(lexer::TokenType::LEFT_PARENTH, "Expect '(' after 'if'.");
    ExprPtr condition = expression();
    consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after if condition.");

    StmtPtr thenBranch = statement();
    StmtPtr elseBranch = nullptr;
    if (match({lexer::TokenType::ELSE})) {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

/**
 * @brief Parses `while` statement.
 */
StmtPtr Parser::whileStatement() {
    consume(lexer::TokenType::LEFT_PARENTH, "Expect '(' after 'while'.");
    ExprPtr condition = expression();
    consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after condition.");
    StmtPtr body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

/**
 * @brief Parses `print` statement.
 */
StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    consume(lexer::TokenType::SEMICOLON, "Expect ';' after value.");
    return std::make_unique<PrintStmt>(std::move(value));
}

/**
 * @brief Parses `return` statement with optional value expression.
 */
StmtPtr Parser::returnStatement() {
    lexer::Token keyword = previous();
    ExprPtr value = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        value = expression();
    }
    consume(lexer::TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(keyword), std::move(value));
}

/**
 * @brief Parses expression statement.
 */
StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(lexer::TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

/**
 * @brief Parses brace-delimited block body.
 */
std::vector<StmtPtr> Parser::block() {
    std::vector<StmtPtr> statements;

    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        StmtPtr stmt = declaration();
        if (stmt) statements.push_back(std::move(stmt));
    }

    consume(lexer::TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return statements;
}

/**
 * @brief Entry point for expression precedence chain.
 */
ExprPtr Parser::expression() {
    return assignment();
}

/**
 * @brief Parses assignment expression.
 */
ExprPtr Parser::assignment() {
    ExprPtr expr = logicalOr();

    if (match({lexer::TokenType::EQUAL})) {
        lexer::Token equals = previous();
        ExprPtr value = assignment();

        if (auto* variable = dynamic_cast<Variable*>(expr.get())) {
            lexer::Token name = variable->name;
            return std::make_unique<Assign>(std::move(name), std::move(value));
        }
        if (auto* get = dynamic_cast<Get*>(expr.get())) {
            lexer::Token name = get->name;
            ExprPtr object = std::move(get->object);
            return std::make_unique<Set>(std::move(object), std::move(name), std::move(value));
        }

        throw error(equals, "Invalid assignment target.");
    }

    return expr;
}

/**
 * @brief Parses logical OR expression.
 */
ExprPtr Parser::logicalOr() {
    ExprPtr expr = logicalAnd();

    while (match({lexer::TokenType::OR})) {
        lexer::Token op = previous();
        ExprPtr right = logicalAnd();
        expr = std::make_unique<Logical>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses logical AND expression.
 */
ExprPtr Parser::logicalAnd() {
    ExprPtr expr = equality();

    while (match({lexer::TokenType::AND})) {
        lexer::Token op = previous();
        ExprPtr right = equality();
        expr = std::make_unique<Logical>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses equality expression.
 */
ExprPtr Parser::equality() {
    ExprPtr expr = comparison();

    while (match({lexer::TokenType::BANG_EQUAL, lexer::TokenType::EQUAL_EQUAL})) {
        lexer::Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses comparison expression.
 */
ExprPtr Parser::comparison() {
    ExprPtr expr = term();

    while (match({lexer::TokenType::GREATER,
                  lexer::TokenType::GREATER_EQUAL,
                  lexer::TokenType::LESS,
                  lexer::TokenType::LESS_EQUAL})) {
        lexer::Token op = previous();
        ExprPtr right = term();
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses additive expression (`+`, `-`).
 */
ExprPtr Parser::term() {
    ExprPtr expr = factor();

    while (match({lexer::TokenType::MINUS, lexer::TokenType::PLUS})) {
        lexer::Token op = previous();
        ExprPtr right = factor();
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses multiplicative expression (`*`, `/`).
 */
ExprPtr Parser::factor() {
    ExprPtr expr = unary();

    while (match({lexer::TokenType::SLASH, lexer::TokenType::STAR})) {
        lexer::Token op = previous();
        ExprPtr right = unary();
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

/**
 * @brief Parses unary expression.
 */
ExprPtr Parser::unary() {
    if (match({lexer::TokenType::BANG, lexer::TokenType::MINUS})) {
        lexer::Token op = previous();
        ExprPtr right = unary();
        return std::make_unique<Unary>(std::move(op), std::move(right));
    }

    return call();
}

/**
 * @brief Parses call/property-chaining expression suffixes.
 */
ExprPtr Parser::call() {
    ExprPtr expr = primary();

    while (true) {
        if (match({lexer::TokenType::LEFT_PARENTH})) {
            expr = finishCall(std::move(expr));
        } else if (match({lexer::TokenType::DOT})) {
            lexer::Token name =
                consume(lexer::TokenType::IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_unique<Get>(std::move(expr), std::move(name));
        } else {
            break;
        }
    }

    return expr;
}

/**
 * @brief Parses argument list and call closing token.
 */
ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> arguments;
    if (!check(lexer::TokenType::RIGHT_PARENTH)) {
        do {
            if (arguments.size() >= 255) {
                throw error(peek(), "Can't have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match({lexer::TokenType::COMMA}));
    }

    lexer::Token paren = consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after arguments.");
    return std::make_unique<Call>(std::move(callee), std::move(paren), std::move(arguments));
}

/**
 * @brief Parses primary expression forms (literals, identifiers, grouping, this/super).
 */
ExprPtr Parser::primary() {
    if (match({lexer::TokenType::FALSE})) return std::make_unique<Literal>(false);
    if (match({lexer::TokenType::TRUE})) return std::make_unique<Literal>(true);
    if (match({lexer::TokenType::NILL})) return std::make_unique<Literal>(nullptr);
    if (match({lexer::TokenType::THIS})) return std::make_unique<This>(previous());
    if (match({lexer::TokenType::SUPER})) {
        lexer::Token keyword = previous();
        consume(lexer::TokenType::DOT, "Expect '.' after 'super'.");
        lexer::Token method =
            consume(lexer::TokenType::IDENTIFIER, "Expect superclass method name.");
        return std::make_unique<Super>(std::move(keyword), std::move(method));
    }

    if (match({lexer::TokenType::NUMBER})) {
        return std::make_unique<Literal>(std::stod(previous().getLiteral()));
    }

    if (match({lexer::TokenType::STRING})) {
        return std::make_unique<Literal>(previous().getLiteral());
    }

    if (match({lexer::TokenType::IDENTIFIER})) {
        return std::make_unique<Variable>(previous());
    }

    if (match({lexer::TokenType::LEFT_PARENTH})) {
        ExprPtr expr = expression();
        consume(lexer::TokenType::RIGHT_PARENTH, "Expect ')' after expression.");
        return std::make_unique<Grouping>(std::move(expr));
    }

    throw error(peek(), "Expected expression.");
}

/**
 * @brief Tries to match and consume any token in the given set.
 */
bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks whether current token has requested type.
 */
bool Parser::check(lexer::TokenType type) const {
    if (isAtEnd()) return false;
    return peek().getType() == type;
}

/**
 * @brief Advances parser cursor and returns previous token.
 */
const lexer::Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

/**
 * @brief Returns true when current token is EOF sentinel.
 */
bool Parser::isAtEnd() const {
    return peek().getType() == lexer::TokenType::END_OF_FILE;
}

/**
 * @brief Gets current token without consuming it.
 */
const lexer::Token& Parser::peek() const {
    return tokens[current];
}

/**
 * @brief Gets most recently consumed token.
 */
const lexer::Token& Parser::previous() const {
    return tokens[current - 1];
}

/**
 * @brief Consumes required token kind or raises parse error.
 */
const lexer::Token& Parser::consume(lexer::TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

/**
 * @brief Panic-mode recovery after parse error.
 *
 * Skips tokens until likely declaration/statement boundary is found.
 */
void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().getType() == lexer::TokenType::SEMICOLON) return;

        switch (peek().getType()) {
            case lexer::TokenType::CLASS:
            case lexer::TokenType::FUN:
            case lexer::TokenType::VAR:
            case lexer::TokenType::FOR:
            case lexer::TokenType::IF:
            case lexer::TokenType::WHILE:
            case lexer::TokenType::PRINT:
            case lexer::TokenType::RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}

/**
 * @brief Emits parse diagnostic and returns marker error value.
 */
Parser::ParseError Parser::error(const lexer::Token& token, const std::string& message) {
    hadParseError = true;
    std::string location = token.getType() == lexer::TokenType::END_OF_FILE ? "end" : token.getLexeme();
    if (::litecode::structuredDiagnosticsEnabled()) {
        std::cerr << ::litecode::formatDiagnostic(::litecode::DiagnosticStage::Parse,
                                                 token.getLine(),
                                                 location,
                                                 message)
                  << '\n';
    } else {
        std::cerr << "[Line " << token.getLine() << "] Error";
        if (token.getType() == lexer::TokenType::END_OF_FILE) {
            std::cerr << " at end";
        } else {
            std::cerr << " at '" << token.getLexeme() << "'";
        }
        std::cerr << ": " << message << '\n';
    }
    return ParseError{};
}

}  // namespace parser
