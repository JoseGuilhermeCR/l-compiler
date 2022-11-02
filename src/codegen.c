#include "codegen.h"
#include "symbol_table.h"
#include "token.h"
#include "utils.h"

#include <assert.h>
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
static uint64_t current_label_counter;

static void
get_next_label(char *buffer, uint32_t size)
{
    snprintf(buffer, size, "L%lu", current_label_counter++);
}

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
            "\n\tsection .text ; add_exit_syscall.\n"
            "\tmov rax, 60\n"
            "\tmov rdi, %u\n"
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
codegen_reset_tmp(void)
{
    current_bss_tmp_address = 0;
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

    fprintf(file,
            "\n\tsection .text ; codegen_logic_negate.\n"
            "\tmov al, [%s + %lu]\n"
            "\tneg al\n"
            "\tadd al, 1\n"
            "\tmov [TMP + %lu], al\n",
            label,
            original_address,
            f->address);
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

    fprintf(file,
            "\n\tsection .text ; codegen_convert_to_floating_point.\n"
            "\tmov eax, [%s + %lu]\n"
            "\tcdqe\n"
            "\tcvtsi2ss xmm0, rax\n"
            "\tmovss [TMP + %lu], xmm0\n",
            label,
            original_address,
            info->address);
}

static void
perform_addition_or_subtraction(const char *instr,
                                struct codegen_value_info *exps_info,
                                const struct codegen_value_info *t_info)
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
        fprintf(file,
                "\tmovss xmm0, [%s + %lu]\n"
                "\tmovss xmm1, [%s + %lu]\n"
                "\t%sss xmm0, xmm1\n"
                "\tmovss [TMP + %lu], xmm0\n",
                exps_label,
                original_address,
                t_label,
                t_info->address,
                instr,
                exps_info->address);
    } else if (exps_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(file,
                "\tmov eax, [%s + %lu]\n"
                "\tmov ebx, [%s + %lu]\n"
                "\t%s eax, ebx\n"
                "\tmov [TMP + %lu], eax\n",
                exps_label,
                original_address,
                t_label,
                t_info->address,
                instr,
                exps_info->address);
    } else {
        UNREACHABLE();
    }
}

void
codegen_perform_addition(struct codegen_value_info *exps_info,
                         const struct codegen_value_info *t_info)
{
    perform_addition_or_subtraction("add", exps_info, t_info);
}

void
codegen_perform_subtraction(struct codegen_value_info *exps_info,
                            const struct codegen_value_info *t_info)
{
    perform_addition_or_subtraction("sub", exps_info, t_info);
}

void
codegen_perform_logical_or(struct codegen_value_info *exps_info,
                           const struct codegen_value_info *t_info)
{
    assert(exps_info->type == t_info->type);

    const uint64_t original_address = exps_info->address;
    const char *exps_label = label_from_section(exps_info->section);
    const char *t_label = label_from_section(t_info->section);

    exps_info->section = SYMBOL_SECTION_NONE;
    exps_info->address =
        get_next_address(&current_bss_tmp_address, exps_info->size);

    char je_label_buffer[16];
    get_next_label(je_label_buffer, sizeof(je_label_buffer));

    fprintf(file,
            "\n\tsection .text ; codegen_perform_logical_or.\n"
            "\tmov al, [%s + %lu]\n"
            "\tmov bl, [%s + %lu]\n"
            "\tadd al, bl\n"
            "\tcmp al, 0\n"
            "\tje %s\n"
            "\tmov al, 1\n"
            "%s:\n"
            "\tmov [TMP + %lu], al\n",
            exps_label,
            original_address,
            t_label,
            t_info->address,
            je_label_buffer,
            je_label_buffer,
            exps_info->address);
}

void
codegen_negate(struct codegen_value_info *t_info)
{
    const uint64_t original_address = t_info->address;
    const char *label = label_from_section(t_info->section);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    fputs("\n\tsection .text ; codegen_negate.\n", file);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(file,
                "\tmov rax, 0\n"
                "\tcvtsi2ss xmm0, rax\n"
                "\tmovss xmm1, [%s + %lu]\n"
                "\tsubss xmm0, xmm1\n"
                "\tmovss [TMP + %lu], xmm0\n",
                label,
                original_address,
                t_info->address);
    } else if (t_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(file,
                "\tmov eax, [%s + %lu]\n"
                "\tneg eax\n"
                "\tmov [TMP + %lu], eax\n",
                label,
                original_address,
                t_info->address);
    } else {
        UNREACHABLE();
    }
}

