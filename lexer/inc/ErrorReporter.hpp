#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP

#include <iostream>
#include <string>
namespace lexer{
    class ErrorReporter {
    public:
        static void report(int line, const std::string& where, const std::string& message) {
            std::cerr << "[line " << line << "] Error " << where << ": " << message << '\n';
            hadErrorFlag = true; 
        }

        static bool hadError() {
            return hadErrorFlag;
        }

        static void reset() {
            hadErrorFlag = false;
        }

    private:
        static inline bool hadErrorFlag = false;
    };
}
#endif