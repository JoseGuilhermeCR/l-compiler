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
codegen_init(const char *pathname);

void
codegen_dump(void);

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
