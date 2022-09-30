#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if !defined(VERDE)
#define ERR_STREAM stderr
#else
#define ERR_STREAM stdout
#endif

#define MAX_FILE_SIZE (32U * 1024U)
#define MAX_LEXEME_SIZE (32U)

#define SYNTATIC_ERROR()                                                       \
    do {                                                                       \
        fprintf(ERR_STREAM,                                                    \
                "SYNTATIC_ERROR FUNCTION %s LINE %i\n",                        \
                __func__,                                                      \
                __LINE__);                                                     \
    } while (0)

#if defined(ASSERT_UNREACHABLE)
#define UNREACHABLE() do { assert(0); } while (0)
#else
#define UNREACHABLE()
#endif

enum token
{
    TOKEN_IDENTIFIER, /* A */
    TOKEN_CONST,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_WHILE,
    TOKEN_IF,
    TOKEN_FLOAT,
    TOKEN_ELSE,
    TOKEN_READLN,
    TOKEN_DIV,
    TOKEN_STRING,
    TOKEN_WRITE,
    TOKEN_WRITELN,
    TOKEN_MOD,
    TOKEN_BOOLEAN,
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

// Main Stuff

struct stdin_file
{
    char *buffer;
    uint32_t size;
};

// Symbol Table Stuff

struct symbol
{
    char lexeme[MAX_LEXEME_SIZE + 1];
    enum token token;
    struct symbol *next;
};

struct symbol_table
{
    struct symbol *symbols;
    uint32_t capacity;
};

static uint32_t
hash_symbol_lexeme(const char *lexeme)
{
    // TODO(Jose): Make a better hashing function.
    uint32_t sum = 0;
    while (*lexeme)
        sum += *lexeme++;
    return sum;
}

static int
symbol_table_create(struct symbol_table *table, uint32_t capacity)
{
    table->capacity = capacity;
#if !defined(VERDE)
    table->symbols = calloc(capacity, sizeof(*table->symbols));
#else
    table->symbols = (struct symbol *)calloc(capacity, sizeof(*table->symbols));
#endif
    if (!table->symbols)
        return -1;
    return 0;
}

static struct symbol *
symbol_table_search(struct symbol_table *table, const char *lexeme)
{
    assert(strlen(lexeme) <= MAX_LEXEME_SIZE &&
           "lexeme bigger than MAX_LEXEME_SIZE");

    const uint32_t idx = hash_symbol_lexeme(lexeme) % table->capacity;

    struct symbol *s = &table->symbols[idx];
    if (s->lexeme[0] == '\0')
        return NULL;

    while (s) {
        if (strncasecmp(s->lexeme, lexeme, MAX_LEXEME_SIZE) == 0)
            return s;
        s = s->next;
    }

    return NULL;
}

static int
symbol_table_insert(struct symbol_table *table,
                    const char *lexeme,
                    enum token token)
{
    assert(strlen(lexeme) <= MAX_LEXEME_SIZE &&
           "lexeme bigger than MAX_LEXEME_SIZE");

    const uint32_t idx = hash_symbol_lexeme(lexeme) % table->capacity;

    struct symbol *s = &table->symbols[idx];
    // Empty entry, great!
    if (s->lexeme[0] == '\0') {
        strncpy(s->lexeme, lexeme, MAX_LEXEME_SIZE);
        s->token = token;
        return 0;
    }

    // Collision, go to the end of the symbols while checking if
    // any of them is already what we are trying to insert, so
    // that we don't insert any duplicates in the table.
    if (strncasecmp(s->lexeme, lexeme, MAX_LEXEME_SIZE) == 0)
        return -1;

    struct symbol *previous = s;
    struct symbol *next = s->next;
    while (next) {
        if (strncasecmp(next->lexeme, lexeme, MAX_LEXEME_SIZE) == 0)
            return -1;
        previous = next;
        next = next->next;
    }

#if !defined(VERDE)
    next = malloc(sizeof(*next));
#else
    next = (struct symbol *)malloc(sizeof(*next));
#endif
    assert(next && "failed to allocate memory for new symbol.");

    strncpy(next->lexeme, lexeme, MAX_LEXEME_SIZE);
    next->token = token;
    next->next = NULL;

