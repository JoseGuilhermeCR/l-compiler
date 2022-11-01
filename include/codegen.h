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

void
codegen_add_unnit_value(struct symbol *id_entry);

void
codegen_add_value(struct symbol *id_entry,
                  uint8_t has_minus,
                  const char *lexeme,
                  uint32_t lexeme_size);

#endif