void
codegen_perform_multiplication(struct codegen_value_info *t_info,
                               const struct codegen_value_info *f_info)
{
    assert(t_info->type == f_info->type);

    const uint64_t original_address = t_info->address;
    const char *t_label = label_from_section(t_info->section);
    const char *f_label = label_from_section(f_info->section);

    fputs("\n\tsection .text ; codegen_perform_multiplication.\n", file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(file,
                "\tmovss xmm0, [%s + %lu]\n"
                "\tmovss xmm1, [%s + %lu]\n"
                "\tmulss xmm0, xmm1\n"
                "\tmovss [TMP + %lu], xmm0\n",
                t_label,
                original_address,
                f_label,
                f_info->address,
                t_info->address);
    } else if (t_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(file,
                "\tmov eax, [%s + %lu]\n"
                "\tmov ebx, [%s + %lu]\n"
                "\timul ebx\n"
                "\tmov [TMP + %lu], eax\n",
                t_label,
                original_address,
                f_label,
                f_info->address,
                t_info->address);
    } else {
        UNREACHABLE();
    }
}

void
codegen_perform_division(struct codegen_value_info *t_info,
                         const struct codegen_value_info *f_info)
{
    assert(t_info->type == f_info->type);

    if (t_info->type == SYMBOL_TYPE_INTEGER) {
        codegen_perform_integer_division(t_info, f_info);
        return;
    }

    const uint64_t original_address = t_info->address;
    const char *t_label = label_from_section(t_info->section);
    const char *f_label = label_from_section(f_info->section);

    fputs("\n\tsection .text ; codegen_perform_division.\n", file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(file,
                "\tmovss xmm0, [%s + %lu]\n"
                "\tmovss xmm1, [%s + %lu]\n"
                "\tdivss xmm0, xmm1\n"
                "\tmovss [TMP + %lu], xmm0\n",
                t_label,
                original_address,
                f_label,
                f_info->address,
                t_info->address);
    } else {
        UNREACHABLE();
    }
}

static void
perform_integer_division(struct codegen_value_info *t_info,
                         const struct codegen_value_info *f_info)
{
    assert(t_info->type == f_info->type);

    const uint64_t original_address = t_info->address;
    const char *t_label = label_from_section(t_info->section);
    const char *f_label = label_from_section(f_info->section);

    fputs("\n\tsection .text ; perform_integer_division.\n", file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(file,
                "\tmov eax, [%s + %lu]\n"
                "\tmov ebx, [%s + %lu]\n"
                "\tcdq\n"
                "\tidiv ebx\n",
                t_label,
                original_address,
                f_label,
                f_info->address);
    } else {
        UNREACHABLE();
    }
}

void
codegen_perform_integer_division(struct codegen_value_info *t_info,
                                 const struct codegen_value_info *f_info)
{
    perform_integer_division(t_info, f_info);
    fprintf(file,
            "\tmov [TMP + %lu], eax ; codegen_perform_integer_division\n",
            t_info->address);
}

void
codegen_perform_mod(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info)
{
    perform_integer_division(t_info, f_info);
    fprintf(file,
            "\tmov [TMP + %lu], edx ; codegen_perform_integer_division\n",
            t_info->address);
}

void
codegen_perform_and(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info)
{
    assert(t_info->type == f_info->type);

    const uint64_t original_address = t_info->address;
    const char *t_label = label_from_section(t_info->section);
    const char *f_label = label_from_section(f_info->section);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    char jne_label_buffer[16];
    get_next_label(jne_label_buffer, sizeof(jne_label_buffer));

    char end_label_buffer[16];
    get_next_label(end_label_buffer, sizeof(end_label_buffer));

    fprintf(file,
            "\n\tsection .text ; codegen_perform_logical_and.\n"
            "\tmov al, [%s + %lu]\n"
            "\tmov bl, [%s + %lu]\n"
            "\tadd al, bl\n"
            "\tcmp al, 2\n"
            "\tjne %s\n"
            "\tmov al, 1\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov al, 0\n"
            "%s:\n"
            "\tmov [TMP + %lu], al\n",
            t_label,
            original_address,
            f_label,
            f_info->address,
            jne_label_buffer,
            end_label_buffer,
            jne_label_buffer,
            end_label_buffer,
            t_info->address);
}

