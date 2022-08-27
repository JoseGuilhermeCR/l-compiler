#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <optional>
#include <variant>

#include "Token.h"

class SymbolTable;

class LexerError {
public:
    LexerError(uint64_t line, std::string&& msg);

    static LexerError make_non_existent_lexeme_error(uint64_t line, const std::string& lexeme);

    friend std::ostream& operator<<(std::ostream& out, const LexerError& error);
    
private:
    uint64_t m_line;
    std::string m_msg;
};

class Lexer {
    public:
        Lexer(const std::string& file, SymbolTable& table);

        std::variant<std::monostate, Token, LexerError> get_next_token();

    private:
        bool is_in_alphabet(char c) const;
        std::optional<Token> try_token_from_reserved_word(std::string_view lexeme) const;

        const std::string& m_file;
        SymbolTable& m_symbol_table;
        uint64_t m_cursor;
        uint64_t m_line;
};

