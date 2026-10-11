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
#include "interpreter/inc/HostConfig.hpp"
#include "interpreter/inc/Interpreter.hpp"
#include "resolver/inc/Resolver.hpp"
#include "bytecode/inc/Compiler.hpp"
#include "bytecode/inc/VM.hpp"

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
            litecode(int argc, char* argv[]) : argc(argc), argv(argv), replBatchLimit(replMaxBatches()) {
                options = ::litecode::parseCliArguments(argc, argv);
                if (options.showHelp) {
                    std::cout << ::litecode::usageText();
                }
                if (options.showVersion) {
                    std::cout << ::litecode::versionText();
                }
                if (options.hasError) {
                    std::cerr << options.errorMessage << std::endl;
                    std::cerr << ::litecode::usageText();
                    exit(64);
                }
            }
            ~litecode(){}
            /**
             * @brief Selects file or interactive mode and contains unexpected host exceptions.
             *
             * Argument errors exit during construction; this method handles only
             * successfully parsed options and delegates mode-specific status policy.
             */
            void start() {
                try{
                    if (options.showHelp || options.showVersion) {
                        return;
                    }
                    if (!options.scriptPath.empty()) {
                        runFile(options.scriptPath); // File mode
                    }
                    else {
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
            ::litecode::CliOptions options;
            interpreter::Interpreter interpreter;
            std::vector<std::vector<parser::StmtPtr>> programBatches;
            size_t replBatchLimit;

            /**
             * @brief Reinitializes interpreter and drops retained REPL AST batches.
             *
             * Runtime functions retain non-owning pointers into their declarations,
             * so clearing the batches and replacing the interpreter together ends
             * the old session without leaving callable objects pointing at dead ASTs.
             */
            void resetReplState(bool announce = true) {
                interpreter = ::interpreter::Interpreter();
                programBatches.clear();
                if (announce) {
                    std::cout << "State reset." << std::endl;
                }
            }

            /**
             * @brief Chooses the configured successful-input count before automatic reset.
             *
             * A nonzero CLI value takes precedence; otherwise the environment helper
             * supplies the fallback. Zero disables this reset policy.
             *
             * @return Number of successful inputs before reset, or 0 when disabled.
             */
            static size_t replAutoResetThreshold() {
                size_t envValue = ::litecode::parsePositiveSizeEnv("LITECODE_REPL_AUTO_RESET_EVERY", 0, 1024);
                if (envValue != 0) {
                    return envValue;
                }
                return 0;
            }

            /**
             * @brief Reads the environment-configured retained-AST batch limit.
             *
             * Reaching the limit causes a session reset before the next tree-walk
             * batch is retained, bounding AST memory while preserving valid closure
             * declaration pointers within the active session.
             * @return Maximum number of kept program batches.
             */
            static size_t replMaxBatches() {
                size_t envValue = ::litecode::parsePositiveSizeEnv("LITECODE_REPL_MAX_BATCHES", 32, 256);
                return envValue;
            }

            /** @brief Prints the command text shared with the REPL help formatter. */
            static void printReplHelp() {
                std::cout << ::litecode::replHelpText();
            }

            /**
             * @brief Reads and executes a script, mapping front-end/runtime failures to process exit codes.
             * @param path Script file path.
             *
             * File-open/read failures use the input-error status; scan, parse,
             * resolve, and runtime failures use the script-error status.
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

                    run(content, options.useBytecode);

                    if(hadError)
                        exit(65);
                }
                catch(const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    exit(66);
                }
            }

            /**
             * @brief Reads one source batch per prompt and preserves tree-walk session state.
             *
             * Supports:
             * - `.reset` command to clear session state.
             * - optional auto-reset via `LITECODE_REPL_AUTO_RESET_EVERY`.
             *
             * Only successful executions advance the automatic-reset counter; a
             * failed batch leaves the counter unchanged and does not terminate the REPL.
             */
            void runPrompt(){
                try {
                    std::string inputline;
                    size_t successfulRuns = 0;
                    const size_t autoResetEvery = options.replAutoResetEvery > 0 ? options.replAutoResetEvery : replAutoResetThreshold();
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
                        if (inputline == ".help") {
                            printReplHelp();
                            hadError = false;
                            continue;
                        }
                        if (inputline == ".reset") {
                            resetReplState();
                            successfulRuns = 0;
                            hadError = false;
                            continue;
                        }
                        if (inputline == ".quit") {
                            break;
                        }

                        run(inputline, options.useBytecode);
                        if (!hadError) {
                            ++successfulRuns;
                            if (autoResetEvery > 0 && successfulRuns >= autoResetEvery) {
                                resetReplState();
                                successfulRuns = 0;
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
             * @brief Runs one source batch through scanning, parsing, then a selected backend.
             * @param inputSource Raw Litecode source text.
             * @param vmMode Selects the experimental compiler/VM path after parsing.
             *
             * Tree-walk mode resolves and executes against the persistent interpreter,
             * retaining the AST so closure declarations stay alive. VM mode compiles
             * and executes immediately with a fresh VM, and currently bypasses the
             * resolver and the tree-walk batch-retention path.
             */
            void run(const std::string& inputSource, bool vmMode = false) {
                ErrorReporter::reset();

                // 1. Lexing
                Scanner scanner(inputSource);
                std::vector<Token> tokens = scanner.scanTokens();
                if (ErrorReporter::hadError()) {
                    hadError = true;
                    return;
                }

                const bool dumpTokens = options.dumpTokens || (std::getenv("LITECODE_DUMP_TOKENS") != nullptr && std::string(std::getenv("LITECODE_DUMP_TOKENS")) != "0");
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

                if (vmMode) {
                    // The VM is an alternate backend over the shared AST. It does
                    // not use the resolver or the persistent tree-walk interpreter;
                    // keep this boundary explicit until both backends share a
                    // resolved intermediate representation and equivalent state model.
                    bytecode::Compiler compiler;
                    bytecode::Chunk chunk = compiler.compileProgram(statements);
                    std::ostringstream vmOut;
                    bytecode::VirtualMachine vm;
                    bytecode::InterpretStatus status = vm.interpret(chunk, vmOut);
                    std::cout << vmOut.str();
                    hadError = status != bytecode::InterpretStatus::Ok;
                    return;
                }

                // Keep AST batches alive across REPL iterations so function/method
                // declarations can safely retain pointers into statement trees.
                // However, unbounded retention can cause long-session memory growth.
                // Reset the session when the retention window is exceeded to keep the
                // REPL safe while preserving the expected stateful semantics.
                if (programBatches.size() >= replBatchLimit) {
                    resetReplState(false);
                    std::cout << "State reset (batch limit reached)." << std::endl;
                }

                programBatches.push_back(std::move(statements));
                auto& currentBatch = programBatches.back();

                const bool dumpAst = options.dumpAst || (std::getenv("LITECODE_DUMP_AST") != nullptr && std::string(std::getenv("LITECODE_DUMP_AST")) != "0");
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
    * Process boundary: construct the CLI host, then let it select and run the
    * requested mode. Script-specific exit codes are produced inside the host.
     */
    lexer::litecode lc(argc, argv);
    lc.start();

    return 0;
}
