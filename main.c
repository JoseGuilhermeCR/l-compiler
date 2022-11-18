/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#include "codegen.h"
#include "file.h"
#include "lexer.h"
#include "semantic_and_syntatic.h"
#include "symbol_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Extracts a filename from pathname. The returned buffer must be freed.
 * Examples:
 *  /home/user/file.txt -> file.txt
 * ./file.txt -> file.txt
 *   file.txt -> file.txt
 * */
static char *
extract_filename(const char *pathname)
{
    char *buffer = malloc(256);
    if (!buffer)
        return NULL;

    const char *last_separator = strrchr(pathname, '/');
    if (last_separator)
        ++last_separator;
    else
        last_separator = pathname;

    char *ptr = buffer;
    while (*last_separator) {
        *ptr++ = *last_separator++;
    }

    *ptr = 0;
    return buffer;
}

int
main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(ERR_STREAM,
                "Usage: %s <program_file> [--keep-unoptimized] "
                "[--assemble-and-link]\n",
                argv[0]);
        return -1;
    }

    // Checks for optional parameters:
    // --keep-unoptimized will leave a copy of the generated assembly that
    // didn't pass through the peephole.
    // --assemble-and-link will use nasm and ld to generate an executable for
    // the program.
    uint8_t keep_unoptimized = 0;
    uint8_t assemble_and_link = 0;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--keep-unoptimized") == 0)
            keep_unoptimized = 1;
        else if (strcmp(argv[i], "--assemble-and-link") == 0)
            assemble_and_link = 1;
    }

    int status = 0;

    // Read the l's program source file.
    struct file file;
    status = read_file(&file, argv[1]);
    if (status < 0) {
        fprintf(ERR_STREAM, "Failed to read %s\n", argv[1]);
        return -1;
    }

    // Initialize compiler's main structures.
    struct symbol_table table;
    status = symbol_table_create(&table, 64);
    if (status < 0) {
        fputs("Failed to create symbol table.\n", ERR_STREAM);
        goto symbol_table_err;
    }
    symbol_table_populate_with_keywords(&table);

    struct lexer lexer;
    lexer_init(&lexer, &file, &table);

    struct syntatic_ctx syntatic_ctx;
    syntatic_init(&syntatic_ctx, &lexer);

    codegen_init();

    status = -1;
    enum lexer_result result =
        lexer_get_next_token(&lexer, &syntatic_ctx.entry);
    if (result == LEXER_RESULT_ERROR) {
        lexer_print_error(&lexer);
        goto lexer_err;
    }

    // If we've found the first token, kickstart the syntatic analyzer.
    // All the rest will be done inside it.
    if (result == LEXER_RESULT_FOUND) {
        status = syntatic_start(&syntatic_ctx);
        if (status == 0) {
            // Compilation occurred successfully!
            char *filename = extract_filename(argv[1]);

            fprintf(ERR_STREAM, "Compiled lines: %u\n", lexer.line);

            if (codegen_dump(filename, keep_unoptimized, assemble_and_link) <
                0) {
                fputs("codegen_dump failed.\n", ERR_STREAM);
                free(filename);
            }
        }
    }

    codegen_destroy();

lexer_err:
    symbol_table_destroy(&table);

symbol_table_err:
    destroy_file(&file);
    return status;
}
