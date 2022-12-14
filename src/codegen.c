/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
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
 * Use better instructions.
 *
 * Get rid of write and read. In a proper language we should have support
 * for functions and be able to call the kernel natively or at least be able
 * to embed some assembly to call it for us without relying on hard coded
 * assembly on the code generator.
 * */

static FILE *tmp_file;
static char template_filename[] = "XXXXXX.asm";

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
    const uint64_t aligned_address = *address + not_aligned_by;
    *address = aligned_address + size;
    return aligned_address;
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

    fprintf(tmp_file,
            "\t; Generated on %04u/%02u/%02u - %02u:%02u\n"
            "\tglobal _start\n"
            "\tsection .bss\n"
            "TMP:\n"
            "\tresb 0x10000\n"
            "UNNIT_MEM:\n"
            "\tsection .data\n"
            "INIT_MEM:\n"
            "\tsection .rodata\n"
            "CONST_MEM:\n"
            "\tsection .text\n"
            "_start:\n",
            tm.tm_year + 1900,
            tm.tm_mon,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min);
}

static void
add_error_handler(const char *error_handler_name,
                  const char *error_message,
                  uint8_t exit_code)
{
    assert(exit_code && "Zero exit code might mean success to the user.");

    fprintf(tmp_file,
            "\tsection .text\n"
            "%s_HANDLER:\n"
            // Write Syscall.
            "\tmov rax, 1\n"
            "\tmov rdi, 2\n"
            "\tmov rsi, %s_MSG\n"
            "\tmov rdx, %s_MSG_LEN\n"
            "\tsyscall\n"
            // Exit Syscall.
            "\tmov rax, 60\n"
            "\tmov rdi, %u\n"
            "\tsyscall\n"
            // Data for Error.
            "\tsection .rodata\n"
            "%s_MSG: db \"Error: %s\",0xA,0x0\n"
            "%s_MSG_LEN: equ $-INVALID_INPUT_MSG\n",
            error_handler_name,
            error_handler_name,
            error_handler_name,
            exit_code,
            error_handler_name,
            error_message,
            error_handler_name);
}

static void
add_error_handlers(void)
{
    fputs("\t; Error Handlers\n", tmp_file);
    add_error_handler("INVALID_INPUT", "Invalid Input. Exitting...", 1);
}

static void
add_exit_syscall(uint8_t error_code)
{
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; add_exit_syscall.\n"
            "\tmov rax, 60\n"
            "\tmov rdi, %u\n"
            "\tsyscall\n",
            error_code);
}

int
codegen_init(void)
{
    const int fd = mkstemps(template_filename, 4);
    if (fd < 0)
        return -1;

    tmp_file = fdopen(fd, "w+");
    if (!tmp_file)
        return -1;

    dump_template();
    return 0;
}

static void
remove_unnecessary_section_commands(FILE *file, FILE *dump_file)
{
    enum section
    {
        NONE,
        TEXT,
        BSS,
        RODATA,
        DATA,
    } section = NONE;

    fseek(file, 0, SEEK_SET);

    char line_buffer[512];
    memset(line_buffer, 0, sizeof(line_buffer));

    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        uint8_t unnecesary_line = 0;

        if (strncmp(line_buffer, "\tsection .data", 14) == 0) {
            if (section == DATA)
                unnecesary_line = 1;
            else
                section = DATA;
        } else if (strncmp(line_buffer, "\tsection .rodata", 16) == 0) {
            if (section == RODATA)
                unnecesary_line = 1;
            else
                section = RODATA;
        } else if (strncmp(line_buffer, "\tsection .bss", 13) == 0) {
            if (section == BSS)
                unnecesary_line = 1;
            else
                section = BSS;
        } else if (strncmp(line_buffer, "\tsection .text", 14) == 0) {
            if (section == TEXT)
                unnecesary_line = 1;
            else
                section = TEXT;
        }

        if (!unnecesary_line)
            fputs(line_buffer, dump_file);
    }

    fflush(dump_file);
}

