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
            "\n\tsection .text\n"
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
codegen_add_unnit_value(struct symbol *id_entry)
{
    uint64_t size;
    if (id_entry->symbol_type != SYMBOL_TYPE_STRING)
        size = size_from_type_or_lexeme(id_entry->symbol_type, 0);
    else
        size = 256;

    const uint64_t address = get_next_address(&current_bss_address, size);
    fputs("\n\tsection .bss\n", file);
    fprintf(file, "\tresb %lu\t; @ 0x%lx\n", size, address);

    id_entry->address = address;
}

void
codegen_add_value(struct symbol *id_entry,
                  uint8_t has_minus,
                  const char *lexeme,
                  uint32_t lexeme_size)
{
    if (id_entry->symbol_type != SYMBOL_TYPE_FLOATING_POINT &&
        id_entry->symbol_type != SYMBOL_TYPE_INTEGER && has_minus) {
        UNREACHABLE();
    }

    const uint64_t size =
        size_from_type_or_lexeme(id_entry->symbol_type, lexeme_size);

    uint64_t *addr_counter;
    const char *section_name;
    if (id_entry->symbol_class == SYMBOL_CLASS_VAR) {
        addr_counter = &current_data_address;
        section_name = ".data";
    } else if (id_entry->symbol_class == SYMBOL_CLASS_CONST) {
        addr_counter = &current_rodata_address;
        section_name = ".rodata";
    } else {
        UNREACHABLE();
    }

    const uint64_t address = get_next_address(addr_counter, size);

    fprintf(file, "\n\tsection %s\n", section_name);

    switch (id_entry->symbol_type) {
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

    switch (id_entry->symbol_type) {
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

    id_entry->address = address;
}