static void
change_value_to_bool(struct codegen_value_info *info)
{
    info->section = SYMBOL_SECTION_NONE;
    info->type = SYMBOL_TYPE_LOGIC;
    info->size = size_from_type(info->type);
}

static void
load_and_compare(struct codegen_value_info *exp_info,
                 const struct codegen_value_info *exps_info)
{
    const char *exp_label = label_from_section(exp_info->section);
    const char *exps_label = label_from_section(exps_info->section);

    fputs("\n\tsection .text ; load_and_compare.\n", file);

    switch (exp_info->type) {
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_LOGIC:
            fprintf(file,
                    "\tmov al, [%s + %lu]\n"
                    "\tmov bl, [%s + %lu]\n"
                    "\tcmp al, bl\n",
                    exp_label,
                    exp_info->address,
                    exps_label,
                    exps_info->address);
            break;
        case SYMBOL_TYPE_INTEGER:
            fprintf(file,
                    "\tmov eax, [%s + %lu]\n"
                    "\tmov ebx, [%s + %lu]\n"
                    "\tcmp eax, ebx\n",
                    exp_label,
                    exp_info->address,
                    exps_label,
                    exps_info->address);
            break;
        case SYMBOL_TYPE_FLOATING_POINT:
            fprintf(file,
                    "\tmovss xmm0, [%s + %lu]\n"
                    "\tmovss xmm1, [%s + %lu]\n"
                    "\tcomiss xmm0, xmm1\n",
                    exp_label,
                    exp_info->address,
                    exps_label,
                    exps_info->address);
            break;
        default:
            UNREACHABLE();
    }
}

static void
generate_comparison_jump(enum token operation_tok, enum symbol_type type)
{
    const char *instr;
    switch (operation_tok) {
        case TOKEN_EQUAL:
            instr = "je";
            break;
        case TOKEN_NOT_EQUAL:
            instr = "jne";
            break;
        case TOKEN_LESS:
            if (type != SYMBOL_TYPE_FLOATING_POINT)
                instr = "jl";
            else
                instr = "jb";
            break;
        case TOKEN_LESS_EQUAL:
            if (type != SYMBOL_TYPE_FLOATING_POINT)
                instr = "jle";
            else
                instr = "jbe";
            break;
        case TOKEN_GREATER:
            if (type != SYMBOL_TYPE_FLOATING_POINT)
                instr = "jg";
            else
                instr = "ja";
            break;
        case TOKEN_GREATER_EQUAL:
            if (type != SYMBOL_TYPE_FLOATING_POINT)
                instr = "jge";
            else
                instr = "jae";
            break;
        default:
            UNREACHABLE();
    }

    char cmp_ok_label[16];
    get_next_label(cmp_ok_label, sizeof(cmp_ok_label));

    char cmp_not_ok_label[16];
    get_next_label(cmp_not_ok_label, sizeof(cmp_not_ok_label));

    fputs("\t; generate_comparison_jump.\n", file);
    fprintf(file,
            "\t%s %s\n"
            "\tmov al, 0\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov al, 1\n"
            "%s:\n",
            instr,
            cmp_ok_label,
            cmp_not_ok_label,
            cmp_ok_label,
            cmp_not_ok_label);
}

