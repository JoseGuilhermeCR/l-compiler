/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
read_file_from_stdin(struct file *file, uint32_t capacity)
{
    const size_t total = (size_t)capacity + 1;

    file->size = 0;
    file->buffer = malloc(total);
    if (!file->buffer)
        return -1;

    char c = (char)fgetc(stdin);
    while (!feof(stdin) && file->size < capacity) {
        file->buffer[file->size++] = c;
        c = (char)fgetc(stdin);
    }
    file->buffer[file->size] = '\0';

    return 0;
}

int
read_file(struct file *file, const char *pathname)
{
    FILE *stream = fopen(pathname, "r");
    if (!stream)
        return -1;

    fseek(stream, 0, SEEK_END);
    uint64_t size = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    file->size = size;
    file->buffer = malloc(size);
    if (!file->buffer) {
        fclose(stream);
        return -1;
    }

    fread(file->buffer, 1, file->size, stream);

    fclose(stream);
    return 0;
}

void
destroy_file(struct file *file)
{
    free(file->buffer);
    file->buffer = NULL;
    file->size = 0;
}
