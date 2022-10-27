/*
 * This is free and unencumbered software released into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * Jose Guilherme de Castro Rodrigues - 2022 - 651201
 */

#ifndef LEXER_H_
#define LEXER_H_

#include "file.h"
#include "token.h"

#include <stdint.h>

enum lexer_state
{
    LEXER_STATE_INITIAL,
    LEXER_STATE_KEYWORD_OR_IDENTIFIER,    /* 1 */
    LEXER_STATE_LOGICAL_AND,              /* 3 */
    LEXER_STATE_LOGICAL_OR,               /* 4 */
    LEXER_STATE_NOT_OR_NOT_EQUAL,         /* 5 */
    LEXER_STATE_ASSIGNMENT,               /* 6 */
    LEXER_STATE_LESS_OR_LESS_EQUAL,       /* 7 */
    LEXER_STATE_GREATER_OR_GREATER_EQUAL, /* 8 */
    LEXER_STATE_DIVISION_OR_COMENTARY,    /* 9 */
    LEXER_STATE_IN_COMMENTARY,            /* 10 */
    LEXER_STATE_LEAVING_COMMENTARY,       /* 11 */
    LEXER_STATE_ENTER_CHAR_CONSTANT,      /* 14 */
    LEXER_STATE_LEAVE_CHAR_CONSTANT,      /* 16 */
    LEXER_STATE_STRING_CONSTANT,          /* 13 */
    LEXER_STATE_CHAR_OR_NUMBER,           /* 17 */
    LEXER_STATE_ENTER_HEXADECIMAL_CHAR,   /* 18 */
    LEXER_STATE_HEXADECIMAL_CHAR,         /* 19 */
    LEXER_STATE_INTEGER_OR_FLOAT,         /* 12 */
    LEXER_STATE_START_FLOAT,              /* 15 */
    LEXER_STATE_FLOAT                     /* 20 */
};

enum lexer_result
{
    LEXER_RESULT_FOUND,
    LEXER_RESULT_EMPTY,
    LEXER_RESULT_ERROR
};

enum lexer_error
{
    LEXER_ERROR_NONE,
    LEXER_ERROR_UNEXPECTED_EOF,
    LEXER_ERROR_INVALID_LEXEME,
    LEXER_ERROR_INVALID_CHARACTER
};

enum constant_type
{
    CONSTANT_TYPE_BOOLEAN,
    CONSTANT_TYPE_CHAR,
    CONSTANT_TYPE_STRING,
    CONSTANT_TYPE_INTEGER,
    CONSTANT_TYPE_FLOAT
};

struct lexeme
{
    char buffer[MAX_LEXEME_SIZE + 1];
    uint32_t size;
};

struct lexer
{
    const struct file *file;
    struct symbol_table *symbol_table;
    enum lexer_state state;
    enum lexer_error error;
    struct lexeme lexeme;
    uint32_t cursor;
    uint32_t line;
};

struct lexical_entry
{
    enum token token;
    struct lexeme lexeme;
    enum constant_type constant_type;

    /* Used only when token = TOKEN_IDENTIFIER. */
    struct symbol *symbol_table_entry;
    uint8_t is_new_identifier;
};

void
lexer_init(struct lexer *lexer,
           const struct file *file,
           struct symbol_table *table);

enum lexer_result
lexer_get_next_token(struct lexer *lexer, struct lexical_entry *entry);

void
lexer_print_error(const struct lexer *lexer);

const char *
get_lexeme_from_token(enum token token);

#endif
