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

#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include "token.h"

#include <stdint.h>

/**
 * @brief A symbol that belongs to the symbol table.
 * Each symbol may be linked in a singly linked list.
 */
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

/**
 * @brief Creates a symbol table with the specified capacity. The symbol table
 * is a hash table which handles collision by adding the entry into a singly
 * linked list. This table does *not* distinguish upper and lower case
 * characters.
 *
 * @param table Pointer to the table structure which will be created.
 * @param capacity Capacity of the table. The capacity is directly related to
 * the collision rates... A smaller capacity will undoubtely result in a higher
 * rate of collisions.
 *
 * @return 0 in case of success, -1 in case of errors.
 */
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