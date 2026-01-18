cmake_minimum_required(VERSION 3.16)

project(litecode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Source files
# -----------------------------
set(LEXER_SOURCES
    lexer/src/Scanner.cpp
)

set(PARSER_SOURCES
    parser/src/Parser.cpp
    parser/src/AstPrinter.cpp
)

set(MAIN_SOURCES
    main.cpp
)

# -----------------------------
# Executable
# -----------------------------
add_executable(litecode
    ${MAIN_SOURCES}
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
)

# -----------------------------
# Include paths
# -----------------------------
target_include_directories(litecode PRIVATE
    lexer/inc
    parser/inc
)