#pragma once

#include "Lexer.h"
#include "Token.h"

#include <optional>

class Syntatic {
public:
    Syntatic(Lexer& lexer);

    void run();
    void entry_symbol(const Token& token);

private:
    Lexer& m_lexer;

    std::optional<Token> next_token();

    void decl_const(const Token& id);
    void decl_var(const Token& id);
};
