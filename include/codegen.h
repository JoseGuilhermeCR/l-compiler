#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "symbol_table.h"

#include <stdint.h>

struct codegen_constant;

struct code_generator
{
    struct codegen_constant *constants;
};

int
codegen_init(struct code_generator *generator);

void
codegen_destro(struct code_generator *generator);

int
codegen_dump_to_file(struct code_generator *generator, const char *pathname);

uint64_t
codegen_add_constant(struct code_generator *generator,
                     enum symbol_type type,
                     const char *value);

#endif
