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

            void runFile(const std::string& path) {
                try {
                    std::ifstream file(path, std::ios::binary);
                    if(!file.is_open()) {
                        throw std::ios_base::failure("Unable to open file..\n");
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
                // 1. Lexing
                Scanner scanner(inputSource);
                std::vector<Token> tokens = scanner.scanTokens();

                // Optional: keep token dump for now
                for (const auto& token : tokens) {
                    std::cout << token << std::endl;
                }

                // 2. Parsing (expression only)
                parser::Parser parser(tokens);
                parser::ExprPtr expr = parser.parse();

                if (!expr) return;

                // 3. AST Printing
                parser::AstPrinter printer;
                std::cout << printer.print(*expr) << std::endl;
            }

            static void error(int line, std::string message) {
                report(line, " ", message);
            }
            static void report(int line, std::string where, std::string message) {
                std::cout << "[line " << line << " ] Error " << where << ": " << message;
                hadError = true;
            }
    };
}


int main(int argc, char* argv[]) {
    lexer::litecode lc(argc, argv);
    lc.start();

    return 0;
}