static void
perform_peephole(FILE *file, FILE *dump_file)
{
    // A very naive implementation in which we look for consecutive loads and
    // stores and remove those instructions that have no impact in execution.
    fseek(file, 0, SEEK_SET);

    enum
    {
        WAITING_FIRST_MOV,
        WAITING_SECOND_MOV,
    } state = WAITING_FIRST_MOV;

    char line[512];
    char tmp_line[512];
    char store_mem_dst[32];
    char store_reg_src[32];
    char load_mem_src[32];
    char load_reg_dst[32];

    uint32_t avoided_moves = 0;

    while (fgets(line, sizeof(line), file)) {
    check_again:
        switch (state) {
            case WAITING_FIRST_MOV:
                if (strncmp(line, "\tmov", 4) != 0 &&
                    strncmp(line, "\tmovss", 6) != 0) {
                    fputs(line, dump_file);
                } else {
                    fputs(line, dump_file);

                    const uint8_t is_store = strstr(line, "],") != NULL;
                    if (is_store) {
                        strtok(line, "[");
                        const char *ptr = strtok(NULL, "[");

                        char *mem_dst = store_mem_dst;
                        while (*ptr != ']')
                            *mem_dst++ = *ptr++;
                        *mem_dst = 0;

                        // Skip ",] ".
                        ptr += 3;

                        char *reg_src = store_reg_src;
                        while (*ptr != '\n')
                            *reg_src++ = *ptr++;
                        *reg_src = 0;

                        state = WAITING_SECOND_MOV;
                    }
                }
                break;
            case WAITING_SECOND_MOV:
                if (strncmp(line, "\tmov", 4) == 0 ||
                    strncmp(line, "\tmovss", 6) == 0) {
                    state = WAITING_FIRST_MOV;

                    const uint8_t is_load = strstr(line, ", [") != NULL;
                    if (is_load) {
                        strncpy(tmp_line, line, sizeof(line));

                        strtok(tmp_line, " ");
                        const char *ptr = strtok(NULL, " ");

                        char *load_dst = load_reg_dst;
                        while (*ptr != ',')
                            *load_dst++ = *ptr++;
                        *load_dst = 0;

                        // Skip ", [".
                        ptr += 3;

                        char *mem_src = load_mem_src;
                        while (*ptr != ']')
                            *mem_src++ = *ptr++;
                        *mem_src = 0;

                        if (strcmp(load_mem_src, store_mem_dst) == 0 &&
                            strcmp(load_reg_dst, store_reg_src) == 0) {
                            ++avoided_moves;
                        } else {
                            fputs(line, dump_file);
                        }
                    } else {
                        // Ok, this is not a load and therefore we can't remove
                        // it... but is it a store that's followed by a load? We
                        // can check that by checking this line again in the
                        // first state!
                        goto check_again;
                    }
                } else {
                    uint8_t instruction_line = 1;

                    if (strncmp(line, "\tsection", 8) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\t;", 2) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\talign", 6) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\talignb", 7) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\tdb", 3) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\tdd", 3) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\ttimes", 6) == 0) {
                        instruction_line = 0;
                    } else if (strncmp(line, "\tresb", 5) == 0) {
                        instruction_line = 0;
                    } else if (line[0] != '\t') {
                        // LABEL.
                        instruction_line = 0;
                    }

                    fputs(line, dump_file);
                    if (instruction_line) {
                        state = WAITING_FIRST_MOV;
                    }
                }
                break;
        }
    }

    fprintf(ERR_STREAM, "Peephole avoided moves: %u.\n", avoided_moves);
}

int
codegen_dump(const char *pathname,
             uint8_t keep_unoptimized,
             uint8_t assemble_and_link)
{
    if (!tmp_file)
        return -1;

    int err = 0;

    add_exit_syscall(0);
    add_error_handlers();
    fflush(tmp_file);

    char remove_section_filename[256];
    char peephole_filename[256];

    snprintf(remove_section_filename,
             sizeof(remove_section_filename),
             "%s-removed-sections-%s",
             pathname,
             template_filename);
    snprintf(peephole_filename,
             sizeof(peephole_filename),
             "%s-peephole-%s",
             pathname,
             template_filename);

    FILE *unnecessary_sections_removed_file =
        fopen(remove_section_filename, "w+");
    if (!unnecessary_sections_removed_file)
        return -1;

    FILE *peephole_file = fopen(peephole_filename, "w+");
    if (!peephole_file) {
        err = -1;
        goto peephole_file_err;
    }

    remove_unnecessary_section_commands(tmp_file,
                                        unnecessary_sections_removed_file);
    perform_peephole(unnecessary_sections_removed_file, peephole_file);

    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s.asm", pathname);

    FILE *output = fopen(output_filename, "w+");
    if (!output) {
        err = -1;
        goto output_file_err;
    }

    fseek(peephole_file, 0, SEEK_SET);
    while (1) {
        const char c = fgetc(peephole_file);
        if (c == EOF)
            break;
        fputc(c, output);
    }

    fclose(output);
    output = NULL;

    fprintf(ERR_STREAM, "Assembly output in: %s.\n", output_filename);

    if (keep_unoptimized) {
        fprintf(ERR_STREAM,
                "Assembly without peephole in: %s.\n",
                remove_section_filename);
    }

    if (assemble_and_link) {
        char buffer[1024];

        snprintf(buffer,
                 sizeof(buffer),
                 "nasm -f elf64 %s -o %s.o",
                 output_filename,
                 output_filename);
        fprintf(ERR_STREAM, "Running: \"%s\".\n", buffer);

        if (system(buffer) == 0) {
            snprintf(buffer,
                     sizeof(buffer),
                     "ld %s.o -o %s.out",
                     output_filename,
                     output_filename);
            fprintf(ERR_STREAM, "Running: \"%s\".\n", buffer);

            if (system(buffer) == 0)
                fprintf(ERR_STREAM,
                        "Successfully assembled and linked. Executable in: "
                        "%s.out.\n",
                        output_filename);
        }

        // Remove the assembled but not linked file.
        snprintf(buffer, sizeof(buffer), "%s.o", output_filename);
        remove(buffer);
    }

output_file_err:
    fclose(peephole_file);
    peephole_file = NULL;
    remove(peephole_filename);

peephole_file_err:
    fclose(unnecessary_sections_removed_file);
    unnecessary_sections_removed_file = NULL;
    if (err || !keep_unoptimized)
        remove(remove_section_filename);

    return err;
}

