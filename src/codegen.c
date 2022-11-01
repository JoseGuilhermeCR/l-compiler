#include "codegen.h"
#include "symbol_table.h"
#include "utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_VALUE_SIZE 256

/* FIXME's
 * Align the variables and constants in memory?
 * Use better instructions.
 * Separate constant data into .ro_data.
 * */

static FILE *file;

static uint64_t
get_next_address(uint64_t size)
{
    static uint64_t current_data_address;

    const uint64_t address = current_data_address;
    current_data_address += size;

    return address;
}

static uint64_t
size_from_type_or_lexeme(enum symbol_type type, uint32_t lexeme_size)
{
    switch (type) {
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            return 4;
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_LOGIC:
            return 1;
        case SYMBOL_TYPE_STRING:
            // String's lexeme is inside quotation marks. -2
            // In memory, it will have a \0 at the end. +1
            return lexeme_size - 1;
        default:
            UNREACHABLE();
    }
}

static void
dump_template(void)
{
    time_t now = time(NULL);

    struct tm tm;
    localtime_r(&now, &tm);

    fprintf(file,
            "; Generated on %04u/%02u/%02u - %02u:%02u\n"
            "global _start\n"
            "section .bss\n"
            "TMP:\n"
            "\tresb 0x10000\n"
            "UNNIT_MEM:\n"
            "section .data\n"
            "INIT_MEM:\n"
            "section .text\n"
            "_start:\n",
            tm.tm_year + 1900,
            tm.tm_mon,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min);
}

static void
add_exit_syscall(uint8_t error_code)
{
    fprintf(file,
            "section .text\n"
            "\tmov rax, 60\n"
            "\tmov rsi, %u\n"
            "\tsyscall\n",
            error_code);
}

int
codegen_init(const char *pathname)
{
    file = fopen(pathname, "w");
    if (!file)
        return -1;

    dump_template();

    return 0;
}

void
codegen_dump(void)
{
    if (!file)
        return;

    add_exit_syscall(0);
    fflush(file);
}

void
codegen_destroy(void)
{
    // TODO(Jose): Free constants.
    if (file) {
        fclose(file);
        file = NULL;
    }
}

void
codegen_write_text(const char *fmt, ...)
{
    if (!file)
        return;

    va_list args;
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);
}

uint64_t
codegen_add_unnit_value(enum symbol_type type)
{
    uint64_t size;
    if (type != SYMBOL_TYPE_STRING)
        size = size_from_type_or_lexeme(type, 0);
    else
        size = 256;

    const uint64_t address = get_next_address(size);
    fputs("section .bss\n", file);
    fprintf(file, "\tresb %lu\t; @ 0x%lx\n", size, address);

    return address;
}

uint64_t
codegen_add_value(enum symbol_type type,
                  uint8_t has_minus,
                  const char *lexeme,
                  uint32_t lexeme_size)
{
    if (type != SYMBOL_TYPE_FLOATING_POINT && type != SYMBOL_TYPE_INTEGER &&
        has_minus) {
        UNREACHABLE();
    }

    const uint64_t address =
        get_next_address(size_from_type_or_lexeme(type, lexeme_size));

    fputs("section .data\n", file);

    switch (type) {
        case SYMBOL_TYPE_LOGIC:
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_STRING:
            fputs("\tdb ", file);
            break;
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            fputs("\tdd ", file);
            break;
        default:
            UNREACHABLE();
    }

    if (has_minus)
        fputc('-', file);

    switch (type) {
        case SYMBOL_TYPE_LOGIC:
            if (is_case_insensitive_equal("true", lexeme))
                fputc('1', file);
            else
                fputc('0', file);
            break;
        case SYMBOL_TYPE_STRING:
            fprintf(file, "%s,0", lexeme);
            break;
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            fputs(lexeme, file);
            break;
        default:
            UNREACHABLE();
    }

    fprintf(file, "\t; @ 0x%lx\n", address);
    return address;
}
