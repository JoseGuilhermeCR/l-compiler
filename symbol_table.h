/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include "token.h"

#include <stdint.h>
#include <stdio.h>

enum symbol_class
{
    SYMBOL_CLASS_NONE,
    SYMBOL_CLASS_VAR,
    SYMBOL_CLASS_CONST
};

enum symbol_type
{
    SYMBOL_TYPE_NONE,
    SYMBOL_TYPE_INTEGER,
    SYMBOL_TYPE_LOGIC,
    SYMBOL_TYPE_STRING,
    SYMBOL_TYPE_FLOATING_POINT,
    SYMBOL_TYPE_CHAR
};

enum symbol_section
{
    SYMBOL_SECTION_NONE,
    SYMBOL_SECTION_DATA,
    SYMBOL_SECTION_BSS,
    SYMBOL_SECTION_RODATA,
};

/**
 * @brief A symbol that belongs to the symbol table.
 * Each symbol may be linked in a singly linked list.
 */
struct symbol
{
    char lexeme[MAX_LEXEME_SIZE + 1];
    enum token token;
    enum symbol_class symbol_class;
    enum symbol_type symbol_type;
    enum symbol_section symbol_section;
    uint64_t address;
    uint64_t size;
    struct symbol *next;
};

struct symbol_table
{
    struct symbol *symbols;
    uint32_t capacity;
};

/*
 * Creates a symbol table with the specified capacity. The symbol table
 * is a hash table which handles collision by adding the entry into a singly
 * linked list. This table does *not* distinguish upper and lower case
 * characters.
 *
 * table is Pointer to the table structure which will be created.
 *
 * capacity is Capacity of the table. The capacity is directly related to
 * the collision rates... A smaller capacity will undoubtely result in a higher
 * rate of collisions.
 *
 * */
int
symbol_table_create(struct symbol_table *table, uint32_t capacity);

/*
 * Searches for a symbol with the specific lexeme in the table.
 * If a symbol is found, returns a pointer to it, otherwise returns NULL.
 * */
struct symbol *
symbol_table_search(struct symbol_table *table, const char *lexeme);

/*
 * Inserts a symbol in the table and returns a pointer to it.
 * If the symbol could not be inserted, returns NULL.
 * */
struct symbol *
symbol_table_insert(struct symbol_table *table,
                    const char *lexeme,
                    enum token token);

/*
 * Perform insertion of l's language keywords into the symbol table.
 * */
void
symbol_table_populate_with_keywords(struct symbol_table *table);

/*
 * Destroy the table and deallocated memory used by it.
 * */
void
symbol_table_destroy(struct symbol_table *table);

/*
 * Prints every non-default entry in the table to file.
 * */
void
symbol_table_dump_to(struct symbol_table *table, FILE *file);

#endif
