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

static uint64_t current_bss_tmp_address;
static uint64_t current_data_address;
static uint64_t current_bss_address;
static uint64_t current_rodata_address;

static uint64_t
get_next_address(uint64_t *address, uint64_t size)
{
    const uint64_t tmp = *address;
    *address += size;
    return tmp;
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
            "\tglobal _start\n"
            "\n\tsection .bss\n"
            "TMP:\n"
            "\tresb 0x10000\n"
            "UNNIT_MEM:\n"
            "\n\tsection .data\n"
            "INIT_MEM:\n"
            "\n\tsection .rodata\n"
            "CONST_MEM:\n"
            "\n\tsection .text\n"
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
            "\n\tsection .text ; Exit syscall.\n"
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

void
codegen_add_unnit_value(enum symbol_type type, struct codegen_value_info *info)
{
    if (type != SYMBOL_TYPE_STRING)
        info->size = size_from_type_or_lexeme(type, 0);
    else
        info->size = 256;

    info->address = get_next_address(&current_bss_address, info->size);
    fputs("\n\tsection .bss\n", file);
    fprintf(file, "\tresb %lu\t; @ 0x%lx\n", info->size, info->address);

    info->section = SYMBOL_SECTION_BSS;
}

void
codegen_add_value(enum symbol_type type,
                  enum symbol_class class,
                  uint8_t has_minus,
                  const char *lexeme,
                  uint32_t lexeme_size,
                  struct codegen_value_info *info)
{
    if (type != SYMBOL_TYPE_FLOATING_POINT && type != SYMBOL_TYPE_INTEGER &&
        has_minus) {
        UNREACHABLE();
    }

    info->size = size_from_type_or_lexeme(type, lexeme_size);

    uint64_t *addr_counter;
    const char *section_name;
    if (class == SYMBOL_CLASS_VAR) {
        info->section = SYMBOL_SECTION_DATA;
        addr_counter = &current_data_address;
        section_name = ".data";
    } else if (class == SYMBOL_CLASS_CONST) {
        info->section = SYMBOL_SECTION_RODATA;
        addr_counter = &current_rodata_address;
        section_name = ".rodata";
    } else {
        UNREACHABLE();
    }

    info->address = get_next_address(addr_counter, info->size);

    fprintf(file, "\n\tsection %s\n", section_name);

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

    fprintf(file, "\t; @ 0x%lx\n", info->address);
}

void
codegen_add_tmp(enum symbol_type type,
                const char *lexeme,
                uint32_t lexeme_size,
                struct codegen_value_info *info)
{
    // We can't move strings / floating points from registers to memory,
    // declare them in memory.
    if (type == SYMBOL_TYPE_STRING || type == SYMBOL_TYPE_FLOATING_POINT) {
        const uint8_t has_minus = 0;
        codegen_add_value(
            type, SYMBOL_CLASS_CONST, has_minus, lexeme, lexeme_size, info);
        return;
    }

    // Otherwise, move them to a register and then into memory.
    info->section = SYMBOL_SECTION_NONE;
    info->size = size_from_type_or_lexeme(type, 0);
    info->address = get_next_address(&current_bss_tmp_address, info->size);

    fputs("\n\tsection .text ; Moving constant to f.\n", file);

    if (type != SYMBOL_TYPE_LOGIC) {
        fprintf(file, "\tmov rax, %s\n", lexeme);
    } else {
        if (is_case_insensitive_equal("true", lexeme)) {
            fputs("\tmov rax, 1\n", file);
        } else if (is_case_insensitive_equal("false", lexeme)) {
            fputs("\tmov rax, 0\n", file);
        } else {
            UNREACHABLE();
        }
    }

    fprintf(file, "\tmov [TMP + %lu], rax\n", info->address);
}

void
codegen_negate_f(struct codegen_value_info *f)
{
    const uint64_t original_address = f->address;

    // Generate a new temporary address.
    f->address = get_next_address(&current_bss_tmp_address, f->size);

    fputs("\n\tsection .text ; Negating f.\n", file);

    // Move value from f1 to rax.
    fprintf(file, "\tmov rax, [TMP + %lu]\n", original_address);

    // Negate it. (Remember the instructions we use are limited.)
    fputs("\tneg rax\n", file);
    fputs("\tadd rax, 1\n", file);

    // Move value from rax to f.
    fprintf(file, "\tmov [TMP + %lu], rax\n", f->address);
}
