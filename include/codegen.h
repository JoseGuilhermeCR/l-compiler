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
codegen_add_constant(enum symbol_type type,
                     uint8_t has_minus,
                     const char *value);

#endif