    previous->next = next;
    return 0;
}

static void
symbol_table_populate_with_keywords(struct symbol_table *table)
{
    assert(symbol_table_insert(table, "const", TOKEN_CONST) == 0);
    assert(symbol_table_insert(table, "int", TOKEN_INT) == 0);
    assert(symbol_table_insert(table, "char", TOKEN_CHAR) == 0);
    assert(symbol_table_insert(table, "while", TOKEN_WHILE) == 0);
    assert(symbol_table_insert(table, "if", TOKEN_IF) == 0);
    assert(symbol_table_insert(table, "float", TOKEN_FLOAT) == 0);
    assert(symbol_table_insert(table, "else", TOKEN_ELSE) == 0);
    assert(symbol_table_insert(table, "readln", TOKEN_READLN) == 0);
    assert(symbol_table_insert(table, "div", TOKEN_DIV) == 0);
    assert(symbol_table_insert(table, "string", TOKEN_STRING) == 0);
    assert(symbol_table_insert(table, "write", TOKEN_WRITE) == 0);
    assert(symbol_table_insert(table, "writeln", TOKEN_WRITELN) == 0);
    assert(symbol_table_insert(table, "mod", TOKEN_MOD) == 0);
    assert(symbol_table_insert(table, "boolean", TOKEN_BOOLEAN) == 0);
}

static void
symbol_table_destroy(struct symbol_table *table)
{
    for (uint32_t i = 0; i < table->capacity; ++i) {
        struct symbol *s = &table->symbols[i];
        if (s->lexeme[0] == '\0')
            continue;

        s = s->next;
        while (s) {
            struct symbol *tmp = s->next;
            free(s);
            s = tmp;
        }
    }

    free(table->symbols);
    table->symbols = NULL;
    table->capacity = 0;
}

// Lexer Stuff

enum lexer_state
{
    LEXER_STATE_INITIAL,
    LEXER_STATE_KEYWORD_OR_IDENTIFIER,    /* 1 */
    LEXER_STATE_LOGICAL_AND,              /* 3 */
    LEXER_STATE_LOGICAL_OR,               /* 4 */
    LEXER_STATE_NOT_OR_NOT_EQUAL,         /* 5 */
    LEXER_STATE_ASSIGNMENT,               /* 6 */
    LEXER_STATE_LESS_OR_LESS_EQUAL,       /* 7 */
    LEXER_STATE_GREATER_OR_GREATER_EQUAL, /* 8 */
    LEXER_STATE_DIVISION_OR_COMENTARY,    /* 9 */
    LEXER_STATE_IN_COMMENTARY,            /* 10 */
    LEXER_STATE_LEAVING_COMMENTARY,       /* 11 */
    LEXER_STATE_ENTER_CHAR_CONSTANT,      /* 14 */
    LEXER_STATE_LEAVE_CHAR_CONSTANT,      /* 16 */
    LEXER_STATE_STRING_CONSTANT,          /* 13 */
    LEXER_STATE_CHAR_OR_NUMBER,           /* 17 */
    LEXER_STATE_ENTER_HEXADECIMAL_CHAR,   /* 18 */
    LEXER_STATE_HEXADECIMAL_CHAR,         /* 19 */
    LEXER_STATE_INTEGER_OR_FLOAT,         /* 12 */
    LEXER_STATE_START_FLOAT,              /* 15 */
    LEXER_STATE_FLOAT                     /* 20 */
};

enum lexer_result
{
    LEXER_RESULT_FOUND,
    LEXER_RESULT_EMPTY,
    LEXER_RESULT_ERROR
};

enum lexer_error
{
    LEXER_ERROR_NONE,
    LEXER_ERROR_UNEXPECTED_EOF,
    LEXER_ERROR_INVALID_LEXEME,
    LEXER_ERROR_INVALID_CHARACTER
};

enum constant_type
{
    CONSTANT_TYPE_BOOLEAN,
    CONSTANT_TYPE_CHAR,
    CONSTANT_TYPE_STRING,
    CONSTANT_TYPE_INTEGER,
    CONSTANT_TYPE_FLOAT
};

struct lexeme
{
    char buffer[MAX_LEXEME_SIZE + 1];
    uint32_t size;
};

struct lexer
{
    const struct stdin_file *file;
    struct symbol_table *symbol_table;
    enum lexer_state state;
    enum lexer_error error;
    struct lexeme lexeme;
    uint32_t cursor;
    uint32_t line;
};

struct lexical_entry
{
    enum token token;
    struct lexeme lexeme;
    enum constant_type constant_type;
};

// Lexer Stuff
static void
lexeme_append_or_error(struct lexeme *l, char c)
{
    l->buffer[l->size++] = c;
    assert(l->size <= sizeof(l->buffer) && "Lexeme buffer overflow.");
}

static int
is_in_alphabet(char c)
{
    if (isalnum(c) || c == EOF)
        return 1;
    const char *extra = " _.,;:()[]{}+*-\"'/|@&%!?><=\n\e";
    return strchr(extra, c) != NULL;
}

static void
lexer_init(struct lexer *lexer,
           const struct stdin_file *file,
           struct symbol_table *table)
{
    lexer->file = file;
    lexer->symbol_table = table;
    lexer->error = LEXER_ERROR_NONE;
    lexer->cursor = 0;
    lexer->line = 1;
}

static enum lexer_result
lexer_get_next_token(struct lexer *lexer, struct lexical_entry *entry)
{
    if (lexer->cursor >= lexer->file->size)
        return LEXER_RESULT_EMPTY;

    memset(&entry->lexeme, 0, sizeof(entry->lexeme));

    lexer->state = LEXER_STATE_INITIAL;
    while (lexer->cursor != lexer->file->size) {
        const char c = lexer->file->buffer[lexer->cursor++];

        if (c == '\n')
            ++lexer->line;

        if (!is_in_alphabet(c)) {
            lexer->error = LEXER_ERROR_INVALID_CHARACTER;
            return LEXER_RESULT_ERROR;
        }

        switch (lexer->state) {
            case LEXER_STATE_INITIAL:
                memset(&lexer->lexeme, 0, sizeof(lexer->lexeme));

                if (c == ' ' || c == '\n')
                    break;

                lexeme_append_or_error(&lexer->lexeme, c);

                if (isalpha(c) || c == '_') {
                    lexer->state = LEXER_STATE_KEYWORD_OR_IDENTIFIER;
                    break;
                } else if (isdigit(c) && c != '0') {
                    // 0 is not treated here, because 0 might be the start
                    // of a hexadecimal character.
                    lexer->state = LEXER_STATE_INTEGER_OR_FLOAT;
                    break;
                }

                switch (c) {
                    case '&':
                        lexer->state = LEXER_STATE_LOGICAL_AND;
                        break;
                    case '|':
                        lexer->state = LEXER_STATE_LOGICAL_OR;
                        break;
                    case '!':
                        lexer->state = LEXER_STATE_NOT_OR_NOT_EQUAL;
                        break;
                    case ':':
                        lexer->state = LEXER_STATE_ASSIGNMENT;
                        break;
                    case '=':
                        entry->token = TOKEN_EQUAL;
                        return LEXER_RESULT_FOUND;
                    case ',':
                        entry->token = TOKEN_COMMA;
                        return LEXER_RESULT_FOUND;
                    case '+':
                        entry->token = TOKEN_PLUS;
                        return LEXER_RESULT_FOUND;
                    case '-':
                        entry->token = TOKEN_MINUS;
                        return LEXER_RESULT_FOUND;
                    case '*':
                        entry->token = TOKEN_TIMES;
                        return LEXER_RESULT_FOUND;
                    case ';':
                        entry->token = TOKEN_SEMICOLON;
                        return LEXER_RESULT_FOUND;
                    case '(':
                        entry->token = TOKEN_OPENING_PAREN;
                        return LEXER_RESULT_FOUND;
                    case ')':
                        entry->token = TOKEN_CLOSING_PAREN;
                        return LEXER_RESULT_FOUND;
                    case '[':
                        entry->token = TOKEN_OPENING_SQUARE_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case ']':
                        entry->token = TOKEN_CLOSING_SQUARE_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '{':
                        entry->token = TOKEN_OPENING_CURLY_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '}':
                        entry->token = TOKEN_CLOSING_CURLY_BRACKET;
                        return LEXER_RESULT_FOUND;
                    case '<':
                        lexer->state = LEXER_STATE_LESS_OR_LESS_EQUAL;
                        break;
                    case '>':
                        lexer->state = LEXER_STATE_GREATER_OR_GREATER_EQUAL;
                        break;
                    case '/':
                        lexer->state = LEXER_STATE_DIVISION_OR_COMENTARY;
                        break;
                    case '\'':
                        lexer->state = LEXER_STATE_ENTER_CHAR_CONSTANT;
                        break;
                    case '"':
                        lexer->state = LEXER_STATE_STRING_CONSTANT;
                        break;
                    case '0':
                        lexer->state = LEXER_STATE_CHAR_OR_NUMBER;
                        break;
                    case '.':
                        lexer->state = LEXER_STATE_START_FLOAT;
                        break;
                }

                break;
            case LEXER_STATE_KEYWORD_OR_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;

                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));

                    const struct symbol *s = symbol_table_search(
                        lexer->symbol_table, lexer->lexeme.buffer);
                    if (s) {
                        entry->token = s->token;
                        return LEXER_RESULT_FOUND;
                    }

                    const int error = symbol_table_insert(lexer->symbol_table,
                                                          lexer->lexeme.buffer,
                                                          TOKEN_IDENTIFIER);
                    assert(error == 0 &&
                           "symbol_table_insert cannot fail at this point.");

                    entry->token = TOKEN_IDENTIFIER;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_LOGICAL_AND:
                if (c == '&') {
                    entry->token = TOKEN_LOGICAL_AND;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LOGICAL_OR:
                if (c == '|') {
                    entry->token = TOKEN_LOGICAL_OR;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_NOT_OR_NOT_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_NOT_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_NOT;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_ASSIGNMENT:
                if (c == '=') {
                    entry->token = TOKEN_ASSIGNMENT;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LESS_OR_LESS_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_LESS_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_LESS;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_GREATER_OR_GREATER_EQUAL:
                if (c == '=') {
                    entry->token = TOKEN_GREATER_EQUAL;
                    return LEXER_RESULT_FOUND;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_GREATER;
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_DIVISION_OR_COMENTARY:
                if (c != '*') {
                    --lexer->cursor;
                    entry->token = TOKEN_DIVISION;
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->state = LEXER_STATE_IN_COMMENTARY;
                }
                break;
            case LEXER_STATE_IN_COMMENTARY:
                if (c == '*')
                    lexer->state = LEXER_STATE_LEAVING_COMMENTARY;
                break;
            case LEXER_STATE_LEAVING_COMMENTARY:
                if (c == '/')
                    lexer->state = LEXER_STATE_INITIAL;
                else if (c != '*')
                    lexer->state = LEXER_STATE_IN_COMMENTARY;
                break;
            case LEXER_STATE_ENTER_CHAR_CONSTANT:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isprint(c)) {
                    lexer->state = LEXER_STATE_LEAVE_CHAR_CONSTANT;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_LEAVE_CHAR_CONSTANT:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (c == '\'') {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_STRING_CONSTANT:
                if (c == '\n') {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }

                lexeme_append_or_error(&lexer->lexeme, c);
                if (c == '"') {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_CHAR_OR_NUMBER:
                if (tolower(c) == 'x') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_ENTER_HEXADECIMAL_CHAR;
                } else if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_INTEGER_OR_FLOAT;
                } else if (c == '.') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_FLOAT;
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_INTEGER;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_ENTER_HEXADECIMAL_CHAR:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isxdigit(c)) {
                    lexer->state = LEXER_STATE_HEXADECIMAL_CHAR;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_HEXADECIMAL_CHAR:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isxdigit(c)) {
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_CHAR;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                lexer->error = LEXER_ERROR_INVALID_LEXEME;
                return LEXER_RESULT_ERROR;
            case LEXER_STATE_INTEGER_OR_FLOAT:
                if (c == '.') {
                    lexeme_append_or_error(&lexer->lexeme, c);
                    lexer->state = LEXER_STATE_FLOAT;
                } else if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_INTEGER;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
            case LEXER_STATE_START_FLOAT:
                lexeme_append_or_error(&lexer->lexeme, c);
                if (isdigit(c)) {
                    lexer->state = LEXER_STATE_FLOAT;
                } else {
                    lexer->error = LEXER_ERROR_INVALID_LEXEME;
                    return LEXER_RESULT_ERROR;
                }
                break;
            case LEXER_STATE_FLOAT:
                if (isdigit(c)) {
                    lexeme_append_or_error(&lexer->lexeme, c);
                } else {
                    --lexer->cursor;
                    entry->token = TOKEN_CONSTANT;
                    entry->constant_type = CONSTANT_TYPE_FLOAT;
                    memcpy(
                        &entry->lexeme, &lexer->lexeme, sizeof(struct lexeme));
                    return LEXER_RESULT_FOUND;
                }
                break;
        }
    }

    if (lexer->state != LEXER_STATE_INITIAL) {
        lexer->error = LEXER_ERROR_UNEXPECTED_EOF;
        return LEXER_RESULT_ERROR;
    }
    return LEXER_RESULT_EMPTY;
}

static void
lexer_print_error(const struct lexer *lexer)
{
    assert(lexer->error != LEXER_ERROR_NONE);

    fprintf(ERR_STREAM, "%i\n", lexer->line);
    switch (lexer->error) {
        case LEXER_ERROR_UNEXPECTED_EOF:
            fputs("fim de arquivo nao esperado.\n", ERR_STREAM);
            break;
        case LEXER_ERROR_INVALID_LEXEME:
            fprintf(ERR_STREAM,
                    "lexema nao identificado [%s].\n",
                    lexer->lexeme.buffer);
            break;
        case LEXER_ERROR_INVALID_CHARACTER:
            fputs("caractere invalido.\n", ERR_STREAM);
            break;
        default:
            UNREACHABLE();
    }
}

// Syntatic Stuff

enum syntatic_match_result
{
    SYNTATIC_MATCH_OK,
    SYNTATIC_MATCH_ERROR,
};

struct syntatic_ctx
{
    struct lexer *lexer;
    struct lexical_entry *entry;
};

static enum syntatic_match_result
syntatic_match_token(struct syntatic_ctx *ctx, enum token token)
{
    if (ctx->entry->token == token) {
        enum lexer_result result = lexer_get_next_token(ctx->lexer, ctx->entry);
        if (result == LEXER_RESULT_ERROR) {
            lexer_print_error(ctx->lexer);
            return SYNTATIC_MATCH_ERROR;
        } else if (result == LEXER_RESULT_EMPTY) {
            goto syntatic_error;
        }

        return SYNTATIC_MATCH_OK;
    }

syntatic_error:
    // TODO(Jose): Print a syntatic error.
    fprintf(ERR_STREAM, "SYNTATIC_ERROR\n");
    return SYNTATIC_MATCH_ERROR;
}

static int
syntatic_decl_var(struct syntatic_ctx *ctx)
{
    if (syntatic_match_token(ctx, TOKEN_IDENTIFIER) != SYNTATIC_MATCH_OK)
        return -1;

    // Handle possible assignment for the first declared variable.
    if (ctx->entry->token == TOKEN_ASSIGNMENT) {
        if (syntatic_match_token(ctx, TOKEN_ASSIGNMENT) != SYNTATIC_MATCH_OK)
            return -1;

        if (ctx->entry->token == TOKEN_MINUS) {
            if (syntatic_match_token(ctx, TOKEN_MINUS) != SYNTATIC_MATCH_OK)
                return -1;

            if (syntatic_match_token(ctx, TOKEN_CONSTANT) != SYNTATIC_MATCH_OK)
                return -1;
        } else if (syntatic_match_token(ctx, TOKEN_CONSTANT) !=
                   SYNTATIC_MATCH_OK) {
            return -1;
        }
    }

    if (ctx->entry->token == TOKEN_SEMICOLON) {
        if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
            return -1;
        puts("decl_var_success");
        return 0;
    } else if (ctx->entry->token != TOKEN_COMMA) {
        puts("not semicolon nor comma");
        return -1;
    }

    // Handle multiple variable declaration.
    while (ctx->entry->token == TOKEN_COMMA) {
        if (syntatic_match_token(ctx, TOKEN_COMMA) != SYNTATIC_MATCH_OK)
            return -1;

        if (syntatic_match_token(ctx, TOKEN_IDENTIFIER) != SYNTATIC_MATCH_OK)
            return -1;

        // Handle possible assignment for variable.
        if (ctx->entry->token == TOKEN_ASSIGNMENT) {
            if (syntatic_match_token(ctx, TOKEN_ASSIGNMENT) !=
                SYNTATIC_MATCH_OK)
                return -1;

            if (ctx->entry->token == TOKEN_MINUS) {
                if (syntatic_match_token(ctx, TOKEN_MINUS) != SYNTATIC_MATCH_OK)
                    return -1;

                if (syntatic_match_token(ctx, TOKEN_CONSTANT) !=
                    SYNTATIC_MATCH_OK)
                    return -1;
            } else if (syntatic_match_token(ctx, TOKEN_CONSTANT) !=
                       SYNTATIC_MATCH_OK) {
                return -1;
            }
        }
    }

    if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
        return -1;

    puts("decl_var_success");
    return 0;
}

static int
syntatic_decl_const(struct syntatic_ctx *ctx)
{
    if (syntatic_match_token(ctx, TOKEN_IDENTIFIER) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_EQUAL) != SYNTATIC_MATCH_OK)
        return -1;

    if (ctx->entry->token == TOKEN_MINUS) {
        if (syntatic_match_token(ctx, TOKEN_MINUS) != SYNTATIC_MATCH_OK)
            return -1;

        if (syntatic_match_token(ctx, TOKEN_CONSTANT) != SYNTATIC_MATCH_OK)
            return -1;
    } else if (syntatic_match_token(ctx, TOKEN_CONSTANT) != SYNTATIC_MATCH_OK) {
        return -1;
    }

    if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
        return -1;

    puts("decl_const_success");
    return 0;
}

static int
syntatic_read(struct syntatic_ctx *ctx)
{
    if (syntatic_match_token(ctx, TOKEN_OPENING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_IDENTIFIER) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_CLOSING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
        return -1;

    puts("readln_success");
    return 0;
}

static int
syntatic_is_first_of_f(struct syntatic_ctx *ctx)
{
    switch (ctx->entry->token) {
        case TOKEN_NOT:
        case TOKEN_OPENING_PAREN:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_CONSTANT:
        case TOKEN_IDENTIFIER:
            return 1;
        default:
            return 0;
    }

    UNREACHABLE();
}

static int
syntatic_exp(struct syntatic_ctx *ctx);

static int
syntatic_f(struct syntatic_ctx *ctx)
{
    if (!syntatic_is_first_of_f(ctx)) {
        SYNTATIC_ERROR();
        return -1;
    }

    enum token tok = ctx->entry->token;
    if (syntatic_match_token(ctx, tok) != SYNTATIC_MATCH_OK)
        return -1;

    switch (tok) {
        case TOKEN_NOT:
            if (syntatic_f(ctx) < 0)
                return -1;
            break;
        case TOKEN_OPENING_PAREN:
            if (syntatic_exp(ctx) < 0)
                return -1;
            if (syntatic_match_token(ctx, TOKEN_CLOSING_PAREN) !=
                SYNTATIC_MATCH_OK)
                return -1;
            break;
        case TOKEN_INT:
        case TOKEN_FLOAT:
            if (syntatic_match_token(ctx, TOKEN_OPENING_PAREN) !=
                SYNTATIC_MATCH_OK)
                return -1;
            if (syntatic_exp(ctx) < 0)
                return -1;
            if (syntatic_match_token(ctx, TOKEN_CLOSING_PAREN) !=
                SYNTATIC_MATCH_OK)
                return -1;
            break;
        case TOKEN_CONSTANT:
            break;
        case TOKEN_IDENTIFIER:
            if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
                if (syntatic_match_token(ctx, TOKEN_OPENING_SQUARE_BRACKET) !=
                    SYNTATIC_MATCH_OK)
                    return -1;
                if (syntatic_exp(ctx) < 0)
                    return -1;
                if (syntatic_match_token(ctx, TOKEN_CLOSING_SQUARE_BRACKET) !=
                    SYNTATIC_MATCH_OK)
                    return -1;
            }
            break;
        default:
            UNREACHABLE();
    }

    puts("f_success");
    return 0;
}

static int
syntatic_t(struct syntatic_ctx *ctx)
{
    if (syntatic_f(ctx) < 0)
        return -1;

    enum token tok = ctx->entry->token;
    while (tok == TOKEN_TIMES || tok == TOKEN_LOGICAL_AND || tok == TOKEN_MOD ||
           tok == TOKEN_DIV || tok == TOKEN_DIVISION) {
        if (syntatic_match_token(ctx, tok) != SYNTATIC_MATCH_OK)
            return -1;

        if (syntatic_f(ctx) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    puts("t_success");
    return 0;
}

static int
syntatic_exps(struct syntatic_ctx *ctx)
{
    enum token tok = ctx->entry->token;
    if (tok == TOKEN_MINUS || tok == TOKEN_PLUS) {
        if (syntatic_match_token(ctx, ctx->entry->token) != SYNTATIC_MATCH_OK)
            return -1;
    }

    if (syntatic_t(ctx) < 0)
        return -1;

    tok = ctx->entry->token;
    while (tok == TOKEN_PLUS || tok == TOKEN_MINUS || tok == TOKEN_LOGICAL_OR) {
        if (syntatic_match_token(ctx, tok) != SYNTATIC_MATCH_OK)
            return -1;

        if (syntatic_t(ctx) < 0)
            return -1;

        tok = ctx->entry->token;
    }

    puts("exps_success");
    return 0;
}

static int
syntatic_exp(struct syntatic_ctx *ctx)
{
    if (syntatic_exps(ctx) < 0)
        return -1;

    switch (ctx->entry->token) {
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
            if (syntatic_match_token(ctx, ctx->entry->token) !=
                SYNTATIC_MATCH_OK)
                return -1;

            if (syntatic_exps(ctx) < 0)
                return -1;
            break;
        default:
            break;
    }

    puts("exp_success");
    return 0;
}

static int
syntatic_write(struct syntatic_ctx *ctx)
{
    if (syntatic_match_token(ctx, TOKEN_OPENING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_exp(ctx) < 0)
        return -1;

    while (ctx->entry->token == TOKEN_COMMA) {
        if (syntatic_match_token(ctx, TOKEN_COMMA) != SYNTATIC_MATCH_OK)
            return -1;
        if (syntatic_exp(ctx) < 0)
            return -1;
    }

    if (syntatic_match_token(ctx, TOKEN_CLOSING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
        return -1;

    puts("write_success");
    return 0;
}

static int
syntatic_attr(struct syntatic_ctx *ctx)
{
    if (ctx->entry->token == TOKEN_OPENING_SQUARE_BRACKET) {
        if (syntatic_match_token(ctx, TOKEN_OPENING_SQUARE_BRACKET) !=
            SYNTATIC_MATCH_OK)
            return -1;

        if (syntatic_exp(ctx) < 0)
            return -1;

        if (syntatic_match_token(ctx, TOKEN_CLOSING_SQUARE_BRACKET) !=
            SYNTATIC_MATCH_OK)
            return -1;
    }

    if (syntatic_match_token(ctx, TOKEN_ASSIGNMENT) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_exp(ctx) < 0)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_SEMICOLON) != SYNTATIC_MATCH_OK)
        return -1;

    puts("attr_ok");
    return 0;
}

static int
syntatic_paren_exp(struct syntatic_ctx *ctx)
{
    if (syntatic_match_token(ctx, TOKEN_OPENING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    if (syntatic_exp(ctx) < 0)
        return -1;

    if (syntatic_match_token(ctx, TOKEN_CLOSING_PAREN) != SYNTATIC_MATCH_OK)
        return -1;

    return 0;
}

static int
syntatic_is_first_of_command(struct syntatic_ctx *ctx)
{
    switch (ctx->entry->token) {
        case TOKEN_SEMICOLON:
        case TOKEN_IDENTIFIER:
        case TOKEN_WHILE:
        case TOKEN_IF:
        case TOKEN_READLN:
        case TOKEN_WRITE:
        case TOKEN_WRITELN:
            return 1;
        default:
            return 0;
    }

    UNREACHABLE();
}

static int
syntatic_while(struct syntatic_ctx *ctx);

static int
syntatic_command(struct syntatic_ctx *ctx)
{
    enum token tok = ctx->entry->token;
    if (syntatic_match_token(ctx, tok) != SYNTATIC_MATCH_OK)
        return -1;

    switch (tok) {
        case TOKEN_SEMICOLON:
            break;
        case TOKEN_WRITE:
        case TOKEN_WRITELN:
            if (syntatic_write(ctx) < 0)
                return -1;
            break;
        case TOKEN_READLN:
            if (syntatic_read(ctx) < 0)
                return -1;
            break;
        case TOKEN_IDENTIFIER:
            if (syntatic_attr(ctx) < 0)
                return -1;
            break;
        case TOKEN_WHILE:
            if (syntatic_while(ctx) < 0)
                return -1;
            break;
        default:
            UNREACHABLE();
    }

    return 0;
}

static int
syntatic_while(struct syntatic_ctx *ctx)
{
    if (syntatic_paren_exp(ctx) < 0)
        return -1;

    if (syntatic_is_first_of_command(ctx)) {

    } else if (ctx->entry->token == TOKEN_OPENING_CURLY_BRACKET) {
    }

    SYNTATIC_ERROR();
    return -1;
}

static int
syntatic_is_first_of_s(struct syntatic_ctx *ctx)
{
    if (syntatic_is_first_of_command(ctx))
        return 1;

    switch (ctx->entry->token) {
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_BOOLEAN:
        case TOKEN_CHAR:
        case TOKEN_CONST:
            return 1;
        default:
            return 0;
    }

    UNREACHABLE();
}

static int
syntatic_start(struct syntatic_ctx *ctx)
{
    while (syntatic_is_first_of_s(ctx)) {
        if (syntatic_is_first_of_command(ctx)) {
            if (syntatic_command(ctx) < 0)
                return -1;
        } else {
            // Handle DECL_VAR and DECL_CONST.
            enum token tok = ctx->entry->token;
            if (syntatic_match_token(ctx, tok) != SYNTATIC_MATCH_OK)
                return -1;

            switch (tok) {
                case TOKEN_INT:
                case TOKEN_FLOAT:
                case TOKEN_STRING:
                case TOKEN_BOOLEAN:
                case TOKEN_CHAR:
                    if (syntatic_decl_var(ctx) < 0)
                        return -1;
                    break;
                case TOKEN_CONST:
                    if (syntatic_decl_const(ctx) < 0)
                        return -1;
                    break;
                default:
                    SYNTATIC_ERROR();
                    break;
            }
        }
    }

    SYNTATIC_ERROR();
    return -1;
}

static void
syntatic_init(struct syntatic_ctx *ctx,
              struct lexer *lexer,
              struct lexical_entry *entry)
{
    ctx->lexer = lexer;
    ctx->entry = entry;
}

// Main Stuff
static int
read_file_from_stdin(struct stdin_file *file, uint32_t capacity)
{
    file->size = 0;
#if !defined(VERDE)
    file->buffer = malloc(capacity + 1);
#else
    file->buffer = (char *)malloc(capacity + 1);
#endif
    if (!file->buffer)
        return -1;

    while (file->size < capacity && !feof(stdin))
        file->buffer[file->size++] = fgetc(stdin);
    file->buffer[file->size] = '\0';

    // Remove <CR> from file. Basically, make file
    // have only <LF> as line terminator.
    char *cr = strchr(file->buffer, '\r');
    while (cr) {
        *cr = ' ';
        cr = strchr(file->buffer, '\r');
    }

    return 0;
}

static void
destroy_stdin_file(struct stdin_file *file)
{
    free(file->buffer);
    file->buffer = NULL;
    file->size = 0;
}

int
main(void)
{
    struct stdin_file file;
    assert(read_file_from_stdin(&file, MAX_FILE_SIZE) == 0);

    struct symbol_table table;
    assert(symbol_table_create(&table, 64) == 0);

    symbol_table_populate_with_keywords(&table);

    struct lexer lexer;
    lexer_init(&lexer, &file, &table);

    struct lexical_entry entry;
    enum lexer_result result = lexer_get_next_token(&lexer, &entry);
    if (result == LEXER_RESULT_FOUND) {
        struct syntatic_ctx syntatic_ctx;
        syntatic_init(&syntatic_ctx, &lexer, &entry);
        syntatic_start(&syntatic_ctx);
    }

    symbol_table_destroy(&table);
    destroy_stdin_file(&file);
    return 0;
}
