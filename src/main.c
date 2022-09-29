#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define ERR_STREAM stderr

#define MAX_FILE_SIZE (32U * 1024U)
#define MAX_LEXEME_SIZE (32U)

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
            __builtin_unreachable();
    }
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
    while (result == LEXER_RESULT_FOUND)
        result = lexer_get_next_token(&lexer, &entry);

    if (result == LEXER_RESULT_ERROR) {
        lexer_print_error(&lexer);
    } else {
        fprintf(ERR_STREAM, "%i linhas compiladas.\n", lexer.line);
    }

    // TODO(Jose): Destroy table.
    destroy_stdin_file(&file);
    return 0;
}
