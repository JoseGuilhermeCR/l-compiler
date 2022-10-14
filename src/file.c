#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
read_file_from_stdin(struct file *file, uint32_t capacity)
{
    file->size = 0;
    file->buffer = malloc(capacity + 1);
    if (!file->buffer)
        return -1;

    while (file->size < capacity && !feof(stdin))
        file->buffer[file->size++] = (char)fgetc(stdin);
    file->buffer[file->size] = '\0';

    // Removing <CR><LF> from files.
    char *ending = NULL;
    if (strstr(file->buffer, "\r\n")) {
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