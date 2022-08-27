#pragma once

#include <string>
#include <unordered_map>

enum class TokenType;

/* A symbol is created from an *identifier* Token. */
class Symbol {
};

class SymbolTable {
    public:
        SymbolTable();
        void insert(std::string_view lexeme);

    private:
        std::unordered_map<std::string, Symbol> m_table;
};
