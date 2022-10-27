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

#include "semantic_and_syntatic.h"

#include "utils.h"
#include "symbol_table.h"

#include <stdio.h>

static void
semantic_apply_sr1(struct lexical_entry *entry, enum token tok)
{
    switch (tok) {
        case TOKEN_INT:
            entry->symbol_table_entry->symbol_type = SYMBOL_TYPE_INTEGER;
            break;
        case TOKEN_FLOAT:
            entry->symbol_table_entry->symbol_type = SYMBOL_TYPE_FLOATING_POINT;
            break;
        case TOKEN_STRING:
            entry->symbol_table_entry->symbol_type = SYMBOL_TYPE_STRING;
            break;
        case TOKEN_BOOLEAN:
            entry->symbol_table_entry->symbol_type = SYMBOL_TYPE_LOGIC;
            break;
        case TOKEN_CHAR:
            entry->symbol_table_entry->symbol_type = SYMBOL_TYPE_CHAR;
            break;
        default:
            break;
    }

    entry->symbol_table_entry->symbol_class = SYMBOL_CLASS_VAR;
}

static void
syntatic_report_unexpected_token_error(struct syntatic_ctx *ctx)
{
    fprintf(ERR_STREAM, "%i\n", ctx->lexer->line);
    if (ctx->entry->lexeme.size) {
        fprintf(ERR_STREAM,
                "token nao esperado [%s].\n",
                ctx->entry->lexeme.buffer);
    } else {
        fprintf(ERR_STREAM,
                "token nao esperado [%s].\n",
                get_lexeme_from_token(ctx->entry->token));
    }
}

static void
syntatic_report_unexpected_eof_error(struct syntatic_ctx *ctx)
{
    fprintf(ERR_STREAM, "%i\n", ctx->lexer->line);
    fputs("fim de arquivo nao esperado.\n", ERR_STREAM);
}

static enum syntatic_result
syntatic_match_token(struct syntatic_ctx *ctx, enum token token)
{
    if (ctx->found_last_token) {
        syntatic_report_unexpected_eof_error(ctx);
        return SYNTATIC_ERROR;
    }

    if (ctx->entry->token == token) {
        enum lexer_result result = lexer_get_next_token(ctx->lexer, ctx->entry);
        if (result == LEXER_RESULT_ERROR) {
            lexer_print_error(ctx->lexer);
            return SYNTATIC_ERROR;
        } else if (result == LEXER_RESULT_EMPTY) {
            ctx->found_last_token = 1;
        }

        return SYNTATIC_OK;
    }

    // TODO(Jose): Print a syntatic error.
    syntatic_report_unexpected_token_error(ctx);
    return SYNTATIC_ERROR;
}

#define MATCH_OR_ERROR(ctx_ptr, tok)                                           \
    do {                                                                       \
        enum syntatic_result _sr = syntatic_match_token((ctx_ptr), tok);       \
        if (_sr != SYNTATIC_OK)                                                \
            return -1;                                                         \
    } while (0)

static int
syntatic_decl_var(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

    if (ctx->entry->token == TOKEN_ASSIGNMENT) {
        MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);

        if (ctx->entry->token == TOKEN_MINUS) {
            MATCH_OR_ERROR(ctx, TOKEN_MINUS);
            MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
        } else {
            MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
        }
    }

    if (ctx->entry->token == TOKEN_SEMICOLON) {
        MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
        return 0;
    } else if (ctx->entry->token == TOKEN_COMMA) {
        while (ctx->entry->token == TOKEN_COMMA) {
            MATCH_OR_ERROR(ctx, TOKEN_COMMA);
            MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

            // Handle possible assignment for variable.
            if (ctx->entry->token == TOKEN_ASSIGNMENT) {
                MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);

                if (ctx->entry->token == TOKEN_MINUS) {
                    MATCH_OR_ERROR(ctx, TOKEN_MINUS);
                    MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
                } else {
                    MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
                }
            }
        }

        MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
        return 0;
    }

    syntatic_report_unexpected_token_error(ctx);
    return -1;
}

static int
syntatic_decl_const(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);
    MATCH_OR_ERROR(ctx, TOKEN_EQUAL);

    if (ctx->entry->token == TOKEN_MINUS) {
        MATCH_OR_ERROR(ctx, TOKEN_MINUS);
        MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
    } else {
        MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
    }

    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_read(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);
    MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_is_first_of_f(struct syntatic_ctx *ctx)
{
    switch (ctx->entry->token) {
        case TOKEN_NOT:
        case TOKEN_OPENING_PAREN:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_CONSTANT:
        case TOKEN_IDENTIFIER:
            return 1;
        default:
            return 0;
    }
}

