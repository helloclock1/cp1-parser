cmake_minimum_required(VERSION 3.31)
project(MyParserProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(parser_lib src/formatter.cpp src/parser.cpp src/tokenizer.cpp)

target_include_directories(parser_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(beautify apps/main.cpp)

target_link_libraries(beautify PRIVATE parser_lib)
