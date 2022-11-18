/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#ifndef SEMANTIC_AND_SYNTATIC_H_
#define SEMANTIC_AND_SYNTATIC_H_

#include "codegen.h"
#include "lexer.h"

#include <stdint.h>

enum syntatic_result
{
    SYNTATIC_OK,
    SYNTATIC_ERROR,
};

struct syntatic_ctx
{
    struct lexer *lexer;
    struct lexical_entry entry;
    struct lexical_entry last_entry;
    uint8_t found_last_token;
};

/*
 * Initializes the syntatic_ctx structure.
 * */
void
syntatic_init(struct syntatic_ctx *ctx, struct lexer *lexer);

/*
 * Kickstarts the syntatic analysis.
 *
 * If it's successfull, this function returns 0.
 * */
int
syntatic_start(struct syntatic_ctx *ctx);

#endif
