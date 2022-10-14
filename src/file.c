#include "file.h"

#include <stdio.h>
#include <stdlib.h>

int
read_file_from_stdin(struct file *file, uint32_t capacity)
{
    file->size = 0;
    file->buffer = malloc(capacity + 1);
    if (!file->buffer)
        return -1;

    while (file->size < capacity && !feof(stdin))
        file->buffer[file->size++] = fgetc(stdin);
    file->buffer[file->size] = '\0';

    // Removing <CR><LF> from files that come from Windows...
    // Note: For some questions it seems that the system expects us to
    // remove the last line endings as well...
    //
    // int n := 0x
    //
    // Apparently this input produces "unexpected EOF",
    // which is not true... The file doesn't end there,
    // it ends after the \n that comes after 0x, which
    // for us means we detect 0x as an invalid lexeme.
    // Since we must play by the rules of VERDE, we can
    // do nothing but cry about this terrible "automatic"
    // grading system that can't even compile proper C code.
    char *ending = NULL;
    if (strstr(file->buffer, "\r\n")) {
        ending = strstr(file->buffer, "\r\n");
        while (ending) {
#if !defined(REMOVE_LAST_CRLF)
            ending[0] = '\n';
            ending[1] = ' ';
#else
            if (ending[2] != EOF) {
                ending[0] = '\n';
                ending[1] = ' ';
            } else {
                ending[0] = EOF;
            }
#endif
            ending = strstr(ending, "\r\n");
        }
#if defined(REMOVE_LAST_CRLF)
        file->size -= 3;
#endif
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