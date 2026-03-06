#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "lexer/inc/Scanner.hpp"
#include "lexer/inc/ErrorReporter.hpp"
#include "parser/inc/Parser.hpp"
#include "parser/inc/AstPrinter.hpp"
#include "interpreter/inc/Interpreter.hpp"
#include "resolver/inc/Resolver.hpp"

namespace lexer {

    static bool hadError = false;

    class litecode{
        public:
            litecode(int argc, char* argv[]) : argc(argc), argv(argv){}
            ~litecode(){}
            void start() {
                try{
                    if(argc > 2){
                        std::cout << "Usage: toyl [script]" << std::endl;
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

            void runPrompt(){
                try {
                    std::string inputline;
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

                        run(inputline);
                        hadError = false;
                    }
                }
                catch(const std::exception& e) {
                    std::cerr << "Error. " << e.what() << std::endl;
                }
            }

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
    lexer::litecode lc(argc, argv);
    lc.start();

    return 0;
}
