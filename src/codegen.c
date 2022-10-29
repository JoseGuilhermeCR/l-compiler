#include "codegen.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

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

int
codegen_init(struct code_generator *generator)
{
    return 0;
}

void
codegen_destroy(struct code_generator *generator)
{
}

int
codegen_dump_to_file(struct code_generator *generator, const char *pathname)
{
    FILE *file = fopen(pathname, "w");
    if (!file) {
        // TODO(Jose): Report error.
        return -1;
    }

    dump_template(file);
    add_exit_syscall(file, 0);

    fclose(file);
    return 0;
}
