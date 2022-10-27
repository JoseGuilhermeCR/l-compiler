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

#include "symbol_table.h"

#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void
symbol_init(struct symbol *s, const char *lexeme, enum token token)
{
    strncpy(s->lexeme, lexeme, MAX_LEXEME_SIZE);
    s->token = token;
    s->symbol_class = SYMBOL_CLASS_NONE;
    s->symbol_type = SYMBOL_TYPE_NONE;
    s->next = NULL;
}

static uint8_t
is_case_insensitive_equal(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *rhs != '\0') {
        if (tolower(*lhs) != tolower(*rhs))
            return 0;

        ++lhs;
        ++rhs;
    }

    return *lhs == '\0' && *rhs == '\0';
}

static uint32_t
hash_symbol_lexeme(const char *lexeme)
{
    // TODO(Jose): Make a better hashing function.
    uint32_t sum = 0;
    while (*lexeme) {
        sum += tolower(*lexeme);
        ++lexeme;
    }
    return sum;
}

int
symbol_table_create(struct symbol_table *table, uint32_t capacity)
{
    table->capacity = capacity;
    table->symbols = calloc(capacity, sizeof(*table->symbols));
    if (!table->symbols)
        return -1;
    return 0;
}

struct symbol *
symbol_table_search(struct symbol_table *table, const char *lexeme)
{
    assert(strlen(lexeme) <= MAX_LEXEME_SIZE &&
           "lexeme bigger than MAX_LEXEME_SIZE");

    const uint32_t idx = hash_symbol_lexeme(lexeme) % table->capacity;

    struct symbol *s = &table->symbols[idx];
    if (s->lexeme[0] == '\0')
        return NULL;

    while (s) {
        if (is_case_insensitive_equal(s->lexeme, lexeme))
            return s;
        s = s->next;
    }

    return NULL;
}

struct symbol *
symbol_table_insert(struct symbol_table *table,
                    const char *lexeme,
                    enum token token)
{
    assert(strlen(lexeme) <= MAX_LEXEME_SIZE &&
           "lexeme bigger than MAX_LEXEME_SIZE");

    const uint32_t idx = hash_symbol_lexeme(lexeme) % table->capacity;

    struct symbol *s = &table->symbols[idx];
    // Empty entry, great!
    if (s->lexeme[0] == '\0') {
        symbol_init(s, lexeme, token);
        return s;
    }

    // Collision, go to the end of the symbols while checking if
    // any of them is already what we are trying to insert, so
    // that we don't insert any duplicates in the table.
    if (is_case_insensitive_equal(s->lexeme, lexeme))
        return NULL;

    struct symbol *previous = s;
    struct symbol *next = s->next;
    while (next) {
        if (is_case_insensitive_equal(next->lexeme, lexeme))
            return NULL;
        previous = next;
        next = next->next;
    }

    next = malloc(sizeof(*next));
    assert(next && "failed to allocate memory for new symbol.");

    symbol_init(next, lexeme, token);

    previous->next = next;
    return next;
}

void
symbol_table_populate_with_keywords(struct symbol_table *table)
{
    assert(symbol_table_insert(table, "const", TOKEN_CONST));
    assert(symbol_table_insert(table, "int", TOKEN_INT));
    assert(symbol_table_insert(table, "char", TOKEN_CHAR));
    assert(symbol_table_insert(table, "while", TOKEN_WHILE));
    assert(symbol_table_insert(table, "if", TOKEN_IF));
    assert(symbol_table_insert(table, "float", TOKEN_FLOAT));
    assert(symbol_table_insert(table, "else", TOKEN_ELSE));
    assert(symbol_table_insert(table, "readln", TOKEN_READLN));
    assert(symbol_table_insert(table, "div", TOKEN_DIV));
    assert(symbol_table_insert(table, "string", TOKEN_STRING));
    assert(symbol_table_insert(table, "write", TOKEN_WRITE));
    assert(symbol_table_insert(table, "writeln", TOKEN_WRITELN));
    assert(symbol_table_insert(table, "mod", TOKEN_MOD));
    assert(symbol_table_insert(table, "boolean", TOKEN_BOOLEAN));
    assert(symbol_table_insert(table, "true", TOKEN_CONSTANT));
    assert(symbol_table_insert(table, "false", TOKEN_CONSTANT));
}

void
symbol_table_destroy(struct symbol_table *table)
{
    for (uint32_t i = 0; i < table->capacity; ++i) {
        struct symbol *s = &table->symbols[i];
        if (s->lexeme[0] == '\0')
            continue;

        s = s->next;
        while (s) {
            struct symbol *tmp = s->next;
            free(s);
            s = tmp;
        }
    }

    free(table->symbols);
    table->symbols = NULL;
    table->capacity = 0;
}
