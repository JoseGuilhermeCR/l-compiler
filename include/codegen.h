#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "symbol_table.h"

#include <stdint.h>

struct codegen_value_info
{
    uint64_t address;
    uint64_t size;
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
                  uint32_t lexeme_size,
                  struct codegen_value_info *info);

void
codegen_add_tmp(enum symbol_type type,
                const char *lexeme,
                uint32_t lexeme_size,
                struct codegen_value_info *info);

#endif
