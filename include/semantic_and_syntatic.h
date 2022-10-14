#ifndef SEMANTIC_AND_SYNTATIC_H_
#define SEMANTIC_AND_SYNTATIC_H_

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
    struct lexical_entry *entry;
    uint8_t found_last_token;
};

void
syntatic_init(struct syntatic_ctx *ctx,
              struct lexer *lexer,
              struct lexical_entry *entry);

int
syntatic_start(struct syntatic_ctx *ctx);

#endif
