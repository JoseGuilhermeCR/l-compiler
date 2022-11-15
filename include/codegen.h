/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "symbol_table.h"

#include <stdint.h>

struct codegen_value_info
{
    uint64_t address;
    uint64_t size;
    enum symbol_type type;
    enum symbol_section section;
};

int
codegen_init(void);

/*
 * Dumps all the assembly that has been generated to the file
 * at pathname.
 *
 * Optionally keeps the unoptimized version of the assembly and
 * assembles and link the final assembly into an executable by
 * using nasm and ld.
 * */
int
codegen_dump(const char *pathname,
             uint8_t keep_unoptimized,
             uint8_t assemble_and_link);

/*
 * Finishes the codegen process without generating any assembly.
 * */
void
codegen_destroy(void);

/*
 * Reset the temporary address counter. Should be used before processing
 * commands.
 * */
void
codegen_reset_tmp(void);

/*
 * Adds an unitialized value. It works more like a reservation: storage is
 * reserved according to the max size of the variable.
 *
 * Address and other information is stored in info.
 *
 * Variables that are not initialized are placed in .bss, which is then zeroed
 * by the operating system when loading the executable for execution, therefore
 * use of unitialized values is safe.
 * */
void
codegen_add_unnit_value(enum symbol_type type, struct codegen_value_info *info);

/*
 * Adds an initialized value.
 *
 * Address and other information is stored in info.
 *
 * Variables that are initialized are placed in .data.
 * Constants are placed in .rodata.
 * */
void
codegen_add_value(enum symbol_type type,
                  enum symbol_class class,
                  uint8_t has_minus,
                  const char *lexeme,
                  struct codegen_value_info *info);

/*
 * Adds an initialized value to temporary storage (first 64kB of .bss).
 * */
void
codegen_add_tmp(enum symbol_type type,
                const char *lexeme,
                struct codegen_value_info *info);

/*
 * Generates code to negate the boolean value at f.
 * */
void
codegen_logic_negate(struct codegen_value_info *f);

/*
 * Generates code to convert the integer at info to float.
 * */
void
codegen_convert_to_floating_point(struct codegen_value_info *info);

/*
 * Generates code to convert the float at info to integer.
 * */
void
codegen_convert_to_integer(struct codegen_value_info *info);

/*
 * Generates code to perform the addition of exps_info and t_info.
 * */
void
codegen_perform_addition(struct codegen_value_info *exps_info,
                         const struct codegen_value_info *t_info);

/*
 * Generates code to perform the subtraction of exps_info and t_info.
 * */
void
codegen_perform_subtraction(struct codegen_value_info *exps_info,
                            const struct codegen_value_info *t_info);

/*
 * Generates code to perform the logical or of exps_info and t_info.
 * */
void
codegen_perform_logical_or(struct codegen_value_info *exps_info,
                           const struct codegen_value_info *t_info);

/*
 * Generates code to negate the integer / float value in t_info.
 * */
void
codegen_negate(struct codegen_value_info *t_info);

/*
 * Generates code to perform the multiplication of t_info and f_info.
 * */
void
codegen_perform_multiplication(struct codegen_value_info *t_info,
                               const struct codegen_value_info *f_info);

/*
 * Generates code to perform the division of t_info and f_info.
 * */
void
codegen_perform_division(struct codegen_value_info *t_info,
                         const struct codegen_value_info *f_info);

/*
 * Generates code to perform the integer division of t_info and f_info.
 * */
void
codegen_perform_integer_division(struct codegen_value_info *t_info,
                                 const struct codegen_value_info *f_info);

/*
 * Generates code to perform the mod of t_info and f_info.
 * */
void
codegen_perform_mod(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info);

/*
 * Generates code to perform the logical and of t_info and f_info.
 * */
void
codegen_perform_and(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info);

/*
 * Generates code to perform a comparison between exp_info and exps_info.
 * The kind of comparison that'll be made depends on operation_tok.
 * */
void
codegen_perform_comparison(enum token operation_tok,
                           struct codegen_value_info *exp_info,
                           const struct codegen_value_info *exps_info);

/*
 * Generates code to move a value from an expression's temporary storage to
 * the storage of a variable.
 * */
void
codegen_move_to_id_entry(struct symbol *id_entry,
                         const struct codegen_value_info *exp);

/*
 * Generates code to move a value from an expression's temporary storage to
 * the storage of a variable at specific index.
 * */
void
codegen_move_to_id_entry_idx(struct symbol *id_entry,
                             const struct codegen_value_info *exp,
                             const struct codegen_value_info *idx_expr_info);

/*
 * Generates code to write an expression.
 * Uses the write syscall, with fd = 1 = stdout.
 * */
void
codegen_write(const struct codegen_value_info *exp, uint8_t needs_new_line);

/*
 * Generates code to move a value at an specific index of the variable to
 * temporary storage.
 * */
void
codegen_move_idx_to_tmp(const struct symbol *id_entry,
                        const struct codegen_value_info *idx_expr_info,
                        struct codegen_value_info *f_info);

/*
 * Generates code to start a loop.
 * */
void
codegen_start_loop(void);

/*
 * Generates code to evaluate a loop's expression.
 * */
void
codegen_eval_loop_expr(const struct codegen_value_info *exp);

/*
 * Generates code to finish a loop.
 * */
void
codegen_finish_loop(void);

/*
 * Generates code start an if.
 * */
void
codegen_start_if(const struct codegen_value_info *exp);

/*
 * Generates code to perform the if jmp.
 * */
void
codegen_if_jmp(void);

/*
 * Generates code to start an else.
 * */
void
codegen_start_else(void);

/*
 * Generates code to finish an if / else.
 * */
void
codegen_finish_if(uint8_t had_else);

/*
 * Reads a value into a variable.
 * Uses read syscall, with fd = 0 = stdin.
 * */
void
codegen_read_into(struct symbol *id_entry);

#endif
