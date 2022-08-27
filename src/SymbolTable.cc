#include "SymbolTable.h"

#include "Utils.h"

SymbolTable::SymbolTable()
{}

void SymbolTable::insert(std::string_view lexeme)
{
    std::string lowercase_lexeme = utils::to_lowercase(lexeme);
    if (m_table.contains(lowercase_lexeme))
        return;
    m_table.emplace(std::move(lowercase_lexeme), Symbol{});
}
