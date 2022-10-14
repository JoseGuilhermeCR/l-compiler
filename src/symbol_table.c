#include "symbol_table.h"

#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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

int
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
        strncpy(s->lexeme, lexeme, MAX_LEXEME_SIZE);
        s->token = token;
        return 0;
    }

    // Collision, go to the end of the symbols while checking if
    // any of them is already what we are trying to insert, so
    // that we don't insert any duplicates in the table.
    if (is_case_insensitive_equal(s->lexeme, lexeme))
        return -1;

    struct symbol *previous = s;
    struct symbol *next = s->next;
    while (next) {
        if (is_case_insensitive_equal(next->lexeme, lexeme))
            return -1;
        previous = next;
        next = next->next;
    }

#if !defined(VERDE)
    next = malloc(sizeof(*next));
#else
    next = (struct symbol *)malloc(sizeof(*next));
#endif
    assert(next && "failed to allocate memory for new symbol.");

    strncpy(next->lexeme, lexeme, MAX_LEXEME_SIZE);
    next->token = token;
    next->next = NULL;

    previous->next = next;
    return 0;
}

void
symbol_table_populate_with_keywords(struct symbol_table *table)
{
    assert(symbol_table_insert(table, "const", TOKEN_CONST) == 0);
    assert(symbol_table_insert(table, "int", TOKEN_INT) == 0);
    assert(symbol_table_insert(table, "char", TOKEN_CHAR) == 0);
    assert(symbol_table_insert(table, "while", TOKEN_WHILE) == 0);
    assert(symbol_table_insert(table, "if", TOKEN_IF) == 0);
    assert(symbol_table_insert(table, "float", TOKEN_FLOAT) == 0);
    assert(symbol_table_insert(table, "else", TOKEN_ELSE) == 0);
    assert(symbol_table_insert(table, "readln", TOKEN_READLN) == 0);
    assert(symbol_table_insert(table, "div", TOKEN_DIV) == 0);
    assert(symbol_table_insert(table, "string", TOKEN_STRING) == 0);
    assert(symbol_table_insert(table, "write", TOKEN_WRITE) == 0);
    assert(symbol_table_insert(table, "writeln", TOKEN_WRITELN) == 0);
    assert(symbol_table_insert(table, "mod", TOKEN_MOD) == 0);
    assert(symbol_table_insert(table, "boolean", TOKEN_BOOLEAN) == 0);
    assert(symbol_table_insert(table, "true", TOKEN_CONSTANT) == 0);
    assert(symbol_table_insert(table, "false", TOKEN_CONSTANT) == 0);
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