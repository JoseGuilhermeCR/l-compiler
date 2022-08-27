#include "Lexer.h"
#include "SymbolTable.h"
#include "Utils.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

constexpr char END_OF_FILE = 0x1A;

enum class LexerState
{
    Initial,
    KeywordOrIdentifier,   /* 1 */
    LogicalAnd,            /* 3 */
    LogicalOr,             /* 4 */
    NotOrNotEqual,         /* 5 */
    Assignment,            /* 6 */
    LessOrLessEqual,       /* 7 */
    GreaterOrGreaterEqual, /* 8 */
    DivisionOrComentary,   /* 9 */
    InCommentary,          /* 10 */
    LeavingCommentary,     /* 11 */
};

LexerError::LexerError(uint64_t line, std::string&& msg)
    : m_line(line)
    , m_msg(std::move(msg))
{
}

LexerError
LexerError::make_non_existent_lexeme_error(uint64_t line,
                                           const std::string& lexeme)
{
    std::string msg = "Non-existent lexeme found: " + lexeme;
    return LexerError(line, std::move(msg));
}

std::ostream&
operator<<(std::ostream& out, const LexerError& error)
{
    out << error.m_line << '\n';
    out << error.m_msg;
    return out;
}

Lexer::Lexer(const std::string& file, SymbolTable& symbol_table)
    : m_file(file)
    , m_symbol_table(symbol_table)
    , m_cursor(0)
    , m_line(1)
{
}

std::variant<std::monostate, Token, LexerError>
Lexer::get_next_token()
{
    if (m_cursor == m_file.size())
        return std::monostate{};

    std::string lexeme;

    LexerState state = LexerState::Initial;
    while (m_cursor < m_file.size()) {
        const char c = m_file[m_cursor++];

        if (c == '\n')
            ++m_line;

        if (!is_in_alphabet(c))
            return LexerError(m_line, "Unexpected character.");

        switch (state) {
            case LexerState::Initial:
                /* Make sure we clear the lexeme
                 * if we've come back from another state
                 * to Initial. */
                lexeme.clear();

                if (c == ' ' || c == '\n')
                    break;

                lexeme += c;

                if (std::isalpha(c) || c == '_') {
                    state = LexerState::KeywordOrIdentifier;
                    break;
                }

                switch (c) {
                    case '&':
                        state = LexerState::LogicalAnd;
                        break;
                    case '|':
                        state = LexerState::LogicalOr;
                        break;
                    case '!':
                        state = LexerState::NotOrNotEqual;
                        break;
                    case ':':
                        state = LexerState::Assignment;
                        break;
                    case '=':
                        return Token(TokenType::Equal);
                    case ',':
                        return Token(TokenType::Comma);
                    case '+':
                        return Token(TokenType::Plus);
                    case '-':
                        return Token(TokenType::Minus);
                    case '*':
                        return Token(TokenType::Times);
                    case ';':
                        return Token(TokenType::Semicolon);
                    case '(':
                        return Token(TokenType::OpeningParen);
                    case ')':
                        return Token(TokenType::ClosingParen);
                    case '[':
                        return Token(TokenType::OpeningSquareBracket);
                    case ']':
                        return Token(TokenType::ClosingSquareBracket);
                    case '{':
                        return Token(TokenType::OpeningCurlyBracket);
                    case '}':
                        return Token(TokenType::ClosingCurlyBracket);
                    case '<':
                        state = LexerState::LessOrLessEqual;
                        break;
                    case '>':
                        state = LexerState::GreaterOrGreaterEqual;
                        break;
                    case '/':
                        state = LexerState::DivisionOrComentary;
                        break;
                }

                break;
            case LexerState::KeywordOrIdentifier:
                if (std::isalnum(c) || c == '_') {
                    lexeme += c;
                } else {
                    --m_cursor;

                    auto possible_token = try_token_from_reserved_word(lexeme);
                    if (possible_token)
                        return *possible_token;

                    /* If no keyword token could be made out of the lexeme,
                     * it means this is an identifier. Add it to the symbol
                     * table.
                     * */
                    m_symbol_table.insert(lexeme);
                    return Token(TokenType::Identifier, std::move(lexeme));
                }
                break;
            case LexerState::LogicalAnd:
                if (c == '&') {
                    return Token(TokenType::LogicalAnd);
                } else {
                    lexeme += c;
                    return LexerError::make_non_existent_lexeme_error(m_line,
                                                                      lexeme);
                }
                break;
            case LexerState::LogicalOr:
                if (c == '|') {
                    return Token(TokenType::LogicalOr);
                } else {
                    lexeme += c;
                    return LexerError::make_non_existent_lexeme_error(m_line,
                                                                      lexeme);
                }
                break;
            case LexerState::NotOrNotEqual:
                if (c == '=') {
                    return Token(TokenType::NotEqual);
                } else {
                    --m_cursor;
                    return Token(TokenType::Not);
                }
                break;
            case LexerState::Assignment:
                if (c == '=') {
                    return Token(TokenType::Assignment);
                } else {
                    lexeme += c;
                    return LexerError::make_non_existent_lexeme_error(m_line,
                                                                      lexeme);
                }
                break;
            case LexerState::LessOrLessEqual:
                if (c == '=') {
                    return Token(TokenType::LessEqual);
                } else {
                    --m_cursor;
                    return Token(TokenType::Less);
                }
                break;
            case LexerState::GreaterOrGreaterEqual:
                if (c == '=') {
                    return Token(TokenType::GreaterEqual);
                } else {
                    --m_cursor;
                    return Token(TokenType::Greater);
                }
                break;
            case LexerState::DivisionOrComentary:
                if (c != '*') {
                    --m_cursor;
                    return Token(TokenType::Division);
                } else {
                    state = LexerState::InCommentary;
                }
                break;
            case LexerState::InCommentary:
                if (c == '*')
                    state = LexerState::LeavingCommentary;
                break;
            case LexerState::LeavingCommentary:
                if (c == '/')
                    state = LexerState::Initial;
                else if (c != '*')
                    state = LexerState::InCommentary;
                break;
        }
    }

    return LexerError(m_line, "Unexpected EOF");
}

