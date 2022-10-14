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