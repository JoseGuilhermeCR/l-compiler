cmake_minimum_required(VERSION 3.14)

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
	"include/utils.h")

target_include_directories(l-compiler PRIVATE "include/")

target_compile_definitions(l-compiler PRIVATE
	"_CRT_SECURE_NO_WARNINGS"
	"MAX_FILE_SIZE=(32U*1024U)"
	"MAX_LEXEME_SIZE=(32U)"
	"ERR_STREAM=stderr")