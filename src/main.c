#include "file.h"
#include "lexer.h"
#include "semantic_and_syntatic.h"
#include "symbol_table.h"

#include <assert.h>
#include <stdio.h>

int
main(void)
{
    // static volatile uint8_t _waiting_for_debug = 1;

    // while (_waiting_for_debug)
    //     ;

    struct file file;
    assert(read_file_from_stdin(&file, MAX_FILE_SIZE) == 0);

    struct symbol_table table;
    assert(symbol_table_create(&table, 64) == 0);

    symbol_table_populate_with_keywords(&table);

    struct lexer lexer;
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

    symbol_table_destroy(&table);
    destroy_stdin_file(&file);
    return 0;
}
