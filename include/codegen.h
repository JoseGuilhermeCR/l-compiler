#ifndef CODEGEN_H_
#define CODEGEN_H_

struct code_generator
{
    int a;
};

int
codegen_init(struct code_generator *generator);

void
codegen_destro(struct code_generator *generator);

int
codegen_dump_to_file(struct code_generator *generator, const char *pathname);

#endif
