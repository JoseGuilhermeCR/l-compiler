/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>

struct file
{
    char *buffer;
    uint32_t size;
};

/*
 * Reads input from stdin. Since we can't know the size of the input
 * beforehand, a capacity needs to be specified for the allocated buffer.
 * */
int
read_file_from_stdin(struct file *file, uint32_t capacity);

/*
 * Reads a file at pathname.
 * */
int
read_file(struct file *file, const char *pathname);

/*
 * Destroys the file by deallocating it's buffer.
 * */
void
destroy_file(struct file *file);

#endif
