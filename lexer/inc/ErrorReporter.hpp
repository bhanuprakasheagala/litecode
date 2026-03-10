#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP

#include <iostream>
#include <string>
namespace lexer{
    /**
     * @file ErrorReporter.hpp
     * @brief Minimal shared error flag/report utility used by the scanner stage.
     */

    /**
     * @brief Global lexer error state and printer.
     *
     * Design intent:
     * - scanner reports lexical diagnostics through this class,
     * - driver checks `hadError()` and decides whether to continue to parsing.
     *
     * This class is intentionally static-only to keep usage simple in early stages.
     */
    class ErrorReporter {
    public:
        /**
         * @brief Reports a formatted lexer error and flips the error flag.
         * @param line Source line of failure.
         * @param where Optional location suffix (for example " at end").
         * @param message Human-readable message.
         */
        static void report(int line, const std::string& where, const std::string& message) {
            std::cerr << "[line " << line << "] Error " << where << ": " << message << '\n';
            hadErrorFlag = true; 
        }

        /**
         * @brief Returns whether any lexer error has been reported in current run.
         * @return True when an error was reported.
         */
        static bool hadError() {
            return hadErrorFlag;
        }

        /**
         * @brief Clears lexer error state before scanning a new source input.
         */
        static void reset() {
            hadErrorFlag = false;
        }

    private:
        /**
         * @brief Process-local scanner error indicator for current driver invocation.
         */
        static inline bool hadErrorFlag = false;
    };
}  // namespace lexer
#endif
