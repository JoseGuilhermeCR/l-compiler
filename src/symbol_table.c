/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#include "symbol_table.h"

#include "token.h"
#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *
symbol_class_to_str(enum symbol_class sc)
{
    switch (sc) {
        case SYMBOL_CLASS_NONE:
            return "NONE";
        case SYMBOL_CLASS_VAR:
            return "VAR";
        case SYMBOL_CLASS_CONST:
            return "CONST";
        default:
            UNREACHABLE();
    }
}

static const char *
symbol_type_to_str(enum symbol_type st)
{
    switch (st) {
        case SYMBOL_TYPE_NONE:
            return "NONE";
        case SYMBOL_TYPE_CHAR:
            return "CHAR";
        case SYMBOL_TYPE_INTEGER:
            return "INTEGER";
        case SYMBOL_TYPE_FLOATING_POINT:
            return "FP";
        case SYMBOL_TYPE_LOGIC:
            return "LOGIC";
        case SYMBOL_TYPE_STRING:
            return "STRING";
        default:
            UNREACHABLE();
    }
}

static const char *
symbol_section_to_str(enum symbol_section ss)
{
    switch (ss) {
        case SYMBOL_SECTION_NONE:
            return "NONE";
        case SYMBOL_SECTION_DATA:
            return "DATA";
        case SYMBOL_SECTION_BSS:
            return "BSS";
        case SYMBOL_SECTION_RODATA:
            return "RODATA";
        default:
            UNREACHABLE();
    }
}

static void
symbol_init(struct symbol *s, const char *lexeme, enum token token)
{
    memset(s, 0, sizeof(*s));

    strncpy(s->lexeme, lexeme, MAX_LEXEME_SIZE);
    s->token = token;
    s->symbol_class = SYMBOL_CLASS_NONE;
    s->symbol_type = SYMBOL_TYPE_NONE;
    s->symbol_section = SYMBOL_SECTION_NONE;
    s->next = NULL;
}

static void
symbol_print(struct symbol *s, FILE *file)
{
    fprintf(file, "@: %p\t", s);
    fprintf(file, "Section: %s\t", symbol_section_to_str(s->symbol_section));
    fprintf(file, "Address: 0x%lx\t", s->address);
    fprintf(file, "Size: %03lu\t", s->size);
    fprintf(file, "Class: %s\t", symbol_class_to_str(s->symbol_class));
    fprintf(file, "Type: %s\t", symbol_type_to_str(s->symbol_type));
    fprintf(file, "Lexeme: %s\n", s->lexeme);
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
    int err = 0;

    err += symbol_table_insert(table, "const", TOKEN_CONST) == NULL;
    err += symbol_table_insert(table, "int", TOKEN_INT) == NULL;
    err += symbol_table_insert(table, "char", TOKEN_CHAR) == NULL;
    err += symbol_table_insert(table, "while", TOKEN_WHILE) == NULL;
    err += symbol_table_insert(table, "if", TOKEN_IF) == NULL;
    err += symbol_table_insert(table, "float", TOKEN_FLOAT) == NULL;
    err += symbol_table_insert(table, "else", TOKEN_ELSE) == NULL;
    err += symbol_table_insert(table, "readln", TOKEN_READLN) == NULL;
    err += symbol_table_insert(table, "div", TOKEN_DIV) == NULL;
    err += symbol_table_insert(table, "string", TOKEN_STRING) == NULL;
    err += symbol_table_insert(table, "write", TOKEN_WRITE) == NULL;
    err += symbol_table_insert(table, "writeln", TOKEN_WRITELN) == NULL;
    err += symbol_table_insert(table, "mod", TOKEN_MOD) == NULL;
    err += symbol_table_insert(table, "boolean", TOKEN_BOOLEAN) == NULL;
    err += symbol_table_insert(table, "true", TOKEN_CONSTANT) == NULL;
    err += symbol_table_insert(table, "false", TOKEN_CONSTANT) == NULL;

    assert(!err && "symbol_table_insert failed.");
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

void
symbol_table_dump_to(struct symbol_table *table, FILE *file)
{
    for (uint32_t i = 0; i != table->capacity; ++i) {
        struct symbol *s = &table->symbols[i];

        if (s->lexeme[0] != '\0' && s->token == TOKEN_IDENTIFIER)
            symbol_print(s, file);

        while (s->next) {
            s = s->next;
            if (s->token == TOKEN_IDENTIFIER)
                symbol_print(s, file);
        }
    }
}
