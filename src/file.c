/*
 * This is free and unencumbered software released into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * Jose Guilherme de Castro Rodrigues - 2022 - 651201
 */

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
destroy_stdin_file(struct file *file)
{
    free(file->buffer);
    file->buffer = NULL;
    file->size = 0;
}
