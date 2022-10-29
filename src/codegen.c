#include "codegen.h"
#include "symbol_table.h"
#include "utils.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_VALUE_SIZE 256

struct codegen_constant
{
    enum symbol_type type;
    char value[MAX_VALUE_SIZE];
    struct codegen_constant *next;
};

static struct codegen_constant *constants;
static FILE *file;

static uint64_t
codegen_constant_init(struct codegen_constant *c,
                      enum symbol_type type,
                      const char *value)
{
    memset(c, 0, sizeof(*c));

    c->type = type;
    strncpy(c->value, value, MAX_VALUE_SIZE);

    // TODO: Ask generator for a new address.

    return 0;
}

static void
dump_template(FILE *file)
{
    time_t now = time(NULL);

    struct tm tm;
    localtime_r(&now, &tm);

    fprintf(file,
            "; Generated on %04u/%02u/%02u - %02u:%02u\n"
            "global _start\n"
            "section .text\n"
            "_start:\n",
            tm.tm_year + 1900,
            tm.tm_mon,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min);
}

static void
add_exit_syscall(FILE *file, uint8_t error_code)
{
    fprintf(file,
            "section .text\n"
            "\tmov rax, 60\n"
            "\tmov rsi, %u\n"
            "\tsyscall\n",
            error_code);
}

static void
dump_constants(FILE *file)
{
    if (!constants)
        return;

    fputs("section .rodata\n", file);

    const struct codegen_constant *c = constants;
    while (c) {
        switch (c->type) {
            case SYMBOL_TYPE_LOGIC:
            case SYMBOL_TYPE_CHAR:
            case SYMBOL_TYPE_STRING:
                fputs("\tdb ", file);
                break;
            case SYMBOL_TYPE_FLOATING_POINT:
            case SYMBOL_TYPE_INTEGER:
                fputs("\tdd ", file);
                break;
            default:
                UNREACHABLE();
        }

        if (c->type != SYMBOL_TYPE_STRING)
            fprintf(file, "%s\n", c->value);
        else
            fprintf(file, "%s,0\n", c->value);

        c = c->next;
    }
}

int
codegen_init(const char *pathname)
{
    file = fopen(pathname, "w");
    if (!file)
        return -1;

    dump_template(file);

    return 0;
}

void
codegen_dump(void)
{
    add_exit_syscall(file, 0);
    dump_constants(file);
}

void
codegen_destroy(void)
{
    // TODO(Jose): Free constants.
    if (file) {
        fclose(file);
        file = NULL;
    }
}

uint64_t
codegen_add_constant(enum symbol_type type, const char *value)
{
    if (!constants) {
        constants = malloc(sizeof(*constants));
        assert(constants && "failed to allocate memory for codegen_constant.");
        return codegen_constant_init(constants, type, value);
    }

    struct codegen_constant *previous = constants;
    struct codegen_constant *next = constants->next;
    while (next) {
        previous = next;
        next = next->next;
    }

    next = malloc(sizeof(*next));
    assert(next && "failed to allocate memory for codegen_constant.");

    previous->next = next;
    return codegen_constant_init(next, type, value);
}
