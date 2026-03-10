#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <limits>
#include "lexer/inc/Scanner.hpp"
#include "lexer/inc/ErrorReporter.hpp"
#include "parser/inc/Parser.hpp"
#include "parser/inc/AstPrinter.hpp"
#include "interpreter/inc/Interpreter.hpp"
#include "resolver/inc/Resolver.hpp"

/**
 * @file main.cpp
 * @brief Command-line driver and REPL host for Litecode interpreter pipeline.
 *
 * Beginner overview:
 * - file mode: read script, run full pipeline once, exit with sysexit-style code.
 * - REPL mode: run pipeline per line and preserve runtime state across inputs.
 */

namespace lexer {

    static bool hadError = false;

    class litecode{
        public:
            /**
             * @brief Constructs driver with process arguments.
             */
            litecode(int argc, char* argv[]) : argc(argc), argv(argv){}
            ~litecode(){}
            /**
             * @brief Entrypoint for selecting file mode or REPL mode.
             */
            void start() {
                try{
                    if(argc > 2){
                        std::cout << "Usage: litecode [script]" << std::endl;
                        exit(64); /* For exit codes, I’m using the conventions defined in the UNIX “sysexits.h” header */
                    }
                    else if(argc == 2) {
                        runFile(argv[1]); // File mode
                    }
                    else{
                        runPrompt(); // Interactive mode
                    }
                }
                catch(const std::exception& e){
                    std::cerr << "Error: " << e.what() << std::endl;
                    exit(1);
                }
            }
        private:
            int argc;
            char** argv;
            interpreter::Interpreter interpreter;
            std::vector<std::vector<parser::StmtPtr>> programBatches;

            /**
             * @brief Reinitializes interpreter and drops retained REPL AST batches.
             *
             * This is used by explicit `.reset` command and optional auto-reset policy.
             */
            void resetReplState() {
                interpreter = ::interpreter::Interpreter();
                programBatches.clear();
            }

            /**
             * @brief Parses optional REPL auto-reset threshold from environment.
             * @return Number of successful inputs before reset, or 0 when disabled/invalid.
             */
            static size_t replAutoResetThreshold() {
                const char* env = std::getenv("LITECODE_REPL_AUTO_RESET_EVERY");
                if (env == nullptr || std::string(env).empty()) {
                    return 0;
                }

                try {
                    unsigned long long value = std::stoull(env);
                    if (value == 0) {
                        return 0;
                    }
                    if (value > std::numeric_limits<size_t>::max()) {
                        return std::numeric_limits<size_t>::max();
                    }
                    return static_cast<size_t>(value);
                } catch (...) {
                    return 0;
                }
            }

            /**
             * @brief Executes script from file path.
             * @param path Script file path.
             */
            void runFile(const std::string& path) {
                try {
                    std::ifstream file(path, std::ios::binary);
                    if(!file.is_open()) {
                        std::cerr << "Error: Unable to open file: " << path << std::endl;
                        exit(66);
                    }
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string content = buffer.str();

                    run(content);

                    if(hadError)
                        exit(65);
                }
                catch(const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    exit(66);
                }
            }

            /**
             * @brief Interactive prompt loop.
             *
             * Supports:
             * - `.reset` command to clear session state.
             * - optional auto-reset via `LITECODE_REPL_AUTO_RESET_EVERY`.
             */
            void runPrompt(){
                try {
                    std::string inputline;
                    size_t successfulRuns = 0;
                    const size_t autoResetEvery = replAutoResetThreshold();
                    while(true){
                        std::cout << "> ";

                        if(!std::getline(std::cin, inputline)) {
                            if(std::cin.eof())
                                break;
                            else{
                                throw std::runtime_error("Error reading input..\n");
                            }
                        }
                        if(inputline.empty()) {
                            continue;
                        }
                        if (inputline == ".reset") {
                            resetReplState();
                            std::cout << "State reset." << std::endl;
                            successfulRuns = 0;
                            hadError = false;
                            continue;
                        }

                        run(inputline);
                        if (!hadError) {
                            ++successfulRuns;
                            if (autoResetEvery > 0 && successfulRuns >= autoResetEvery) {
                                resetReplState();
                                successfulRuns = 0;
                                std::cout << "State reset." << std::endl;
                            }
                        }
                        hadError = false;
                    }
                }
                catch(const std::exception& e) {
                    std::cerr << "Error. " << e.what() << std::endl;
                }
            }

            /**
             * @brief Executes complete frontend/runtime pipeline for one source chunk.
             * @param inputSource Raw Litecode source text.
             *
             * Pipeline:
             * 1. Scanner
             * 2. Parser
             * 3. Resolver
             * 4. Interpreter
             */
            void run(const std::string& inputSource) {
                ErrorReporter::reset();

                // 1. Lexing
                Scanner scanner(inputSource);
                std::vector<Token> tokens = scanner.scanTokens();
                if (ErrorReporter::hadError()) {
                    hadError = true;
                    return;
                }

                // Optional debug dump controlled by environment variable.
                const char* dumpEnv = std::getenv("LITECODE_DUMP_TOKENS");
                const bool dumpTokens = dumpEnv != nullptr && std::string(dumpEnv) != "0";
                if (dumpTokens) {
                    for (const auto& token : tokens) {
                        std::cout << token << std::endl;
                    }
                }

                // 2. Parsing (program: declarations + statements)
                parser::Parser parser(tokens);
                std::vector<parser::StmtPtr> statements = parser.parse();
                if (parser.hadError()) {
                    hadError = true;
                    return;
                }

                // Keep AST batches alive across REPL iterations so function/method
                // declarations can safely retain pointers into statement trees.
                programBatches.push_back(std::move(statements));
                auto& currentBatch = programBatches.back();

                // Optional AST dump controlled by environment variable.
                const char* dumpAstEnv = std::getenv("LITECODE_DUMP_AST");
                const bool dumpAst = dumpAstEnv != nullptr && std::string(dumpAstEnv) != "0";
                if (dumpAst) {
                    parser::AstPrinter printer;
                    std::string output = printer.printProgram(currentBatch);
                    if (!output.empty()) {
                        std::cout << output << std::endl;
                    }
                }

                // 3. Resolution + interpretation
                resolver::Resolver resolver(interpreter);
                resolver.resolve(currentBatch);
                if (resolver.hadError()) {
                    hadError = true;
                    return;
                }

                interpreter.interpret(currentBatch);
                if (interpreter.hadRuntimeError()) {
                    hadError = true;
                }
            }

    };
}


int main(int argc, char* argv[]) {
    /**
     * Process-level entrypoint:
     * construct CLI driver and delegate all mode-specific handling to it.
     */
    lexer::litecode lc(argc, argv);
    lc.start();

    return 0;
}
