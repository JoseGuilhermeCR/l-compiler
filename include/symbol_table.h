#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include "token.h"

#include <stdint.h>

struct symbol
{
    char lexeme[MAX_LEXEME_SIZE + 1];
    enum token token;
    struct symbol *next;
};

struct symbol_table
{
    struct symbol *symbols;
    uint32_t capacity;
};

int
symbol_table_create(struct symbol_table *table, uint32_t capacity);

struct symbol *
symbol_table_search(struct symbol_table *table, const char *lexeme);

int
symbol_table_insert(struct symbol_table *table,
                    const char *lexeme,
                    enum token token);

void
symbol_table_populate_with_keywords(struct symbol_table *table);

void
symbol_table_destroy(struct symbol_table *table);

#endif