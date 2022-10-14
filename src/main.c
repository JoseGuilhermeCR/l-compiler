#include "file.h"
#include "lexer.h"
#include "semantic_and_syntatic.h"
#include "symbol_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static struct file file;
static struct lexer lexer;
static struct symbol_table table;

static void
cleanup(void)
{
    symbol_table_destroy(&table);
    destroy_stdin_file(&file);
}

int
main(void)
{
#if defined(WAIT_ATTACH)
    static volatile uint8_t _waiting_for_debug = 1;

    while (_waiting_for_debug)
        ;
#endif

    assert(atexit(cleanup) == 0);

    assert(read_file_from_stdin(&file, MAX_FILE_SIZE) == 0);

    assert(symbol_table_create(&table, 64) == 0);
    symbol_table_populate_with_keywords(&table);

    lexer_init(&lexer, &file, &table);

    struct lexical_entry entry;
    enum lexer_result result = lexer_get_next_token(&lexer, &entry);
    if (result == LEXER_RESULT_FOUND) {
        struct syntatic_ctx syntatic_ctx;
        syntatic_init(&syntatic_ctx, &lexer, &entry);
        if (syntatic_start(&syntatic_ctx) == 0) {
            fprintf(ERR_STREAM, "%i linhas compiladas.\n", lexer.line);
        }
    } else {
        lexer_print_error(&lexer);
    }

    return 0;
}