bool
Lexer::is_in_alphabet(char c) const
{
    if (std::isalnum(c))
        return true;

    static constexpr std::array<char, 28> special = {
        ' ', '_', '.',  ',', ';', ':', '(', ')', '[', ']', '{', '}', '+', '*',
        '-', '"', '\'', '/', '|', '@', '&', '%', '!', '?', '>', '<', '=', '\n'
    };

    return std::find(special.cbegin(), special.cend(), c) != special.cend();
}

std::optional<Token>
Lexer::try_token_from_reserved_word(std::string_view lexeme) const
{
    const std::string lowercase_lexeme = utils::to_lowercase(lexeme);

    /* Special case: True and False are keywords, but they are still
     * constants.
     * */
    if (lowercase_lexeme == "true" || lowercase_lexeme == "false")
        return Token(TokenConstType::Boolean, std::string(lexeme));

    static constexpr std::array<std::string_view, 15> keywords = {
        "const",  "int", "char",   "while", "if",      "float", "else",
        "readln", "div", "string", "write", "writeln", "mod",   "boolean",
    };

    const auto possible_keyword =
        std::find(keywords.cbegin(), keywords.cend(), lowercase_lexeme);
    if (possible_keyword != keywords.cend())
        return Token(TokenType::Keyword, std::string(*possible_keyword));
    return {};
}

static std::string
read_file_from_stdin()
{
    std::string file;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty())
            file += line + '\n';
    }

    /* Remove the extra new line. */
    if (!file.empty())
        file.pop_back();

    return file;
}

int
main()
{
    const auto file = read_file_from_stdin();
    if (file.empty()) {
        std::cerr << "Empty input..." << '\n';
        return -1;
    }

    SymbolTable table;

    Lexer lexer(file, table);

    auto maybe_token = lexer.get_next_token();
    while (std::holds_alternative<Token>(maybe_token)) {
        const auto& t = std::get<Token>(maybe_token);
        std::cout << "Read(" << t.lexeme() << ')' << '\n';
        maybe_token = lexer.get_next_token();
    }

    if (std::holds_alternative<std::monostate>(maybe_token)) {
        std::cout << "all tokens read" << '\n';
        return 0;
    }

    std::cout << std::get<LexerError>(maybe_token) << '\n';

    return 0;
}
