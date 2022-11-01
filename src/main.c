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
#include <sys/wait.h>
#include <unistd.h>

static struct file file;
static struct lexer lexer;
static struct symbol_table table;

static int
link_object(const char *pathname)
{
    pid_t pid = fork();

    if (pid > 0) {
        char *const ld = "ld";
        char *const args[] = { ld, pathname, NULL };

        if (execvp("ld", args) < 0) {
            perror("execvp");
            return -1;
        }
    } else if (pid == 0) {
        fputs("Waiting for linker...\n", ERR_STREAM);
        if (waitpid(-1, NULL, 0) < -1)
            perror("waitpid");
        return 0;
    }

    perror("fork");
    return pid;
}

static int
assemble(const char *pathname)
{
    pid_t pid = fork();

    if (pid > 0) {
        char *const nasm = "nasm";
        char *const args[] = { nasm, "-f", "elf64", pathname, NULL };

        if (execvp("nasm", args) < 0) {
            perror("execvp");
            return -1;
        }
    } else if (pid == 0) {
        fputs("Waiting for assembler...\n", ERR_STREAM);
        if (waitpid(-1, NULL, 0) < -1)
            perror("waitpid");
        return 0;
    }

    perror("fork");
    return pid;
}

static void
cleanup(void)
{
    codegen_destroy();
    symbol_table_destroy(&table);
    destroy_stdin_file(&file);
}

int
main(int argc, const char *argv[])
{
    const char *out_file = "l.asm";
    codegen_init(out_file);

    assert(atexit(cleanup) == 0);

    if (argc < 2) {
        fputs("Usando entrada do terminal já que nenhum arquivo foi passado.\n", ERR_STREAM);
        assert(read_file_from_stdin(&file, MAX_FILE_SIZE) == 0);
    } else {
        assert(read_file(&file, argv[1]) == 0);
    }

    assert(symbol_table_create(&table, 64) == 0);
    symbol_table_populate_with_keywords(&table);

    lexer_init(&lexer, &file, &table);

    int status = -1;

    struct lexical_entry entry;
    enum lexer_result result = lexer_get_next_token(&lexer, &entry);
    if (result == LEXER_RESULT_FOUND) {
        struct syntatic_ctx syntatic_ctx;
        syntatic_init(&syntatic_ctx, &lexer, &entry);
        status = syntatic_start(&syntatic_ctx);
    } else if (result != LEXER_RESULT_EMPTY) {
        lexer_print_error(&lexer);
    }

    if (status == 0) {
        fprintf(ERR_STREAM, "%i linhas compiladas.\n", lexer.line);

        symbol_table_dump_to(&table, stdout);

        codegen_dump();
        codegen_destroy();

        if (assemble(out_file) == 0) {
            usleep(500 * 1000);
            if (link_object("l.o") < 0) {
                fputs("linking failed.\n", ERR_STREAM);
            }
        } else {
            fputs("assemble failed.\n", ERR_STREAM);
        }
    }

    codegen_destroy();
    return status;
}
