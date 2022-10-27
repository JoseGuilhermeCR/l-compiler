cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(l-compiler LANGUAGES C)

add_executable(l-compiler
	"src/main.c"
	"src/symbol_table.c"
	"src/semantic_and_syntatic.c"
	"src/lexer.c"
	"src/file.c"
	"include/symbol_table.h"
	"include/semantic_and_syntatic.h"
    "include/lexer.h"
	"include/file.h"
	"include/token.h"
	"include/utils.h"
)

target_include_directories(l-compiler PRIVATE "include/")

target_compile_definitions(l-compiler PRIVATE
	"MAX_FILE_SIZE=(32U*1024U)"
	"MAX_LEXEME_SIZE=(32U)"
	"ERR_STREAM=stderr"
)

if (MSVC)
    target_compile_definitions(l-compiler PRIVATE "_CRT_SECURE_NO_WARNINGS")
    target_compile_options(l-compiler PRIVATE /W4)
else()
    target_compile_options(l-compiler PRIVATE -Wall -Wextra)
endif()