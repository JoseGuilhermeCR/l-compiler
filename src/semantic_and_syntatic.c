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

#include "lexer.h"
#include "symbol_table.h"
#include "token.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>

static uint8_t
is_st_arithmetic(enum symbol_type type)
{
    return type == SYMBOL_TYPE_INTEGER || type == SYMBOL_TYPE_FLOATING_POINT;
}

static int
semantic_apply_sr26(enum token operation_tok,
                    enum symbol_type *exp_type,
                    enum symbol_type t_type)
{
    const uint8_t any_string =
        *exp_type == SYMBOL_TYPE_STRING || t_type == SYMBOL_TYPE_STRING;
    const uint8_t are_types_equal = *exp_type == t_type;
    const uint8_t is_arithmetic =
        is_st_arithmetic(*exp_type) && is_st_arithmetic(t_type);

    switch (operation_tok) {
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
            if (!is_arithmetic && !are_types_equal) {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
            if (any_string || (!is_arithmetic && !are_types_equal)) {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        default:
            UNREACHABLE();
    }

    // TODO(Jose): Maybe remove this from here,
    // but the resulting expression will always
    // be of boolean type.
    *exp_type = SYMBOL_TYPE_LOGIC;
    return 0;
}

static int
semantic_apply_sr25(enum token operation_tok,
                    enum symbol_type *exps_type,
                    enum symbol_type t_type)
{
    const uint8_t is_arithmetic =
        is_st_arithmetic(*exps_type) && is_st_arithmetic(t_type);

    switch (operation_tok) {
        case TOKEN_PLUS:
            if (is_arithmetic) {
                // If one of the operands is not a float, implicitly convert it.
                if (t_type == SYMBOL_TYPE_FLOATING_POINT) {
                    *exps_type = SYMBOL_TYPE_FLOATING_POINT;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }

            break;
        case TOKEN_MINUS:
            if (is_arithmetic) {
                // If one of the operands is not a float, implicitly convert it.
                if (t_type == SYMBOL_TYPE_FLOATING_POINT) {
                    *exps_type = SYMBOL_TYPE_FLOATING_POINT;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_LOGICAL_OR:
            if (*exps_type != SYMBOL_TYPE_LOGIC ||
                t_type != SYMBOL_TYPE_LOGIC) {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        default:
            UNREACHABLE();
    }

    return 0;
}

static int
semantic_apply_sr24(enum token operation_tok,
                    enum symbol_type *t_type,
                    enum symbol_type f_type)
{
    const uint8_t is_arithmetic =
        is_st_arithmetic(*t_type) && is_st_arithmetic(f_type);

    switch (operation_tok) {
        case TOKEN_TIMES:
            if (is_arithmetic) {
                // If one of the operands is not a float, implicitly convert it.
                if (f_type == SYMBOL_TYPE_FLOATING_POINT) {
                    *t_type = SYMBOL_TYPE_FLOATING_POINT;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_LOGICAL_AND:
            if (*t_type != SYMBOL_TYPE_LOGIC || f_type != SYMBOL_TYPE_LOGIC) {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_MOD:
            if (is_arithmetic) {
                // "mod" can only be used between integers.
                if (*t_type != SYMBOL_TYPE_INTEGER ||
                    f_type != SYMBOL_TYPE_INTEGER) {
                    fputs("Tipo incompativeis", ERR_STREAM);
                    return -1;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_DIV:
            if (is_arithmetic) {
                // "div" is used for integer division... if any of the operands
                // is a floating point... we have a type error.
                if (*t_type != SYMBOL_TYPE_INTEGER ||
                    f_type != SYMBOL_TYPE_INTEGER) {
                    fputs("Tipos incompativeis.", ERR_STREAM);
                    return -1;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        case TOKEN_DIVISION:
            if (is_arithmetic) {
                // If one of the operands is not a float, implicitly convert it.
                if (f_type == SYMBOL_TYPE_FLOATING_POINT) {
                    *t_type = SYMBOL_TYPE_FLOATING_POINT;
                }
            } else {
                fputs("Tipos incompativeis", ERR_STREAM);
                return -1;
            }
            break;
        default:
            UNREACHABLE();
    }

    return 0;
}

static void
semantic_apply_sr23(enum symbol_type *t_type, enum symbol_type f_type)
{
    *t_type = f_type;
}

static void
semantic_apply_sr22(enum symbol_type *exps_type, enum symbol_type t_type)
{
    *exps_type = t_type;
}

static int
semantic_apply_sr21(enum symbol_type t_type, uint8_t had_signal)
{
    if (!had_signal)
        return 0;

    if (t_type != SYMBOL_TYPE_INTEGER && t_type != SYMBOL_TYPE_FLOATING_POINT)
        return -1;

    return 0;
}

static void
semantic_apply_sr20(enum symbol_type *exp_type, enum symbol_type exps_type)
{
    *exp_type = exps_type;
}

static void
semantic_apply_sr19(enum symbol_type *type,
                    struct symbol *symbol,
                    uint8_t had_brackets)
{
    *type = had_brackets ? SYMBOL_TYPE_CHAR : symbol->symbol_type;
}

static void
semantic_apply_sr18(enum symbol_type *type, enum constant_type const_type)
{
    switch (const_type) {
        case CONSTANT_TYPE_CHAR:
            *type = SYMBOL_TYPE_CHAR;
            break;
        case CONSTANT_TYPE_INTEGER:
            *type = SYMBOL_TYPE_INTEGER;
            break;
        case CONSTANT_TYPE_FLOAT:
            *type = SYMBOL_TYPE_FLOATING_POINT;
            break;
        case CONSTANT_TYPE_STRING:
            *type = SYMBOL_TYPE_STRING;
            break;
        case CONSTANT_TYPE_BOOLEAN:
            *type = SYMBOL_TYPE_LOGIC;
            break;
    }
}

static void
semantic_apply_sr17(enum symbol_type *type)
{
    *type = SYMBOL_TYPE_FLOATING_POINT;
}

static void
semantic_apply_sr16(enum symbol_type *type)
{
    *type = SYMBOL_TYPE_INTEGER;
}

static void
semantic_apply_sr15(enum symbol_type *f_type, enum symbol_type exp_type)
{
    *f_type = exp_type;
}

static int
semantic_apply_sr14(enum symbol_type type)
{
    if (type != SYMBOL_TYPE_LOGIC) {
        fputs("Tipo incompatíveis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr13(enum symbol_type exp_type)
{
    if (exp_type != SYMBOL_TYPE_INTEGER) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr12(enum symbol_type exp_type)
{
    if (exp_type != SYMBOL_TYPE_FLOATING_POINT) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr11(enum symbol_type exp_type)
{
    if (exp_type != SYMBOL_TYPE_LOGIC) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr10(struct symbol *symbol)
{
    assert(symbol &&
           "A NULL symbol means this was probably not an IDENTIFIER.");

    if (symbol->symbol_class != SYMBOL_CLASS_VAR) {
        fputs("Classe incompativel", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr9(struct symbol *id_entry, enum symbol_type exp_type)
{
    assert(id_entry &&
            "A NULL symbol means this was probably not an IDENTIFIER.");
    if (id_entry->symbol_type != exp_type) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr8(uint8_t is_new_identifier)
{
    if (is_new_identifier) {
        fputs("Identificador nao declarado", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr7(enum symbol_type exp_type)
{
    if (exp_type != SYMBOL_TYPE_INTEGER) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr6(struct symbol *symbol)
{
    assert(symbol &&
           "A NULL symbol means this was probably not an IDENTIFIER.");

    if (symbol->symbol_type != SYMBOL_TYPE_STRING) {
        fputs("Tipos incompativeis", ERR_STREAM);
        return -1;
    }

    return 0;
}

static void
semantic_apply_sr5(struct symbol *symbol, enum constant_type const_type)
{
    assert(symbol &&
           "A NULL symbol means this was probably not an IDENTIFIER.");

    switch (const_type) {
        case CONSTANT_TYPE_INTEGER:
            symbol->symbol_type = SYMBOL_TYPE_INTEGER;
            break;
        case CONSTANT_TYPE_FLOAT:
            symbol->symbol_type = SYMBOL_TYPE_FLOATING_POINT;
            break;
        case CONSTANT_TYPE_STRING:
            symbol->symbol_type = SYMBOL_TYPE_STRING;
            break;
        case CONSTANT_TYPE_BOOLEAN:
            symbol->symbol_type = SYMBOL_TYPE_LOGIC;
            break;
        case CONSTANT_TYPE_CHAR:
            symbol->symbol_type = SYMBOL_TYPE_CHAR;
            break;
        default:
            break;
    }

    symbol->symbol_class = SYMBOL_CLASS_CONST;
}

static int
semantic_apply_sr4(uint8_t is_new_identifier)
{
    if (!is_new_identifier) {
        fputs("Identificador ja declarado", ERR_STREAM);
        return -1;
    }

    return 0;
}

static int
semantic_apply_sr3(struct symbol *symbol, enum constant_type ct)
{
    const enum symbol_type st = symbol->symbol_type;
    if ((st == SYMBOL_TYPE_CHAR && ct == CONSTANT_TYPE_CHAR) ||
        (st == SYMBOL_TYPE_LOGIC && ct == CONSTANT_TYPE_BOOLEAN) ||
        (st == SYMBOL_TYPE_STRING && ct == CONSTANT_TYPE_STRING) ||
        (st == SYMBOL_TYPE_FLOATING_POINT && ct == CONSTANT_TYPE_FLOAT) ||
        (st == SYMBOL_TYPE_INTEGER && ct == CONSTANT_TYPE_INTEGER)) {
        return 0;
    }

    fputs("ERRO: Tipos incompativeis...\n", ERR_STREAM);
    return -1;
}

static int
semantic_apply_sr2(struct symbol *symbol, uint8_t has_minus)
{
    assert(symbol &&
           "A NULL symbol means this was probably not an IDENTIFIER.");

    if (!has_minus)
        return 0;

    switch (symbol->symbol_type) {
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_LOGIC:
        case SYMBOL_TYPE_STRING:
            fputs("ERRO: Tipos incompativeis...\n", ERR_STREAM);
            return -1;
        default:
            return 0;
    }
}

static void
semantic_apply_sr1(struct symbol *symbol, enum token tok)
{
    assert(symbol &&
           "A NULL symbol means this was probably not an IDENTIFIER.");

    switch (tok) {
        case TOKEN_INT:
            symbol->symbol_type = SYMBOL_TYPE_INTEGER;
            break;
        case TOKEN_FLOAT:
            symbol->symbol_type = SYMBOL_TYPE_FLOATING_POINT;
            break;
        case TOKEN_STRING:
            symbol->symbol_type = SYMBOL_TYPE_STRING;
            break;
        case TOKEN_BOOLEAN:
            symbol->symbol_type = SYMBOL_TYPE_LOGIC;
            break;
        case TOKEN_CHAR:
            symbol->symbol_type = SYMBOL_TYPE_CHAR;
            break;
        default:
            break;
    }

    symbol->symbol_class = SYMBOL_CLASS_VAR;
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
syntatic_decl_var(struct syntatic_ctx *ctx, enum token type_tok)
{
    uint8_t is_new_identifier = ctx->entry->is_new_identifier;
    struct symbol *id_entry = ctx->entry->symbol_table_entry;

    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

    if (semantic_apply_sr4(is_new_identifier) < 0)
        return -1;

    semantic_apply_sr1(id_entry, type_tok);

    if (ctx->entry->token == TOKEN_ASSIGNMENT) {
        MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);

        if (ctx->entry->token == TOKEN_MINUS) {
            if (semantic_apply_sr2(id_entry, 1) < 0)
                return -1;
            MATCH_OR_ERROR(ctx, TOKEN_MINUS);
        }

        const enum constant_type const_type = ctx->entry->constant_type;

        MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);

        if (semantic_apply_sr3(id_entry, const_type) < 0)
            return -1;
    }

    if (ctx->entry->token == TOKEN_SEMICOLON) {
        MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
        return 0;
    } else if (ctx->entry->token == TOKEN_COMMA) {
        while (ctx->entry->token == TOKEN_COMMA) {
            MATCH_OR_ERROR(ctx, TOKEN_COMMA);

            id_entry = ctx->entry->symbol_table_entry;
            is_new_identifier = ctx->entry->is_new_identifier;

            MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

            if (semantic_apply_sr4(is_new_identifier) < 0)
                return -1;

            semantic_apply_sr1(id_entry, type_tok);

            // Handle possible assignment for variable.
            if (ctx->entry->token == TOKEN_ASSIGNMENT) {
                MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);

                if (ctx->entry->token == TOKEN_MINUS) {
                    if (semantic_apply_sr2(id_entry, 1) < 0)
                        return -1;
                    MATCH_OR_ERROR(ctx, TOKEN_MINUS);
                }

                const enum constant_type const_type = ctx->entry->constant_type;

                MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);

                if (semantic_apply_sr3(id_entry, const_type) < 0)
                    return -1;
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
    uint8_t has_minus = 0;
    struct symbol *id_entry = ctx->entry->symbol_table_entry;

    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);
    MATCH_OR_ERROR(ctx, TOKEN_EQUAL);

    if (ctx->entry->token == TOKEN_MINUS) {
        MATCH_OR_ERROR(ctx, TOKEN_MINUS);
        has_minus = 1;
    }

    enum constant_type const_type = ctx->entry->constant_type;

    MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);

    semantic_apply_sr5(id_entry, const_type);
    if (semantic_apply_sr2(id_entry, has_minus) < 0)
        return -1;

    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_read(struct syntatic_ctx *ctx)
{
    MATCH_OR_ERROR(ctx, TOKEN_READLN);
    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);

    const uint8_t is_new_identifier = ctx->entry->is_new_identifier;
    struct symbol *id_entry = ctx->entry->symbol_table_entry;

    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

    if (semantic_apply_sr8(is_new_identifier) < 0)
        return -1;

    if (semantic_apply_sr10(id_entry) < 0)
        return -1;

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
syntatic_exp(struct syntatic_ctx *ctx, enum symbol_type *type);

static int
syntatic_f(struct syntatic_ctx *ctx, enum symbol_type *f_type)
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
    switch (tok) {
        case TOKEN_NOT:
            MATCH_OR_ERROR(ctx, TOKEN_NOT);
            if (syntatic_f(ctx, f_type) < 0)
                return -1;
            if (semantic_apply_sr14(*f_type) < 0)
                return -1;
            break;
        case TOKEN_OPENING_PAREN: {
            enum symbol_type exp_type = SYMBOL_TYPE_NONE;
            MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
            if (syntatic_exp(ctx, &exp_type) < 0)
                return -1;
            semantic_apply_sr15(f_type, exp_type);
            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
            break;
        }
        case TOKEN_INT: {
            enum symbol_type exp_type = SYMBOL_TYPE_NONE;
            MATCH_OR_ERROR(ctx, TOKEN_INT);
            MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
            if (syntatic_exp(ctx, &exp_type) < 0)
                return -1;
            if (semantic_apply_sr12(exp_type) < 0)
                return -1;
            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
            semantic_apply_sr16(f_type);
            break;
        }
        case TOKEN_FLOAT: {
            enum symbol_type exp_type = SYMBOL_TYPE_NONE;
            MATCH_OR_ERROR(ctx, TOKEN_FLOAT);
            MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
            if (syntatic_exp(ctx, &exp_type) < 0)
                return -1;
            if (semantic_apply_sr13(exp_type) < 0)
                return -1;
            MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
            semantic_apply_sr17(f_type);
            break;
        }
        case TOKEN_CONSTANT: {
            enum constant_type const_type = ctx->entry->constant_type;
            MATCH_OR_ERROR(ctx, TOKEN_CONSTANT);
            semantic_apply_sr18(f_type, const_type);
            break;
        }
        case TOKEN_IDENTIFIER: {
            struct symbol *id_entry = ctx->entry->symbol_table_entry;
            const uint8_t is_new_identifier = ctx->entry->is_new_identifier;
            uint8_t had_brackets = 0;

            MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

            if (semantic_apply_sr8(is_new_identifier) < 0)
                return -1;

            if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
                MATCH_OR_ERROR(ctx, TOKEN_OPENING_SQUARE_BRACKET);

                had_brackets = 1;

                if (semantic_apply_sr6(id_entry) < 0)
                    return -1;

                enum symbol_type exp_type;
                if (syntatic_exp(ctx, &exp_type) < 0)
                    return -1;
                if (semantic_apply_sr7(exp_type) < 0)
                    return -1;
                MATCH_OR_ERROR(ctx, TOKEN_CLOSING_SQUARE_BRACKET);
            }

            semantic_apply_sr19(f_type, id_entry, had_brackets);
            break;
        }
        default:
            UNREACHABLE();
    }

    return 0;
}

static int
syntatic_t(struct syntatic_ctx *ctx, enum symbol_type *t_type)
{
    enum symbol_type f_type = SYMBOL_TYPE_NONE;

    if (syntatic_f(ctx, &f_type) < 0)
        return -1;

    semantic_apply_sr23(t_type, f_type);

    enum token tok = ctx->entry->token;
    while (tok == TOKEN_TIMES || tok == TOKEN_LOGICAL_AND || tok == TOKEN_MOD ||
           tok == TOKEN_DIV || tok == TOKEN_DIVISION) {

        MATCH_OR_ERROR(ctx, tok);

        if (syntatic_f(ctx, &f_type) < 0)
            return -1;

        if (semantic_apply_sr24(tok, t_type, f_type) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    return 0;
}

static int
syntatic_exps(struct syntatic_ctx *ctx, enum symbol_type *exps_type)
{
    uint8_t had_signal = 0;

    enum token tok = ctx->entry->token;
    if (tok == TOKEN_MINUS || tok == TOKEN_PLUS) {
        MATCH_OR_ERROR(ctx, tok);
        had_signal = 1;
    }

    enum symbol_type t_type = SYMBOL_TYPE_NONE;
    if (syntatic_t(ctx, &t_type) < 0)
        return -1;

    if (semantic_apply_sr21(t_type, had_signal) < 0)
        return -1;

    semantic_apply_sr22(exps_type, t_type);

    tok = ctx->entry->token;
    while (tok == TOKEN_PLUS || tok == TOKEN_MINUS || tok == TOKEN_LOGICAL_OR) {
        MATCH_OR_ERROR(ctx, tok);

        if (syntatic_t(ctx, &t_type) < 0)
            return -1;

        if (semantic_apply_sr25(tok, exps_type, t_type) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    return 0;
}

static int
syntatic_exp(struct syntatic_ctx *ctx, enum symbol_type *exp_type)
{
    enum symbol_type exps_type = SYMBOL_TYPE_NONE;
    if (syntatic_exps(ctx, &exps_type) < 0)
        return -1;

    semantic_apply_sr20(exp_type, exps_type);

    switch (ctx->entry->token) {
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL: {
            enum token operation_tok = ctx->entry->token;
            MATCH_OR_ERROR(ctx, ctx->entry->token);
            if (syntatic_exps(ctx, &exps_type) < 0)
                return -1;
            if (semantic_apply_sr26(operation_tok, exp_type, exps_type) < 0)
                return -1;
            break;
        }
        default:
            break;
    }

    return 0;
}

static int
syntatic_write(struct syntatic_ctx *ctx)
{
    enum symbol_type type = SYMBOL_TYPE_NONE;

    MATCH_OR_ERROR(ctx, ctx->entry->token);

    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
    if (syntatic_exp(ctx, &type) < 0)
        return -1;
    while (ctx->entry->token == TOKEN_COMMA) {
        MATCH_OR_ERROR(ctx, TOKEN_COMMA);
        if (syntatic_exp(ctx, &type) < 0)
            return -1;
    }

    MATCH_OR_ERROR(ctx, TOKEN_CLOSING_PAREN);
    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_attr(struct syntatic_ctx *ctx)
{
    enum symbol_type type = SYMBOL_TYPE_NONE;
    const uint8_t is_new_identifier = ctx->entry->is_new_identifier;
    struct symbol *id_entry = ctx->entry->symbol_table_entry;

    MATCH_OR_ERROR(ctx, TOKEN_IDENTIFIER);

    if (semantic_apply_sr8(is_new_identifier) < 0)
        return -1;

    if (semantic_apply_sr10(id_entry) < 0)
        return -1;

    if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
        MATCH_OR_ERROR(ctx, TOKEN_OPENING_SQUARE_BRACKET);

        if (semantic_apply_sr6(id_entry) < 0)
            return -1;

        if (syntatic_exp(ctx, &type) < 0)
            return -1;
        MATCH_OR_ERROR(ctx, TOKEN_CLOSING_SQUARE_BRACKET);
    }

    MATCH_OR_ERROR(ctx, TOKEN_ASSIGNMENT);
    if (syntatic_exp(ctx, &type) < 0)
        return -1;
    if (semantic_apply_sr9(id_entry, type) < 0)
        return -1;
    MATCH_OR_ERROR(ctx, TOKEN_SEMICOLON);
    return 0;
}

static int
syntatic_paren_exp(struct syntatic_ctx *ctx)
{
    enum symbol_type exp_type = SYMBOL_TYPE_NONE;
    MATCH_OR_ERROR(ctx, TOKEN_OPENING_PAREN);
    if (syntatic_exp(ctx, &exp_type) < 0)
        return -1;
    if (semantic_apply_sr11(exp_type) < 0)
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
    MATCH_OR_ERROR(ctx, TOKEN_WHILE);

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
    MATCH_OR_ERROR(ctx, TOKEN_IF);

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
                    if (syntatic_decl_var(ctx, tok) < 0)
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