static void
compare_string(enum token operation_tok,
               struct codegen_value_info *exp_info,
               const struct codegen_value_info *exps_info)
{
    const char *exp_label = label_from_section(exp_info->section);
    const char *exps_label = label_from_section(exps_info->section);

    char loop_beg_label[16];
    get_next_label(loop_beg_label, sizeof(loop_beg_label));

    char ne_label[16];
    get_next_label(ne_label, sizeof(ne_label));

    char e_label[16];
    get_next_label(e_label, sizeof(e_label));

    char end_label[16];
    get_next_label(end_label, sizeof(end_label));

    fprintf(file,
            "\n\tsection .text ; compare_string.\n"
            "\tmov rsi, %s\n"
            "\tadd rsi, %lu\n"
            "\tmov rdi, %s\n"
            "\tadd rdi, %lu\n"
            "%s:\n"
            "\tmov al, [rsi]\n"
            "\tmov bl, [rdi]\n"
            "\tcmp al, bl\n"
            "\tjne %s\n"
            "\tcmp al, 0\n"
            "\tje %s\n"
            "\tadd rsi, 1\n"
            "\tadd rdi, 1\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov al, %u\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov al, %u\n"
            "%s:\n",
            exp_label,
            exp_info->address,
            exps_label,
            exps_info->address,
            loop_beg_label,
            ne_label,
            e_label,
            loop_beg_label,
            ne_label,
            operation_tok == TOKEN_EQUAL ? 0 : 1,
            end_label,
            e_label,
            operation_tok == TOKEN_EQUAL ? 1 : 0,
            end_label);
}

void
codegen_perform_comparison(enum token operation_tok,
                           struct codegen_value_info *exp_info,
                           const struct codegen_value_info *exps_info)
{
    assert(exp_info->type == exps_info->type);

    const uint64_t new_address = get_next_address(
        &current_bss_tmp_address, size_from_type(SYMBOL_TYPE_LOGIC));

    if (exp_info->type != SYMBOL_TYPE_STRING) {
        load_and_compare(exp_info, exps_info);
        generate_comparison_jump(operation_tok, exp_info->type);
    } else {
        compare_string(operation_tok, exp_info, exps_info);
    }

    fprintf(file,
            "\tmov [TMP + %lu], al ; codegen_perform_comparison.\n",
            new_address);

    exp_info->address = new_address;
    change_value_to_bool(exp_info);
}

void
codegen_move_to_id_entry(struct symbol *id_entry,
                         const struct codegen_value_info *exp)
{
    assert(id_entry->symbol_type == exp->type);

    const char *id_label = label_from_section(id_entry->symbol_section);
    const char *exp_label = label_from_section(exp->section);

    fputs("\n\tsection .text ; codegen_move_to_id_entry.\n", file);

    switch (id_entry->symbol_type) {
        case SYMBOL_TYPE_FLOATING_POINT:
            fprintf(file,
                    "\tmovss xmm0, [%s + %lu]\n"
                    "\tmovss [%s + %lu], xmm0\n",
                    exp_label,
                    exp->address,
                    id_label,
                    id_entry->address);
            break;
        case SYMBOL_TYPE_INTEGER:
            fprintf(file,
                    "\tmov eax, [%s + %lu]\n"
                    "\tmov [%s + %lu], eax\n",
                    exp_label,
                    exp->address,
                    id_label,
                    id_entry->address);
            break;
        case SYMBOL_TYPE_LOGIC:
        case SYMBOL_TYPE_CHAR:
            fprintf(file,
                    "\tmov al, [%s + %lu]\n"
                    "\tmov [%s + %lu], al\n",
                    exp_label,
                    exp->address,
                    id_label,
                    id_entry->address);
            break;
        case SYMBOL_TYPE_STRING: {
            char loop_beg_label[16];
            get_next_label(loop_beg_label, sizeof(loop_beg_label));

            char loop_end_label[16];
            get_next_label(loop_end_label, sizeof(loop_end_label));

            fprintf(file,
                    "\tmov rsi, %s\n"
                    "\tadd rsi, %lu\n"
                    "\tmov rdi, %s\n"
                    "\tadd rdi, %lu\n"
                    "%s:\n"
                    "\tmov al, [rdi]\n"
                    "\tmov [rsi], al\n"
                    "\tcmp al, 0\n"
                    "\tje %s\n"
                    "\tadd rdi, 1\n"
                    "\tadd rsi, 1\n"
                    "\tjmp %s\n"
                    "%s:",
                    id_label,
                    id_entry->address,
                    exp_label,
                    exp->address,
                    loop_beg_label,
                    loop_end_label,
                    loop_beg_label,
                    loop_end_label);
            break;
        }
        default:
            UNREACHABLE();
    }
}
