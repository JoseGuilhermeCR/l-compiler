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
    assert(size && "Size cannot be 0.");

    const uint64_t mod = *address % size;
    uint64_t not_aligned_by = 0;
    if (mod)
        not_aligned_by = size - mod;
    const uint64_t tmp = *address + not_aligned_by;
    *address += size;
    return tmp;
}

static uint64_t
size_from_type(enum symbol_type type)
{
    switch (type) {
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            return 4;
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_LOGIC:
            return 1;
        case SYMBOL_TYPE_STRING:
            return 256;
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
    info->size = size_from_type(type);

    info->address = get_next_address(&current_bss_address, info->size);
    fputs("\n\tsection .bss ; codegen_add_unnit_value.\n", file);
    fprintf(file, "\talignb %lu\n", info->size);
    fprintf(file, "\tresb %lu\t; @ 0x%lx\n", info->size, info->address);

    info->section = SYMBOL_SECTION_BSS;
}

void
codegen_add_value(enum symbol_type type,
                  enum symbol_class class,
                  uint8_t has_minus,
                  const char *lexeme,
                  struct codegen_value_info *info)
{
    if (type != SYMBOL_TYPE_FLOATING_POINT && type != SYMBOL_TYPE_INTEGER &&
        has_minus) {
        UNREACHABLE();
    }

    info->size = size_from_type(type);

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

    fprintf(file, "\n\tsection %s ; codegen_add_value.\n", section_name);
    fprintf(file, "\talign %lu\n", info->size);

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

    if (type == SYMBOL_TYPE_STRING) {
        // Reserve enough space for the unnitialized portion of the string.
        // At this point, we've only reserved space for the portion that we
        // explicitly initialized with "db".

        // strlen(lexeme)
        // -2 because of ""
        // +1 because of \0
        const uint64_t already_reserved_size = strlen(lexeme) - 1;
        const uint64_t to_reserve = info->size - already_reserved_size;
        fprintf(file, "\ttimes %lu db 0\n", to_reserve);
    }
}

void
codegen_add_tmp(enum symbol_type type,
                const char *lexeme,
                struct codegen_value_info *info)
{
    // We can't move strings / floating points from registers to memory,
    // declare them in memory.
    if (type == SYMBOL_TYPE_STRING || type == SYMBOL_TYPE_FLOATING_POINT) {
        const uint8_t has_minus = 0;
        codegen_add_value(type, SYMBOL_CLASS_CONST, has_minus, lexeme, info);
        return;
    }

    // Otherwise, move them to a register and then into memory.
    info->section = SYMBOL_SECTION_NONE;
    info->size = size_from_type(type);
    info->address = get_next_address(&current_bss_tmp_address, info->size);

    fputs("\n\tsection .text ; codegen_add_tmp.\n", file);

    const char *reg;
    if (type == SYMBOL_TYPE_INTEGER)
        reg = "eax";
    else if (type == SYMBOL_TYPE_CHAR || type == SYMBOL_TYPE_LOGIC)
        reg = "al";
    else
        UNREACHABLE();

    if (type != SYMBOL_TYPE_LOGIC) {
        fprintf(file, "\tmov %s, %s\n", reg, lexeme);
    } else {
        if (is_case_insensitive_equal("true", lexeme)) {
            fputs("\tmov al, 1\n", file);
        } else if (is_case_insensitive_equal("false", lexeme)) {
            fputs("\tmov al, 0\n", file);
        } else {
            UNREACHABLE();
        }
    }

    fprintf(file, "\tmov [TMP + %lu], %s\n", info->address, reg);
}

static const char *
label_from_section(enum symbol_section section)
{
    switch (section) {
        case SYMBOL_SECTION_NONE:
            return "TMP";
        case SYMBOL_SECTION_BSS:
            return "UNNIT_MEM";
        case SYMBOL_SECTION_DATA:
            return "INIT_MEM";
        case SYMBOL_SECTION_RODATA:
            return "CONST_MEM";
        default:
            UNREACHABLE();
    }
}

void
codegen_logic_negate(struct codegen_value_info *f)
{
    assert(f->type == SYMBOL_TYPE_LOGIC);

    const uint64_t original_address = f->address;
    const char *label = label_from_section(f->section);

    // Generate a new temporary address.
    f->address = get_next_address(&current_bss_tmp_address, f->size);

    fputs("\n\tsection .text ; codegen_logic_negate.\n", file);

    // Move value from f to al.
    fprintf(file, "\tmov al, [%s + %lu]\n", label, original_address);

    // Negate it. (Remember the instructions we use are limited.)
    fputs("\tneg al\n", file);
    fputs("\tadd al, 1\n", file);

    // Move value from rax to f.
    fprintf(file, "\tmov [TMP + %lu], al\n", f->address);
}

void
codegen_convert_to_floating_point(struct codegen_value_info *info)
{
    assert(info->type == SYMBOL_TYPE_INTEGER);

    const uint64_t original_address = info->address;
    const char *label = label_from_section(info->section);

    // Update value information.
    info->section = SYMBOL_SECTION_NONE;
    info->type = SYMBOL_TYPE_FLOATING_POINT;
    info->address = get_next_address(&current_bss_tmp_address, info->size);

    fputs("\n\tsection .text ; codegen_convert_to_floating_point.\n", file);

    // Move value from info to eax.
    fprintf(file, "\tmov eax, [%s + %lu]\n", label, original_address);

    // Extend it's sign.
    fputs("\tcdqe\n", file);

    // Convert the value from rax and place it in xmm0.
    fputs("\tcvtsi2ss xmm0, rax\n", file);

    // Place it into the newly generated address.
    fprintf(file, "\tmovss [TMP + %lu], xmm0\n", info->address);
}

static void
perform_addition_or_subtraction(const char *instr,
                                struct codegen_value_info *exps_info,
                                struct codegen_value_info *t_info)
{
    assert(exps_info->type == t_info->type);

    const uint64_t original_address = exps_info->address;
    const char *exps_label = label_from_section(exps_info->section);
    const char *t_label = label_from_section(t_info->section);

    exps_info->section = SYMBOL_SECTION_NONE;
    exps_info->address =
        get_next_address(&current_bss_tmp_address, exps_info->size);

    fputs("\n\tsection .text ; perform_addition_or_subtraction.\n", file);

    if (exps_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(
            file, "\tmovss xmm0, [%s + %lu]\n", exps_label, original_address);
        fprintf(file, "\tmovss xmm1, [%s + %lu]\n", t_label, t_info->address);
        fprintf(file, "\t%sss xmm0, xmm1\n", instr);
        fprintf(file, "\tmovss [TMP + %lu], xmm0\n", exps_info->address);
    } else if (exps_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(file, "\tmov eax, [%s + %lu]\n", exps_label, original_address);
        fprintf(file, "\tmov ebx, [%s + %lu]\n", t_label, t_info->address);
        fprintf(file, "\t%s eax, ebx\n", instr);
        fprintf(file, "\tmov [TMP + %lu], eax\n", exps_info->address);
    } else {
        UNREACHABLE();
    }
}

void
codegen_perform_addition(struct codegen_value_info *exps_info,
                         struct codegen_value_info *t_info)
{
    perform_addition_or_subtraction("add", exps_info, t_info);
}

void
codegen_perform_subtraction(struct codegen_value_info *exps_info,
                            struct codegen_value_info *t_info)
{
    perform_addition_or_subtraction("sub", exps_info, t_info);
}

void
codegen_perform_logical_or(struct codegen_value_info *exps_info,
                           struct codegen_value_info *t_info)
{
    assert(exps_info->type == t_info->type);
}