static int
syntatic_exp(struct syntatic_ctx *ctx);

static int
syntatic_f(struct syntatic_ctx *ctx)
{
    if (ctx->found_last_token) {
        syntatic_report_unexpected_eof_error(ctx);
        return -1;
    }

    if (!syntatic_is_first_of_f(ctx)) {
        syntatic_report_unexpected_token_error(ctx);
        return -1;
    }

    enum token tok = ctx->entry->token;
    MATCH_OR_ERROR(ctx, tok);
    switch (tok) {
        case TOKEN_NOT:
            if (syntatic_f(ctx) < 0)
                return -1;
            break;
        case TOKEN_OPENING_PAREN:
            if (syntatic_exp(ctx) < 0)
                return -1;
            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
            break;
        case TOKEN_INT:
        case TOKEN_FLOAT:
            MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
            if (syntatic_exp(ctx) < 0)
                return -1;
            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
            break;
        case TOKEN_CONSTANT:
            break;
        case TOKEN_IDENTIFIER:
            if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
                MATCH_OR_ERROR(ctx, TOKEN_OPENING_SQUARE_BRACKET);
                if (syntatic_exp(ctx) < 0)
                    return -1;
                MATCH_OR_ERROR(ctx, TOKEN_CLOSING_SQUARE_BRACKET);
            }
            break;
        default:
            UNREACHABLE();
    }

    return 0;
}