void
codegen_destroy(void)
{
    if (!tmp_file)
        return;

    fclose(tmp_file);
    tmp_file = NULL;

    remove(template_filename);
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
    fputs("\tsection .bss\n\t; codegen_add_unnit_value.\n", tmp_file);
    fprintf(tmp_file, "\talignb %lu\n", info->size);
    fprintf(tmp_file, "\tresb %lu\t; @ 0x%lx\n", info->size, info->address);

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

    fprintf(tmp_file, "\tsection %s\n\t; codegen_add_value.\n", section_name);
    fprintf(tmp_file, "\talign %lu\n", info->size);

    switch (type) {
        case SYMBOL_TYPE_LOGIC:
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_STRING:
            fputs("\tdb ", tmp_file);
            break;
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            fputs("\tdd ", tmp_file);
            break;
        default:
            UNREACHABLE();
    }

    if (has_minus)
        fputc('-', tmp_file);

    switch (type) {
        case SYMBOL_TYPE_LOGIC:
            if (is_case_insensitive_equal("true", lexeme))
                fputc('1', tmp_file);
            else
                fputc('0', tmp_file);
            break;
        case SYMBOL_TYPE_STRING:
            fprintf(tmp_file, "%s,0", lexeme);
            break;
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_FLOATING_POINT:
        case SYMBOL_TYPE_INTEGER:
            fputs(lexeme, tmp_file);
            break;
        default:
            UNREACHABLE();
    }

    fprintf(tmp_file, "\t; @ 0x%lx\n", info->address);

    if (type == SYMBOL_TYPE_STRING) {
        // Reserve enough space for the unnitialized portion of the string.
        // At this point, we've only reserved space for the portion that we
        // explicitly initialized with "db".

        // strlen(lexeme)
        // -2 because of ""
        // +1 because of \0
        const uint64_t already_reserved_size = strlen(lexeme) - 1;
        const uint64_t to_reserve = info->size - already_reserved_size;
        fprintf(tmp_file, "\ttimes %lu db 0\n", to_reserve);
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

    fputs("\tsection .text\n\t; codegen_add_tmp.\n", tmp_file);

    const char *reg;
    if (type == SYMBOL_TYPE_INTEGER)
        reg = "eax";
    else if (type == SYMBOL_TYPE_CHAR || type == SYMBOL_TYPE_LOGIC)
        reg = "al";
    else
        UNREACHABLE();

    if (type != SYMBOL_TYPE_LOGIC) {
        fprintf(tmp_file, "\tmov %s, %s\n", reg, lexeme);
    } else {
        if (is_case_insensitive_equal("true", lexeme)) {
            fputs("\tmov al, 1\n", tmp_file);
        } else if (is_case_insensitive_equal("false", lexeme)) {
            fputs("\tmov al, 0\n", tmp_file);
        } else {
            UNREACHABLE();
        }
    }

    fprintf(tmp_file, "\tmov [TMP + %lu], %s\n", info->address, reg);
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
    f->section = SYMBOL_SECTION_NONE;

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_logic_negate.\n"
            "\tmov al, [%s + %lu]\n"
            "\tneg al\n"
            "\tadd al, 1\n"
            "\tmov [TMP + %lu], al\n",
            label,
            original_address,
            f->address);
}

