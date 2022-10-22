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

#ifndef TOKEN_H_
#define TOKEN_H_

enum token
{
    TOKEN_IDENTIFIER,             /* A */
    TOKEN_CONST,                  /* const */
    TOKEN_INT,                    /* int */
    TOKEN_CHAR,                   /* char */
    TOKEN_WHILE,                  /* while */
    TOKEN_IF,                     /* if */
    TOKEN_FLOAT,                  /* float */
    TOKEN_ELSE,                   /* else */
    TOKEN_READLN,                 /* readln */
    TOKEN_DIV,                    /* div */
    TOKEN_STRING,                 /* string */
    TOKEN_WRITE,                  /* write */
    TOKEN_WRITELN,                /* writeln*/
    TOKEN_MOD,                    /* mod */
    TOKEN_BOOLEAN,                /* boolean */
    TOKEN_CONSTANT,               /* 5.32F */
    TOKEN_LOGICAL_AND,            /* && */
    TOKEN_LOGICAL_OR,             /* || */
    TOKEN_NOT_EQUAL,              /* != */
    TOKEN_NOT,                    /* !  */
    TOKEN_ASSIGNMENT,             /* := */
    TOKEN_EQUAL,                  /* =  */
    TOKEN_COMMA,                  /* ,  */
    TOKEN_PLUS,                   /* +  */
    TOKEN_MINUS,                  /* -  */
    TOKEN_TIMES,                  /* *  */
    TOKEN_SEMICOLON,              /* ; */
    TOKEN_OPENING_PAREN,          /* ( */
    TOKEN_CLOSING_PAREN,          /* ) */
    TOKEN_OPENING_SQUARE_BRACKET, /* [ */
    TOKEN_CLOSING_SQUARE_BRACKET, /* ] */
    TOKEN_OPENING_CURLY_BRACKET,  /* { */
    TOKEN_CLOSING_CURLY_BRACKET,  /* } */
    TOKEN_LESS,                   /* < */
    TOKEN_LESS_EQUAL,             /* <= */
    TOKEN_GREATER,                /* > */
    TOKEN_GREATER_EQUAL,          /* >= */
    TOKEN_DIVISION                /* / */
};

#endif