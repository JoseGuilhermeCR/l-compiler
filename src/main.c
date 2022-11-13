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

#include "codegen.h"
#include "file.h"
#include "lexer.h"
#include "semantic_and_syntatic.h"
#include "symbol_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                "Usage: %s <program_file> [--keep-unoptimized] [--assemble-and-link]\n",
                argv[0]);
        return -1;
    }

    uint8_t keep_unoptimized = 0;
    uint8_t assemble_and_link = 0;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--keep-unoptimized") == 0)
            keep_unoptimized = 1;
        else if (strcmp(argv[i], "--assemble-and-link") == 0)
            assemble_and_link = 1;
    }

    int status = 0;

    struct file file;
    status = read_file(&file, argv[1]);
    if (status < 0) {
        fprintf(ERR_STREAM, "Failed to read %s\n", argv[1]);
        return -1;
    }

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

    if (result == LEXER_RESULT_FOUND) {
        status = syntatic_start(&syntatic_ctx);
        if (status == 0) {
            char *filename = extract_filename(argv[1]);

            fprintf(ERR_STREAM, "Compiled lines: %u\n", lexer.line);

            if (codegen_dump(filename, keep_unoptimized, assemble_and_link) < 0) {
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
