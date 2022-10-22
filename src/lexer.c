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

#include "lexer.h"

#include "symbol_table.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
lexeme_append_or_error(struct lexeme *l, char c)
{
    l->buffer[l->size++] = c;
    assert(l->size <= sizeof(l->buffer) && "Lexeme buffer overflow.");
}

static int
is_in_alphabet(char c)
{
    if (isalnum((unsigned char)c) || c == EOF)
        return 1;
    const char *extra = " _.,;:()[]{}+*-'/|@&%!?><=\"\n\r";
    return strchr(extra, c) != NULL;
}

void
lexer_init(struct lexer *lexer,
           const struct file *file,
           struct symbol_table *table)
{
    lexer->file = file;
    lexer->symbol_table = table;
    lexer->error = LEXER_ERROR_NONE;
    lexer->cursor = 0;
    lexer->line = 1;
}

enum lexer_result
lexer_get_next_token(struct lexer *lexer, struct lexical_entry *entry)
{
    if (lexer->cursor >= lexer->file->size)
        return LEXER_RESULT_EMPTY;

    memset(&entry->lexeme, 0, sizeof(entry->lexeme));

    lexer->state = LEXER_STATE_INITIAL;
    while (lexer->cursor != lexer->file->size) {
        const char c = lexer->file->buffer[lexer->cursor++];

        if (c == '\n')
            ++lexer->line;

        if (!is_in_alphabet(c)) {
            lexer->error = LEXER_ERROR_INVALID_CHARACTER;
            return LEXER_RESULT_ERROR;
        }

        switch (lexer->state) {
            case LEXER_STATE_INITIAL:
                memset(&lexer->lexeme, 0, sizeof(lexer->lexeme));

                if (c == ' ' || c == '\n')
                    break;

                lexeme_append_or_error(&lexer->lexeme, c);

                if (isalpha(c) || c == '_') {
                    lexer->state = LEXER_STATE_KEYWORD_OR_IDENTIFIER;
                    break;
                } else if (isdigit(c) && c != '0') {
                    // 0 is not treated here, because 0 might be the start
                    // of a hexadecimal character.
                    lexer->state = LEXER_STATE_INTEGER_OR_FLOAT;
                    break;
                }

                switch (c) {
                    case '&':
                        lexer->state = LEXER_STATE_LOGICAL_AND;
                        break;
                    case '|':
                        lexer->state = LEXER_STATE_LOGICAL_OR;
                        break;
                    case '!':
                        lexer->state = LEXER_STATE_NOT_OR_NOT_EQUAL;
                        break;
                    case ':':
                        lexer->state = LEXER_STATE_ASSIGNMENT;
                        break;
                    case '=':
                        entry->token = TOKEN_EQUAL;
                        return LEXER_RESULT_FOUND;
                    case ',':
                        entry->token = TOKEN_COMMA;
                        return LEXER_RESULT_FOUND;
                    case '+':
                        entry->token = TOKEN_PLUS;
                        return LEXER_RESULT_FOUND;
                    case '-':
                        entry->token = TOKEN_MINUS;
                        return LEXER_RESULT_FOUND;
                    case '*':
                        entry->token = TOKEN_TIMES;
                        return LEXER_RESULT_FOUND;
                    case ';':
                        entry->token = TOKEN_SEMICOLON;
                        return LEXER_RESULT_FOUND;
                    case '(':
                        entry->token = TOKEN_OPENING_PAREN;
                        return LEXER_RESULT_FOUND;
                    case ')':
                        entry->token = TOKEN_CLOSING_PAREN;
                        return LEXER_RESULT_FOUND;
                    case '[':
                        entry->token = TOKEN_OPENING_SQUARE_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case ']':
                        entry->token = TOKEN_CLOSING_SQUARE_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '{':
                        entry->token = TOKEN_OPENING_CURLY_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '}':
                        entry->token = TOKEN_CLOSING_CURLY_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '<':
                        lexer->state = LEXER_STATE_LESS_OR_LESS_EQUAL;
                        break;
                    case '>':
                        lexer->state = LEXER_STATE_GREATER_OR_GREATER_EQUAL;
                        break;
                    case '/':
                        lexer->state = LEXER_STATE_DIVISION_OR_COMENTARY;
                        break;
                    case '\'':
                        lexer->state = LEXER_STATE_ENTER_CHAR_CONSTANT;
                        break;
                    case '"':
                        lexer->state = LEXER_STATE_STRING_CONSTANT;
                        break;
                    case '0':
                        lexer->state = LEXER_STATE_CHAR_OR_NUMBER;
                        break;
                    case '.':
                        lexer->state = LEXER_STATE_START_FLOAT;
                        break;
                }

                break;
            case LEXER_STATE_KEYWORD_OR_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;

                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));

                    const struct symbol *s = symbol_table_search(
                        lexer->symbol_table, lexer->lexeme.buffer);
                    if (s) {
                        entry->token = s->token;
                        return LEXER_RESULT_FOUND;
                    }

                    const int error = symbol_table_insert(lexer->symbol_table,
                                                          lexer->lexeme.buffer,
                                                          TOKEN_IDENTIFIER);
                    assert(error == 0 &&
                           "symbol_table_insert cannot fail at this point.");

                    entry->token = TOKEN_IDENTIFIER;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_LOGICAL_AND:
                if (c == '&') {
                    entry->token = TOKEN_LOGICAL_AND;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LOGICAL_OR:
                if (c == '|') {
                    entry->token = TOKEN_LOGICAL_OR;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_NOT_OR_NOT_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_NOT_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_NOT;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_ASSIGNMENT:
                if (c == '=') {
                    entry->token = TOKEN_ASSIGNMENT;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LESS_OR_LESS_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_LESS_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_LESS;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_GREATER_OR_GREATER_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_GREATER_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_GREATER;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_DIVISION_OR_COMENTARY:
                if (c != '*') {
                    --lexer->cursor;
                    entry->token = TOKEN_DIVISION;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->state = LEXER_STATE_IN_COMMENTARY;
                }
                break;
            case LEXER_STATE_IN_COMMENTARY:
                if (c == '*')
                    lexer->state = LEXER_STATE_LEAVING_COMMENTARY;
                break;
            case LEXER_STATE_LEAVING_COMMENTARY:
                if (c == '/')
                    lexer->state = LEXER_STATE_INITIAL;
                else if (c != '*')
                    lexer->state = LEXER_STATE_IN_COMMENTARY;
                break;
            case LEXER_STATE_ENTER_CHAR_CONSTANT:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isprint(c)) {
                    lexer->state = LEXER_STATE_LEAVE_CHAR_CONSTANT;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LEAVE_CHAR_CONSTANT:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (c == '\'') {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_STRING_CONSTANT:
                if (c == '\n') {
                    // Don't consider a \n inside a string...
                    // revert the line number and cause an error.
                    --lexer->line;
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }

                lexeme_append_or_error(&lexer->lexeme, c);
                if (c == '"') {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_CHAR_OR_NUMBER:
                if (tolower(c) == 'x') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_ENTER_HEXADECIMAL_CHAR;
                } else if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_INTEGER_OR_FLOAT;
                } else if (c == '.') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_FLOAT;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_INTEGER;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_ENTER_HEXADECIMAL_CHAR:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isxdigit(c)) {
                    lexer->state = LEXER_STATE_HEXADECIMAL_CHAR;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_HEXADECIMAL_CHAR:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isxdigit(c)) {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                lexer->error = LEXER_ERROR_INVALID_LEXEME;
                return LEXER_RESULT_ERROR;
            case LEXER_STATE_INTEGER_OR_FLOAT:
                if (c == '.') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_FLOAT;
                } else if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_INTEGER;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_START_FLOAT:
                if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_FLOAT;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_FLOAT:
                if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_FLOAT;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
        }
    }

    if (lexer->state != LEXER_STATE_INITIAL) {
        lexer->error = LEXER_ERROR_UNEXPECTED_EOF;
        return LEXER_RESULT_ERROR;
    }
    return LEXER_RESULT_EMPTY;
}

