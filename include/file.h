#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>

struct file
{
    char *buffer;
    uint32_t size;
};

int
read_file_from_stdin(struct file *file, uint32_t capacity);

void
destroy_stdin_file(struct file *file);

#endif