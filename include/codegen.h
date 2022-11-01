#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "symbol_table.h"

#include <stdint.h>

int
codegen_init(const char *pathname);

void
codegen_dump(void);

void
codegen_destroy(void);

void
codegen_write_text(const char *fmt, ...);

uint64_t
codegen_add_unnit_value(enum symbol_type type);

uint64_t
codegen_add_value(enum symbol_type type,
                  uint8_t has_minus,
                  const char *lexeme,
                  uint32_t lexeme_size);

#endif
