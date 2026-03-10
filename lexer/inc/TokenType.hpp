#ifndef TOKENTYPE_HPP
#define TOKENTYPE_HPP

namespace lexer {

    /**
     * @file TokenType.hpp
     * @brief Enumerates every lexical token kind recognized by the Litecode scanner.
     *
     * This enum is the shared vocabulary between scanner, parser, resolver, and interpreter.
     * A beginner can think of this as the "alphabet of grammar symbols" used by the parser.
     */

    /**
     * @brief Strongly-typed token categories used by the scanner and parser.
     *
     * Naming note:
     * - `NILL` is the internal token name used in this repository for the source keyword `nil`.
     * - `LEFT_PARENTH` / `RIGHT_PARENTH` are legacy internal names kept for consistency.
     */
    enum class TokenType {
        
        // Single character tokens
        LEFT_PARENTH,   // (
        RIGHT_PARENTH,  // )
        LEFT_BRACE,     // {
        RIGHT_BRACE,    // }
        COMMA,          // ,
        DOT,            // .
        MINUS,          // -
        PLUS,           // +
        SEMICOLON,      // ;
        SLASH,          // /
        STAR,           // *

        // One or two character tokens
        BANG,           // !
        BANG_EQUAL,     // !=
        EQUAL,          // =
        EQUAL_EQUAL,    // ==
        GREATER,        // >
        GREATER_EQUAL,  // >=
        LESS,           // <
        LESS_EQUAL,     // <=

        // Literals
        IDENTIFIER,     // variable or function name
        STRING,         // String literal
        NUMBER,         // Number literal

        // Keywords
        AND,
        CLASS,
        ELSE,
        FALSE,
        FUN,
        FOR,
        IF,
        NILL,
        OR,
        PRINT,
        RETURN,
        SUPER,
        THIS,
        TRUE,
        VAR,
        WHILE,

        // End of file
        END_OF_FILE // EOF marker
    };

    /**
     * @brief Converts a token kind to a stable debug string.
     * @param type Token kind to stringify.
     * @return Human-readable token identifier.
     *
     * This function is primarily used by token dump/debug output and test diagnostics.
     */
    inline const char* toString(TokenType type) {
        switch(type) {
            case TokenType::LEFT_PARENTH: return "LEFT_PARENTH";
            case TokenType::RIGHT_PARENTH: return "RIGHT_PARENTH";
            case TokenType::LEFT_BRACE: return "LEFT_BRACE";
            case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
            case TokenType::COMMA: return "COMMA";
            case TokenType::DOT: return "DOT";
            case TokenType::MINUS: return "MINUS";
            case TokenType::PLUS: return "PLUS";
            case TokenType::SEMICOLON: return "SEMICOLON";
            case TokenType::SLASH: return "SLASH";
            case TokenType::STAR: return "STAR";
            case TokenType::BANG: return "BANG";
            case TokenType::BANG_EQUAL: return "BANG_EQUAL";
            case TokenType::EQUAL: return "EQUAL";
            case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
            case TokenType::GREATER: return "GREATER";
            case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
            case TokenType::LESS: return "LESS";
            case TokenType::LESS_EQUAL: return "LESS_EQUAL";
            case TokenType::IDENTIFIER: return "IDENTIFIER";
            case TokenType::STRING: return "STRING";
            case TokenType::NUMBER: return "NUMBER";
            case TokenType::AND: return "AND";
            case TokenType::CLASS: return "CLASS";
            case TokenType::ELSE: return "ELSE";
            case TokenType::FALSE: return "FALSE";
            case TokenType::FUN: return "FUN";
            case TokenType::FOR: return "FOR";
            case TokenType::IF: return "IF";
            case TokenType::NILL: return "NILL";
            case TokenType::OR: return "OR";
            case TokenType::PRINT: return "PRINT";
            case TokenType::RETURN: return "RETURN";
            case TokenType::SUPER: return "SUPER";
            case TokenType::THIS: return "THIS";
            case TokenType::TRUE: return "TRUE";
            case TokenType::VAR: return "VAR";
            case TokenType::WHILE: return "WHILE";
            case TokenType::END_OF_FILE: return "END_OF_FILE";
            default: return "UNKNOWN";
        }
    }
}  // namespace lexer

#endif
