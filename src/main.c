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

static struct file file;
static struct lexer lexer;
static struct symbol_table table;

static void
cleanup(void)
{
    codegen_destroy();
    symbol_table_destroy(&table);
    destroy_stdin_file(&file);
}

int
main(void)
{
    codegen_init("a.asm");

    codegen_add_constant(SYMBOL_TYPE_STRING, "\"Hello, World!\"");
    codegen_add_constant(SYMBOL_TYPE_FLOATING_POINT, "3.45");

    codegen_dump();
    codegen_destroy();

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

        int result = syntatic_start(&syntatic_ctx);
        symbol_table_dump_to(&table, stdout);

        if (result == 0) {
            fprintf(ERR_STREAM, "%i linhas compiladas.\n", lexer.line);
        }

        return result;
    } else if (result != LEXER_RESULT_EMPTY) {
        lexer_print_error(&lexer);
        return -1;
    }

    return 0;
}
