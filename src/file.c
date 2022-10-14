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

    // Removing <CR><LF> from files.
    char *ending = NULL;
    if (strstr(file->buffer, "\r\n")) {
        fputs("Processing \\r\\n...\n", ERR_STREAM);
        ending = strstr(file->buffer, "\r\n");
        while (ending) {
            ending[0] = '\n';
            ending[1] = ' ';
            ending = strstr(ending, "\r\n");
        }
    }

    return 0;
}

void
destroy_stdin_file(struct file *file)
{
    free(file->buffer);
    file->buffer = NULL;
    file->size = 0;
}