static int
syntatic_t(struct syntatic_ctx *ctx)
{
    if (syntatic_f(ctx) < 0)
        return -1;

    enum token tok = ctx->entry->token;
    while (tok == TOKEN_TIMES || tok == TOKEN_LOGICAL_AND || tok == TOKEN_MOD ||
           tok == TOKEN_DIV || tok == TOKEN_DIVISION) {

        MATCH_OR_ERROR(ctx, tok);
        if (syntatic_f(ctx) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    return 0;
}

static int
syntatic_exps(struct syntatic_ctx *ctx)
{
    enum token tok = ctx->entry->token;
    if (tok == TOKEN_MINUS || tok == TOKEN_PLUS) {
        MATCH_OR_ERROR(ctx, tok);
    }

    if (syntatic_t(ctx) < 0)
        return -1;

    tok = ctx->entry->token;
    while (tok == TOKEN_PLUS || tok == TOKEN_MINUS || tok == TOKEN_LOGICAL_OR) {
        MATCH_OR_ERROR(ctx, tok);

        if (syntatic_t(ctx) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    return 0;
}

static int
syntatic_exp(struct syntatic_ctx *ctx)
{
    if (syntatic_exps(ctx) < 0)
        return -1;

    switch (ctx->entry->token) {
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
            MATCH_OR_ERROR(ctx, ctx->entry->token);
            if (syntatic_exps(ctx) < 0)
                return -1;
            break;
        default:
            break;
    }

    return 0;
}

static int
syntatic_write(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
    if (syntatic_exp(ctx) < 0)
        return -1;
    while (ctx->entry->token == TOKEN_COMMA) {
        MATCH_OR_ERROR(ctx, TOKEN_COMMA);
        if (syntatic_exp(ctx) < 0)
            return -1;
    }

    MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_attr(struct syntatic_ctx *ctx)
{
    if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
        MATCH_OR_ERROR(ctx, TOKEN_OPENING_SQUARE_BRACKET);
        if (syntatic_exp(ctx) < 0)
            return -1;
        MATCH_OR_ERROR(ctx, TOKEN_CLOSING_SQUARE_BRACKET);
    }

    MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);
    if (syntatic_exp(ctx) < 0)
        return -1;
    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_paren_exp(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
    if (syntatic_exp(ctx) < 0)
        return -1;
    MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
    return 0;
}

static int
syntatic_is_first_of_command(struct syntatic_ctx *ctx)
{
    switch (ctx->entry->token) {
        case TOKEN_SEMICOLON:
        case TOKEN_IDENTIFIER:
        case TOKEN_WHILE:
        case TOKEN_IF:
        case TOKEN_READLN:
        case TOKEN_WRITE:
        case TOKEN_WRITELN:
            return 1;
        default:
            return 0;
    }
}

static int
syntatic_while(struct syntatic_ctx *ctx);

static int
syntatic_if(struct syntatic_ctx *ctx);

static int
syntatic_command(struct syntatic_ctx *ctx)
{
    enum token tok = ctx->entry->token;
    MATCH_OR_ERROR(ctx, tok);

    switch (tok) {
        case TOKEN_SEMICOLON:
            break;
        case TOKEN_WRITE:
        case TOKEN_WRITELN:
            if (syntatic_write(ctx) < 0)
                return -1;
            break;
        case TOKEN_READLN:
            if (syntatic_read(ctx) < 0)
                return -1;
            break;
        case TOKEN_IDENTIFIER:
            if (syntatic_attr(ctx) < 0)
                return -1;
            break;
        case TOKEN_WHILE:
            if (syntatic_while(ctx) < 0)
                return -1;
            break;
        case TOKEN_IF:
            if (syntatic_if(ctx) < 0)
                return -1;
            break;
        default:
            UNREACHABLE();
    }

    return 0;
}

static int
syntatic_while(struct syntatic_ctx *ctx)
{
    if (syntatic_paren_exp(ctx) < 0)
        return -1;

    if (syntatic_is_first_of_command(ctx)) {
        if (syntatic_command(ctx) < 0)
            return -1;
        return 0;
    } else if (ctx->entry->token == TOKEN_OPENING_CURLY_BRACKET) {
        MATCH_OR_ERROR(ctx, TOKEN_OPENING_CURLY_BRACKET);

        while (syntatic_is_first_of_command(ctx)) {
            if (syntatic_command(ctx) < 0)
                return -1;
        }

        MATCH_OR_ERROR(ctx, TOKEN_CLOSING_CURLY_BRACKET);
        return 0;
    }

    syntatic_report_unexpected_token_error(ctx);
    return -1;
}

static int
syntatic_if(struct syntatic_ctx *ctx)
{
    if (syntatic_paren_exp(ctx) < 0)
        return -1;

    if (syntatic_is_first_of_command(ctx)) {
        if (syntatic_command(ctx) < 0)
            return -1;

        if (ctx->entry->token == TOKEN_ELSE) {
            MATCH_OR_ERROR(ctx, TOKEN_ELSE);
            if (syntatic_command(ctx) < 0)
                return -1;
        }

        return 0;
    } else if (ctx->entry->token == TOKEN_OPENING_CURLY_BRACKET) {
        MATCH_OR_ERROR(ctx, TOKEN_OPENING_CURLY_BRACKET);

        while (syntatic_is_first_of_command(ctx)) {
            if (syntatic_command(ctx) < 0)
                return -1;
        }

        MATCH_OR_ERROR(ctx, TOKEN_CLOSING_CURLY_BRACKET);

        if (ctx->entry->token == TOKEN_ELSE) {
            MATCH_OR_ERROR(ctx, TOKEN_ELSE);
            MATCH_OR_ERROR(ctx, TOKEN_OPENING_CURLY_BRACKET);

            while (syntatic_is_first_of_command(ctx)) {
                if (syntatic_command(ctx) < 0)
                    return -1;
            }

            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_CURLY_BRACKET);
        }

        return 0;
    }

    syntatic_report_unexpected_token_error(ctx);
    return 1;
}

static int
syntatic_is_first_of_s(struct syntatic_ctx *ctx)
{
    if (syntatic_is_first_of_command(ctx))
        return 1;

    switch (ctx->entry->token) {
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_BOOLEAN:
        case TOKEN_CHAR:
        case TOKEN_CONST:
            return 1;
        default:
            return 0;
    }
}

void
syntatic_init(struct syntatic_ctx *ctx,
              struct lexer *lexer,
              struct lexical_entry *entry)
{
    ctx->lexer = lexer;
    ctx->entry = entry;
    ctx->found_last_token = 0;
}

int
syntatic_start(struct syntatic_ctx *ctx)
{
    while (!ctx->found_last_token && syntatic_is_first_of_s(ctx)) {
        if (syntatic_is_first_of_command(ctx)) {
            if (syntatic_command(ctx) < 0)
                return -1;
        } else {
            // Handle DECL_VAR and DECL_CONST.
            enum token tok = ctx->entry->token;
            MATCH_OR_ERROR(ctx, tok);

            // S.R.: 4 (TODO)

            switch (tok) {
                case TOKEN_INT:
                case TOKEN_FLOAT:
                case TOKEN_STRING:
                case TOKEN_BOOLEAN:
                case TOKEN_CHAR:
                    // S.R.: 1
                    semantic_apply_sr1(ctx->entry, tok);
                    if (syntatic_decl_var(ctx) < 0)
                        return -1;
                    break;
                case TOKEN_CONST:
                    if (syntatic_decl_const(ctx) < 0)
                        return -1;
                    break;
                default:
                    syntatic_report_unexpected_token_error(ctx);
                    break;
            }
        }
    }

    if (!ctx->found_last_token) {
        syntatic_report_unexpected_token_error(ctx);
        return -1;
    }

    return 0;
}
