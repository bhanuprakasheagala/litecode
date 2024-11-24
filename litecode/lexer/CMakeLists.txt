cmake_minimum_required(VERSION 3.10)
project(ToyLangInterpreter)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the directories for the include files and source files
include_directories(${CMAKE_SOURCE_DIR}/lexer/inc)

# Specify the source files for the project
set(SOURCE_FILES
    lexer/src/main.cpp
    lexer/src/Scanner.cpp
)

# Create the executable
add_executable(toylang ${SOURCE_FILES})

# Link necessary libraries (if any required later)
# target_link_libraries(toylang some_library)
