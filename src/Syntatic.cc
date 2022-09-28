#include "Syntatic.h"
#include "Token.h"

#include <iostream>
#include <algorithm>
#include <array>
#include <string_view>
#include <variant>

Syntatic::Syntatic(Lexer& lexer)
    : m_lexer(lexer)
{

}

std::optional<Token>
Syntatic::next_token()
{
    auto possible_next_token = m_lexer.get_next_token();

    if (Token *token = std::get_if<Token>(&possible_next_token)) {
        return *token;
    } else if (LexerError *error = std::get_if<LexerError>(&possible_next_token)) {
        std::cerr << *error << '\n';
        return {};
    }
    
    return {};
}

void
Syntatic::run()
{
    if (auto token = next_token())
        entry_symbol(*token);
}

void
Syntatic::entry_symbol(const Token& token)
{
    if (token.type() == TokenType::Keyword) {
        static constexpr std::array<std::string_view, 5> decl_var_start {
            "int",
            "float",
            "string",
            "char",
            "boolean"
        };

        const bool is_decl_var = std::find(decl_var_start.cbegin(), decl_var_start.cend(), token.lexeme()) != decl_var_start.cend();

        if (is_decl_var) {
            if (auto t = next_token()) {
                decl_var(*t);
            } else {
                // TODO(Jose): Place error here.
            }
        } else if (token.lexeme() == "const") {
            if (auto t = next_token()) {
                decl_const(*t);
            } else {
                // TODO(Jose): Place error here.
            }
        } else {
        }
    }
}


void
Syntatic::decl_const(const Token& id)
{
    // First, check if id is indeed an Identifier.
    if (id.type() != TokenType::Identifier) {
        return;
    }

    // Read next token, it should be =.
    std::optional<Token> token = next_token();
    if (!token || (token->type() != TokenType::Equal)) {
        return;
    }

    // Read next token, it should be - or constant.
    token = next_token();
    if (!token) {
        return;
    }

    // In case it's -, read the const that comes after it
    // and prepare to read the semicolon at the end.
    // In case it's constant, prepare to read the semicolon
    // at the end.
    if (token->type() == TokenType::Minus) {
        token = next_token();
        if (!token || (token->type() != TokenType::Const)) {
            return;
        }
        token = next_token();
    } else if (token->type() == TokenType::Const) {
        token = next_token();
    } else {
        return;
    }

    if (token->type() != TokenType::Semicolon) {
        return;
    }
    
    // SUCCESS
}

void
Syntatic::decl_var(const Token& id)
{
    // First, check if id is indeed an Identifier.
    if (id.type() != TokenType::Identifier) {
        return;
    }

    // Read next token, it should be :=.
    std::optional<Token> token = next_token();
    if (!token || (token->type() != TokenType::Assignment)) {
        return;
    }

    //token = next_token();
    //if (!token ||
}
