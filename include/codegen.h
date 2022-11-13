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

int
codegen_dump(const char *pathname,
             uint8_t keep_unoptimized,
             uint8_t assemble_and_link);

void
codegen_destroy(void);

void
codegen_reset_tmp(void);

void
codegen_add_unnit_value(enum symbol_type type, struct codegen_value_info *info);

void
codegen_add_value(enum symbol_type type,
                  enum symbol_class class,
                  uint8_t has_minus,
                  const char *lexeme,
                  struct codegen_value_info *info);

void
codegen_add_tmp(enum symbol_type type,
                const char *lexeme,
                struct codegen_value_info *info);

void
codegen_logic_negate(struct codegen_value_info *f);

void
codegen_convert_to_floating_point(struct codegen_value_info *info);

void
codegen_convert_to_integer(struct codegen_value_info *info);

void
codegen_perform_addition(struct codegen_value_info *exps_info,
                         const struct codegen_value_info *t_info);

void
codegen_perform_subtraction(struct codegen_value_info *exps_info,
                            const struct codegen_value_info *t_info);

void
codegen_perform_logical_or(struct codegen_value_info *exps_info,
                           const struct codegen_value_info *t_info);

void
codegen_negate(struct codegen_value_info *t_info);

void
codegen_perform_multiplication(struct codegen_value_info *t_info,
                               const struct codegen_value_info *f_info);

void
codegen_perform_division(struct codegen_value_info *t_info,
                         const struct codegen_value_info *f_info);

void
codegen_perform_integer_division(struct codegen_value_info *t_info,
                                 const struct codegen_value_info *f_info);

void
codegen_perform_mod(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info);

void
codegen_perform_and(struct codegen_value_info *t_info,
                    const struct codegen_value_info *f_info);

void
codegen_perform_comparison(enum token operation_tok,
                           struct codegen_value_info *exp_info,
                           const struct codegen_value_info *exps_info);

void
codegen_move_to_id_entry(struct symbol *id_entry,
                         const struct codegen_value_info *exp);

void
codegen_move_to_id_entry_idx(struct symbol *id_entry,
                             const struct codegen_value_info *exp,
                             const struct codegen_value_info *idx_expr_info);

void
codegen_write(const struct codegen_value_info *exp, uint8_t needs_new_line);

void
codegen_move_idx_to_tmp(const struct symbol *id_entry,
                        const struct codegen_value_info *idx_expr_info,
                        struct codegen_value_info *f_info);

void
codegen_start_loop(void);

void
codegen_eval_loop_expr(const struct codegen_value_info *exp);

void
codegen_finish_loop(void);

void
codegen_start_if(const struct codegen_value_info *exp);

void
codegen_if_jmp(void);

void
codegen_start_else(void);

void
codegen_finish_if(uint8_t had_else);

void
codegen_read_into(struct symbol *id_entry);

#endif
