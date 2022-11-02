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
codegen_write_text(const char *fmt, ...);

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
#endif