void
lexer_print_error(const struct lexer *lexer)
{
    assert(lexer->error != LEXER_ERROR_NONE);

    fprintf(ERR_STREAM, "%i\n", lexer->line);
    switch (lexer->error) {
        case LEXER_ERROR_UNEXPECTED_EOF:
            fputs("fim de arquivo nao esperado.\n", ERR_STREAM);
            break;
        case LEXER_ERROR_INVALID_LEXEME:
            fprintf(ERR_STREAM,
                    "lexema nao identificado [%s].\n",
                    lexer->lexeme.buffer);
            break;
        case LEXER_ERROR_INVALID_CHARACTER:
            fputs("caractere invalido.\n", ERR_STREAM);
            break;
        default:
            UNREACHABLE();
    }
}

const char *
get_lexeme_from_token(enum token token)
{
    switch (token) {
        case TOKEN_LOGICAL_AND:
            return "&&";
        case TOKEN_LOGICAL_OR:
            return "||";
        case TOKEN_NOT_EQUAL:
            return "!=";
        case TOKEN_NOT:
            return "!";
        case TOKEN_ASSIGNMENT:
            return ":=";
        case TOKEN_EQUAL:
            return "=";
        case TOKEN_COMMA:
            return ",";
        case TOKEN_PLUS:
            return "+";
        case TOKEN_MINUS:
            return "-";
        case TOKEN_TIMES:
            return "*";
        case TOKEN_SEMICOLON:
            return ";";
        case TOKEN_OPENING_PAREN:
            return "(";
        case TOKEN_CLOSING_PAREN:
            return ")";
        case TOKEN_OPENING_SQUARE_BRACKET:
            return "[";
        case TOKEN_CLOSING_SQUARE_BRACKET:
            return "]";
        case TOKEN_OPENING_CURLY_BRACKET:
            return "{";
        case TOKEN_CLOSING_CURLY_BRACKET:
            return "}";
        case TOKEN_LESS:
            return "<";
        case TOKEN_LESS_EQUAL:
            return "<=";
        case TOKEN_GREATER:
            return ">";
        case TOKEN_GREATER_EQUAL:
            return ">=";
        case TOKEN_DIVISION:
            return "/";
    }

    // Tokens that may have more than
    // one lexeme never should've gotten
    // here.
    UNREACHABLE();
    return "NULL";
}