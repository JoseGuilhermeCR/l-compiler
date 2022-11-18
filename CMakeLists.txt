cmake_minimum_required(VERSION 3.16)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(FATAL_ERROR "Release will silently disable assert calls. Please don't use it.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(l-compiler LANGUAGES C)

add_executable(l-compiler
    main.c
    symbol_table.c
    semantic_and_syntatic.c
    lexer.c
    file.c
    codegen.c
    utils.c
)

target_include_directories(l-compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIRECTORY}
)

target_compile_definitions(l-compiler PRIVATE
    "MAX_FILE_SIZE=(32U*1024U)"
    "MAX_LEXEME_SIZE=(512U)"
	ERR_STREAM=stderr
)

target_compile_options(l-compiler PRIVATE
    -Wall
    -Wextra
    -Wshadow
)
