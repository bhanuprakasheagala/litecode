cmake_minimum_required(VERSION 3.16)

project(litecode LANGUAGES CXX)
include(CTest)

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

set(INTERPRETER_SOURCES
    interpreter/src/Environment.cpp
    interpreter/src/Interpreter.cpp
    interpreter/src/LoxCallable.cpp
)

set(RESOLVER_SOURCES
    resolver/src/Resolver.cpp
)

set(BYTECODE_SOURCES
    # no cpp files yet: bytecode layer is currently compiler-only foundation
)

set(MAIN_SOURCES
    main.cpp
)

# -----------------------------
# Core library
# -----------------------------
add_library(litecode_core
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${INTERPRETER_SOURCES}
    ${RESOLVER_SOURCES}
    ${BYTECODE_SOURCES}
)

# -----------------------------
# Include paths
# -----------------------------
target_include_directories(litecode_core PUBLIC
    lexer/inc
    parser/inc
    interpreter/inc
    resolver/inc
    bytecode/inc
)

# -----------------------------
# Executable
# -----------------------------
add_executable(litecode
    ${MAIN_SOURCES}
)
target_link_libraries(litecode PRIVATE litecode_core)

if(BUILD_TESTING)
    file(GLOB TEST_CASE_DIRS RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "tests/cases/*")
    foreach(case_dir ${TEST_CASE_DIRS})
        if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${case_dir}")
            string(REPLACE "/" "." test_name "${case_dir}")
            add_test(
                NAME "${test_name}"
                COMMAND "${CMAKE_SOURCE_DIR}/tests/run_case.sh"
                        "$<TARGET_FILE:litecode>"
                        "${CMAKE_SOURCE_DIR}/${case_dir}"
            )
        endif()
    endforeach()

    option(LITECODE_ENABLE_GTEST "Enable GoogleTest unit tests" ON)
    if(LITECODE_ENABLE_GTEST)
        find_package(GTest QUIET)
        if(NOT GTest_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            )
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
        endif()

        include(GoogleTest)
        add_executable(litecode_unit_tests
            tests/unit/scanner_test.cpp
            tests/unit/parser_resolver_test.cpp
            tests/unit/interpreter_test.cpp
            tests/unit/environment_test.cpp
            tests/unit/bytecode_test.cpp
        )
        target_link_libraries(litecode_unit_tests PRIVATE litecode_core GTest::gtest_main)
        gtest_discover_tests(litecode_unit_tests)
    endif()
endif()