void
codegen_convert_to_integer(struct codegen_value_info *info)
{
    assert(info->type == SYMBOL_TYPE_FLOATING_POINT);

    const uint64_t original_address = info->address;
    const char *label = label_from_section(info->section);

    // Update value information.
    info->section = SYMBOL_SECTION_NONE;
    info->type = SYMBOL_TYPE_INTEGER;
    info->address = get_next_address(&current_bss_tmp_address, info->size);

    // We definitely want to truncate here and the right instruction
    // would be cvttss2si (the extra t is for truncation).
    // Since I can't use it, I'm gonna round the number before converting... :(
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_convert_to_integer.\n"
            "\tmovss xmm0, [%s + %lu]\n"
            "\troundss xmm0, xmm0, 0b0011\n"
            "\tcvtss2si eax, xmm0\n"
            "\tmov [TMP + %lu], eax\n",
            label,
            original_address,
            info->address);
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

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_convert_to_floating_point.\n"
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

    fputs("\tsection .text\n"
          "\t; perform_addition_or_subtraction.\n",
          tmp_file);

    if (exps_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(tmp_file,
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
        fprintf(tmp_file,
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

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_perform_logical_or.\n"
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

    fputs("\tsection .text\n"
          "\t; codegen_negate.\n",
          tmp_file);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(tmp_file,
                "\tmov rax, 0\n"
                "\tcvtsi2ss xmm0, rax\n"
                "\tmovss xmm1, [%s + %lu]\n"
                "\tsubss xmm0, xmm1\n"
                "\tmovss [TMP + %lu], xmm0\n",
                label,
                original_address,
                t_info->address);
    } else if (t_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(tmp_file,
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

    fputs("\tsection .text\n"
          "\t; codegen_perform_multiplication.\n",
          tmp_file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(tmp_file,
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
        fprintf(tmp_file,
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

    fputs("\tsection .text ; codegen_perform_division.\n", tmp_file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_FLOATING_POINT) {
        fprintf(tmp_file,
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

    fputs("\tsection .text\n"
          "\t; perform_integer_division.\n",
          tmp_file);

    t_info->section = SYMBOL_SECTION_NONE;
    t_info->address = get_next_address(&current_bss_tmp_address, t_info->size);

    if (t_info->type == SYMBOL_TYPE_INTEGER) {
        fprintf(tmp_file,
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
    fprintf(tmp_file,
            "\tmov [TMP + %lu], eax ; codegen_perform_integer_division\n",
            t_info->address);
}

void
codegen_perform_mod(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info)
{
    perform_integer_division(t_info, f_info);
    fprintf(tmp_file,
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

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_perform_logical_and.\n"
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

    fputs("\tsection .text\n"
          "\t; load_and_compare.\n",
          tmp_file);

    switch (exp_info->type) {
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_LOGIC:
            fprintf(tmp_file,
                    "\tmov al, [%s + %lu]\n"
                    "\tmov bl, [%s + %lu]\n"
                    "\tcmp al, bl\n",
                    exp_label,
                    exp_info->address,
                    exps_label,
                    exps_info->address);
            break;
        case SYMBOL_TYPE_INTEGER:
            fprintf(tmp_file,
                    "\tmov eax, [%s + %lu]\n"
                    "\tmov ebx, [%s + %lu]\n"
                    "\tcmp eax, ebx\n",
                    exp_label,
                    exp_info->address,
                    exps_label,
                    exps_info->address);
            break;
        case SYMBOL_TYPE_FLOATING_POINT:
            fprintf(tmp_file,
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

    fputs("\t; generate_comparison_jump.\n", tmp_file);
    fprintf(tmp_file,
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

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; compare_string.\n"
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

    fprintf(tmp_file,
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

    fputs("\tsection .text\n"
          "\t; codegen_move_to_id_entry.\n",
          tmp_file);

    switch (id_entry->symbol_type) {
        case SYMBOL_TYPE_FLOATING_POINT:
            fprintf(tmp_file,
                    "\tmovss xmm0, [%s + %lu]\n"
                    "\tmovss [%s + %lu], xmm0\n",
                    exp_label,
                    exp->address,
                    id_label,
                    id_entry->address);
            break;
        case SYMBOL_TYPE_INTEGER:
            fprintf(tmp_file,
                    "\tmov eax, [%s + %lu]\n"
                    "\tmov [%s + %lu], eax\n",
                    exp_label,
                    exp->address,
                    id_label,
                    id_entry->address);
            break;
        case SYMBOL_TYPE_LOGIC:
        case SYMBOL_TYPE_CHAR:
            fprintf(tmp_file,
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

            fprintf(tmp_file,
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
                    "%s:\n",
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

void
codegen_move_to_id_entry_idx(struct symbol *id_entry,
                             const struct codegen_value_info *exp,
                             const struct codegen_value_info *idx_expr_info)
{
    assert(id_entry->symbol_type == SYMBOL_TYPE_STRING);
    assert(exp->type == SYMBOL_TYPE_CHAR);
    assert(idx_expr_info->type == SYMBOL_TYPE_INTEGER);

    const char *id_label = label_from_section(id_entry->symbol_section);
    const char *exp_label = label_from_section(exp->section);
    const char *idx_expr_info_label =
        label_from_section(idx_expr_info->section);

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_move_to_id_entry_idx.\n"
            "\tmov eax, [%s + %lu]\n"
            "\tadd eax, %s + %lu\n"
            "\tmov bl, [%s + %lu]\n"
            "\tmov [eax], bl\n",
            idx_expr_info_label,
            idx_expr_info->address,
            id_label,
            id_entry->address,
            exp_label,
            exp->address);
}

static void
write_string(const struct codegen_value_info *exp)
{
    const uint64_t tmp_address =
        get_next_address(&current_bss_tmp_address, 257);

    const char *label = label_from_section(exp->section);

    char loop_label[16];
    get_next_label(loop_label, sizeof(loop_label));

    fprintf(tmp_file,
            "\t; write_string\n"
            "\tmov esi, %s + %lu\n"
            "\tmov edi, TMP + %lu\n"
            "%s:\n"
            "\tmov al, [esi]\n"
            "\tmov [edi], al\n"
            "\tadd esi, 1\n"
            "\tadd edi, 1\n"
            "\tcmp al, 0\n"
            "\tjne %s\n"
            "\tmov esi, TMP + %lu\n"
            "\tmov edx, edi\n"
            "\tsub edx, esi\n"
            "\tsub edx, 1\n",
            label,
            exp->address,
            tmp_address,
            loop_label,
            loop_label,
            tmp_address);
}

static void
write_char(const struct codegen_value_info *exp)
{
    const uint64_t tmp_address = get_next_address(&current_bss_tmp_address, 4);

    const char *label = label_from_section(exp->section);
    fprintf(tmp_file,
            "\t; write_char\n"
            // Recover char from memory.
            "\tmov al, [%s + %lu]\n"
            // Place it followed by a \0 in the temporary area.
            "\tmov esi, TMP + %lu\n"
            "\tmov [esi], al\n"
            "\tadd esi, 1\n"
            "\tmov al, 0\n"
            "\tmov [esi], al\n"
            // Address of buffer and size for syscall.
            "\tmov esi, TMP + %lu\n"
            "\tmov edx, 1\n",
            label,
            exp->address,
            tmp_address,
            tmp_address);
}

static void
write_logic(const struct codegen_value_info *exp)
{
    const char *exp_label = label_from_section(exp->section);
    const uint64_t tmp_address = get_next_address(&current_bss_tmp_address, 8);

    char jne_label[16];
    get_next_label(jne_label, sizeof(jne_label));

    char jmp_label[16];
    get_next_label(jmp_label, sizeof(jmp_label));

    fprintf(tmp_file,
            "\t ; write_logic\n"
            "\tmov rax, [%s + %lu]\n"
            "\tcmp rax, 0\n"
            "\tjne %s\n"
            "\tmov rax, \"false\"\n"
            "\tmov [TMP + %lu], rax\n"
            "\tmov rsi, TMP + %lu\n"
            "\tmov rdx, 5\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov rax, \"true\"\n"
            "\tmov [TMP + %lu], rax\n"
            "\tmov rsi, TMP + %lu\n"
            "\tmov rdx, 5\n"
            "%s:\n",
            exp_label,
            exp->address,
            jne_label,
            tmp_address,
            tmp_address,
            jmp_label,
            jne_label,
            tmp_address,
            tmp_address,
            jmp_label);
}

static void
write_integer(const struct codegen_value_info *exp)
{
    const char *exp_label = label_from_section(exp->section);
    const uint64_t tmp_address = get_next_address(&current_bss_tmp_address, 32);

    char jge_label[16];
    get_next_label(jge_label, sizeof(jge_label));

    char loop_beg_label[16];
    get_next_label(loop_beg_label, sizeof(loop_beg_label));

    char loop_1_beg_label[16];
    get_next_label(loop_1_beg_label, sizeof(loop_1_beg_label));

    fprintf(tmp_file,
            "\t; write_integer\n"
            // Number we will convert.
            "\tmov eax, [%s + %lu]\n"
            // String destination buffer.
            "\tmov edi, TMP + %lu\n"
            // Stack counter.
            "\tmov ecx, 0\n"
            // Size of converted string.
            "\tmov esi, 0\n"
            // Check if we need to place the - sign.
            "\tcmp eax, 0\n"
            "\tjge %s\n"
            // We do need!
            "\tmov bl, '-'\n"
            "\tmov [edi], bl\n"
            "\tadd edi, 1\n"
            "\tadd esi, 1\n"
            "\tneg eax\n"
            // Actually convert the number.
            "%s:\n"
            "\tmov ebx, 10\n"
            // Divide and push the rest into the stack
            // until the result is non zero.
            "%s:\n"
            "\tadd ecx, 1\n"
            "\tcdq\n"
            "\tidiv ebx\n"
            "\tpush dx\n"
            "\tcmp eax, 0\n"
            "\tjne %s\n"
            // Total length of string.
            // Our stack counter + original length.
            "\tadd esi, ecx\n"
            // Now we pop every element into the string buffer.
            "%s:\n"
            "\tpop dx\n"
            "\tadd dl, '0'\n"
            "\tmov [edi], dl\n"
            "\tadd edi, 1\n"
            "\tsub ecx, 1\n"
            "\tcmp ecx, 0\n"
            "\tjne %s\n"
            // Place '\0' in the end.
            "\tmov dl, 0\n"
            "\tmov [edi], dl\n"
            // Size from esi to edx for syscall.
            // esi receives buffer adress.
            "\tmov edx, esi\n"
            "\tmov esi, TMP + %lu\n",
            exp_label,
            exp->address,
            tmp_address,
            jge_label,
            jge_label,
            loop_beg_label,
            loop_beg_label,
            loop_1_beg_label,
            loop_1_beg_label,
            tmp_address);
}

static void
write_float(const struct codegen_value_info *exp)
{
    const char *exp_label = label_from_section(exp->section);
    const uint64_t tmp_address = get_next_address(&current_bss_tmp_address, 32);

    char jae_label[16];
    get_next_label(jae_label, sizeof(jae_label));

    char int_conversion_beg_label[16];
    get_next_label(int_conversion_beg_label, sizeof(int_conversion_beg_label));

    char int_string_put_label[16];
    get_next_label(int_string_put_label, sizeof(int_string_put_label));

    char print_label[16];
    get_next_label(print_label, sizeof(print_label));

    char float_conversion_beg_label[16];
    get_next_label(float_conversion_beg_label,
                   sizeof(float_conversion_beg_label));

    fprintf(tmp_file,
            "\t; write_float\n"
            // Number we will convert.
            "\tmovss xmm0, [%s + %lu]\n"
            // String destination buffer.
            "\tmov edi, TMP + %lu\n"
            // Stack counter.
            "\tmov ecx, 0\n"
            // Precision of 6 digits (shared between integer and fraction).
            "\tmov esi, 6\n"
            // Set the divisor.
            "\tmov ebx, 10\n"
            "\tcvtsi2ss xmm2, ebx\n"
            // Check if we need to place the - sign.
            "\tsubss xmm1, xmm1\n"
            "\tcomiss xmm0, xmm1\n"
            "\tjae %s\n"
            "\tmov bl, '-'\n"
            "\tmov [edi], bl\n"
            "\tadd edi, 1\n"
            // Also negate the value.
            "\tmov edx, -1\n"
            "\tcvtsi2ss xmm1, edx\n"
            "\tmulss xmm0, xmm1\n"
            "%s:\n"
            // Place the integer into xmm1 and leave the fraction in xmm0.
            "\troundss xmm1, xmm0, 0b0011\n"
            "\tsubss xmm0, xmm1\n"
            // Convert integer
            "\tcvtss2si eax, xmm1\n"
            "\tmov ebx, 10\n"
            "%s:\n"
            "\tadd ecx, 1\n"
            "\tcdq\n"
            "\tidiv ebx\n"
            "\tpush dx\n"
            "\tcmp eax, 0\n"
            "\tjne %s\n"
            // Calculate the precision (digits we still have to write).
            "\tsub esi, ecx\n"
            // Place integer into buffer.
            "%s:\n"
            "\tpop dx\n"
            "\tadd dl, '0'\n"
            "\tmov [edi], dl\n"
            "\tadd edi, 1\n"
            "\tsub ecx, 1\n"
            "\tcmp ecx, 0\n"
            "\tjne %s\n"
            // Place decimal point.
            "\tmov dl, '.'\n"
            "\tmov [edi], dl\n"
            "\tadd edi, 1\n"
            // Check if we still have precision to convert the fraction.
            "%s:\n"
            "\tcmp esi, 0\n"
            "\tjle %s\n"
            // Multiply the fraction by ten so we can get the next digit.
            "\tmulss xmm0, xmm2\n"
            "\troundss xmm1, xmm0, 0b0011\n"
            // Update xmm0 so that it has only fraction.
            "\tsubss xmm0, xmm1\n"
            // Converted digit in edx.
            "\tcvtss2si edx, xmm1\n"
            "\tadd dl, '0'\n"
            "\tmov [edi], dl\n"
            "\tadd edi, 1\n"
            "\tsub esi, 1\n"
            "\tjmp %s\n"
            "%s:\n"
            // Place NULL terminator.
            "\tmov dl, 0\n"
            "\tmov [edi], dl\n"
            // Beginning of buffer in esi for syscall.
            "\tmov esi, TMP + %lu\n"
            // Calculate size of converted string.
            "\tsub edi, esi\n"
            "\tmov edx, edi\n",
            exp_label,
            exp->address,
            tmp_address,
            jae_label,
            jae_label,
            int_conversion_beg_label,
            int_conversion_beg_label,
            int_string_put_label,
            int_string_put_label,
            float_conversion_beg_label,
            print_label,
            float_conversion_beg_label,
            print_label,
            tmp_address);
}

void
codegen_write(const struct codegen_value_info *exp, uint8_t needs_new_line)
{
    fputs("\tsection .text\n"
          "\t; codegen_write\n",
          tmp_file);

    switch (exp->type) {
        case SYMBOL_TYPE_FLOATING_POINT:
            write_float(exp);
            break;
        case SYMBOL_TYPE_INTEGER:
            write_integer(exp);
            break;
        case SYMBOL_TYPE_STRING:
            write_string(exp);
            break;
        case SYMBOL_TYPE_CHAR:
            write_char(exp);
            break;
        case SYMBOL_TYPE_LOGIC:
            write_logic(exp);
            break;
        default:
            UNREACHABLE();
    }

    if (needs_new_line) {
        // Append a \n to the buffer.
        fputs("\t; Appending \\n to the buffer.\n"
              "\tmov eax, esi\n"
              "\tadd eax, edx\n"
              "\tmov bl, 0x0A\n"
              "\tmov [eax], bl\n"
              "\tadd eax, 1\n"
              "\tmov bl, 0\n"
              "\tmov [eax], bl\n"
              "\tadd edx, 1\n",
              tmp_file);
    }

    fputs("\tmov eax, 1\n"
          "\tmov edi, 1\n"
          "\tsyscall\n",
          tmp_file);
}

void
codegen_move_idx_to_tmp(const struct symbol *id_entry,
                        const struct codegen_value_info *idx_expr_info,
                        struct codegen_value_info *f_info)
{
    assert(id_entry->symbol_type == SYMBOL_TYPE_STRING);
    assert(idx_expr_info->type == SYMBOL_TYPE_INTEGER);

    f_info->type = SYMBOL_TYPE_CHAR;
    f_info->size = size_from_type(f_info->type);
    f_info->address = get_next_address(&current_bss_tmp_address, f_info->size);
    f_info->section = SYMBOL_SECTION_NONE;

    const char *idx_expr_info_label =
        label_from_section(idx_expr_info->section);
    const char *id_entry_label = label_from_section(id_entry->symbol_section);

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_move_idx_to_tmp.\n"
            "\tmov eax, [%s + %lu]\n"
            "\tadd eax, %s + %lu\n"
            "\tmov bl, [eax]\n"
            "\tmov [TMP + %lu], bl\n",
            idx_expr_info_label,
            idx_expr_info->address,
            id_entry_label,
            id_entry->address,
            f_info->address);
}

static char while_loop_start_label[16];
static char while_loop_end_label[16];

void
codegen_start_loop(void)
{
    get_next_label(while_loop_start_label, sizeof(while_loop_start_label));
    get_next_label(while_loop_end_label, sizeof(while_loop_end_label));

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_start_loop.\n"
            "%s:\n",
            while_loop_start_label);
}

void
codegen_eval_loop_expr(const struct codegen_value_info *exp)
{
    assert(exp->type == SYMBOL_TYPE_LOGIC);

    const char *exp_label = label_from_section(exp->section);

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_eval_loop_expr.\n"
            "\tmov al, [%s + %lu]\n"
            "\tcmp al, 0\n"
            "\tje %s\n",
            exp_label,
            exp->address,
            while_loop_end_label);
}

void
codegen_finish_loop(void)
{
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_finish_loop.\n"
            "\tjmp %s\n"
            "%s:\n",
            while_loop_start_label,
            while_loop_end_label);
}

static char if_end_label[16];
static char if_false_label[16];

void
codegen_start_if(const struct codegen_value_info *exp)
{
    assert(exp->type == SYMBOL_TYPE_LOGIC);

    const char *exp_label = label_from_section(exp->section);

    get_next_label(if_end_label, sizeof(if_end_label));
    get_next_label(if_false_label, sizeof(if_false_label));

    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_start_if.\n"
            "\tmov al, [%s + %lu]\n"
            "\tcmp al, 0\n"
            "\tje %s\n",
            exp_label,
            exp->address,
            if_false_label);
}

void
codegen_if_jmp(void)
{
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_if_jmp.\n"
            "\tjmp %s\n",
            if_end_label);
}

void
codegen_start_else(void)
{
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_start_else.\n"
            "%s:\n",
            if_false_label);
}

void
codegen_finish_if(uint8_t had_else)
{
    fprintf(tmp_file,
            "\tsection .text\n"
            "\t; codegen_finish_if.\n"
            "%s:\n",
            had_else ? if_end_label : if_false_label);
}

uint64_t
read_logic(uint64_t buffer_addr)
{
    // For now, accepts false/true as input.
    const uint64_t logic_addr = get_next_address(&current_bss_tmp_address, 1);

    char false_label[16];
    get_next_label(false_label, sizeof(false_label));

    char true_label[16];
    get_next_label(true_label, sizeof(true_label));

    char end_label[16];
    get_next_label(end_label, sizeof(end_label));

    fprintf(tmp_file,
            "\t; read_logic\n"
            "\tmov esi, TMP + %lu\n"
            "\tmov rax, [esi]\n"
            // Checks for false.
            "\tmov rbx, \"false\"\n"
            "\tcmp rax, rbx\n"
            "\tje %s\n"
            // Checks for true.
            "\tmov rbx, \"true\"\n"
            "\tcmp rax, rbx\n"
            "\tje %s\n"
            "\tjmp INVALID_INPUT_HANDLER\n"
            "%s:\n"
            "\tmov byte [TMP + %lu], 0\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tmov byte [TMP + %lu], 1\n"
            "%s:\n",

            buffer_addr,
            false_label,
            true_label,
            false_label,
            logic_addr,
            end_label,
            true_label,
            logic_addr,
            end_label);

    return logic_addr;
}

uint64_t
read_int(uint64_t buffer_addr)
{
    // FIXME:
    // This assumes the first character might be a '-', but
    // apart from that, we assume every character after that
    // is a *valid* digit.

    const uint64_t int_address = get_next_address(&current_bss_tmp_address, 4);

    char loop_start[16];
    get_next_label(loop_start, sizeof(loop_start));

    char loop_end[16];
    get_next_label(loop_end, sizeof(loop_end));

    char no_signal_label[16];
    get_next_label(no_signal_label, sizeof(no_signal_label));

    char end_label[16];
    get_next_label(end_label, sizeof(end_label));

    fprintf(tmp_file,
            "\t; read_int\n"
            "\tmov eax, 0\n"
            "\tmov ebx, 0\n"
            "\tmov ecx, 10\n"
            "\tmov dx, 1\n"
            "\tmov esi, TMP + %lu\n"
            // Take a look at the first character to verify if it's a '-'.
            "\tmov bl, [esi]\n"
            "\tcmp bl, '-'\n"
            "\tjne %s\n"
            // In case it is, store a -1 in the stack...
            "\tmov dx, -1\n"
            "\tadd esi, 1\n"
            "\tmov bl, [esi]\n"
            "%s:\n"
            "\tpush dx\n"
            "\tmov edx, 0\n"
            "%s:\n"
            "\tcmp bl, 0\n"
            "\tje %s\n"
            "\timul ecx\n"
            // We have zeroed edx, if it's not zero after imul,
            // it means an overflow happened.
            "\tcmp edx, 0\n"
            "\tjne INVALID_INPUT_HANDLER\n"
            "\tsub bl, '0'\n"
            "\tadd eax, ebx\n"
            "\tadd esi, 1\n"
            "\tmov bl, [esi]\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tpop cx\n"
            "\tcmp cx, 0\n"
            "\tjg %s\n"
            "\tneg eax\n"
            "%s:\n"
            "\tmov [TMP + %lu], eax\n",
            buffer_addr,
            no_signal_label,
            no_signal_label,
            loop_start,
            loop_end,
            loop_start,
            loop_end,
            end_label,
            end_label,
            int_address);

    return int_address;
}

uint64_t
read_float(uint64_t buffer_addr)
{
    // FIXME:
    // This assumes the first character might be a '-' and that
    // we might find a '.' amidst the input, but no further validation
    // is done... we assume every other character is a *valid* digit.
    const uint64_t float_address =
        get_next_address(&current_bss_tmp_address, 4);

    char int_loop_start[16];
    get_next_label(int_loop_start, sizeof(int_loop_start));

    char float_loop_start[16];
    get_next_label(float_loop_start, sizeof(float_loop_start));

    char loop_end[16];
    get_next_label(loop_end, sizeof(loop_end));

    char no_signal_label[16];
    get_next_label(no_signal_label, sizeof(no_signal_label));

    fprintf(tmp_file,
            "\t; read_float.\n"
            "\tmov eax, 0\n"
            "\tsubss xmm0, xmm0\n"
            "\tmov ebx, 0\n"
            "\tmov ecx, 10\n"
            "\tcvtsi2ss xmm3, ecx\n"
            "\tmovss xmm2, xmm3\n"
            "\tmov rdx, 1\n"
            "\tmov esi, TMP + %lu\n"
            "\tmov bl, [esi]\n"
            "\tcmp bl, '-'\n"
            "\tjne %s\n"
            "\tmov rdx, -1\n"
            "\tadd esi, 1\n"
            "\tmov bl, [esi]\n"
            "%s:\n"
            "\tpush rdx\n"
            "\tmov rdx, 0\n"
            "%s:\n"
            "\tcmp bl, 0\n"
            "\tje %s\n"
            "\tcmp bl, '.'\n"
            "\tje %s\n"
            "\timul ecx\n"
            "\tsub bl, '0'\n"
            "\tadd eax, ebx\n"
            "\tadd esi, 1\n"
            "\tmov bl, [esi]\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tadd esi, 1\n"
            "\tmov bl, [esi]\n"
            "\tcmp bl, 0\n"
            "\tje %s\n"
            "\tsub bl, '0'\n"
            "\tcvtsi2ss xmm1, rbx\n"
            "\tdivss xmm1, xmm2\n"
            "\taddss xmm0, xmm1\n"
            "\tmulss xmm2, xmm3\n"
            "\tjmp %s\n"
            "%s:\n"
            "\tcvtsi2ss xmm1, rax\n"
            "\taddss xmm0, xmm1\n"
            "\tpop rcx\n"
            "\tcvtsi2ss xmm1, rcx\n"
            "\tmulss xmm0, xmm1\n"
            "\tmovss [TMP + %lu], xmm0\n",
            buffer_addr,
            no_signal_label,
            no_signal_label,
            int_loop_start,
            loop_end,
            float_loop_start,
            int_loop_start,
            float_loop_start,
            loop_end,
            float_loop_start,
            loop_end,
            float_address);

    return float_address;
}

void
codegen_read_into(struct symbol *id_entry)
{
    const uint32_t buffer_size = 257;
    const uint64_t tmp_address =
        get_next_address(&current_bss_tmp_address, buffer_size);

    char je_label[16];
    get_next_label(je_label, sizeof(je_label));

    fprintf(
        tmp_file,
        "\tsection .text\n"
        "\t; codegen_read_into.\n"
        "\tmov eax, 0\n"
        "\tmov edi, 0\n"
        "\tmov esi, TMP + %lu\n"
        "\tmov edx, %u\n"
        "\tsyscall\n"
        // Check if we've read something. If so, remove the trailing new line.
        "\tcmp eax, 0\n"
        "\tje %s\n"
        "\tsub esi, 1\n"
        "\tadd esi, eax\n"
        "\tmov byte [esi], 0\n"
        "%s:\n",
        tmp_address,
        buffer_size,
        je_label,
        je_label);

    struct codegen_value_info info;

    switch (id_entry->symbol_type) {
        case SYMBOL_TYPE_INTEGER:
            info.address = read_int(tmp_address);
            break;
        case SYMBOL_TYPE_FLOATING_POINT:
            info.address = read_float(tmp_address);
            break;
        case SYMBOL_TYPE_LOGIC:
            info.address = read_logic(tmp_address);
            break;
        case SYMBOL_TYPE_CHAR:
        case SYMBOL_TYPE_STRING:
            info.address = tmp_address;
            break;
        default:
            UNREACHABLE();
    }

    info.type = id_entry->symbol_type;
    info.size = size_from_type(info.type);
    info.section = SYMBOL_SECTION_NONE;

    codegen_move_to_id_entry(id_entry, &info);
}
