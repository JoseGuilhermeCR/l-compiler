#include "Token.h"

#include <cassert>

Token::Token(TokenType type)
    : m_type(type)
    , m_lexeme({})
    , m_const_type({})
{ }

Token::Token(TokenType type, std::string&& lexeme)
    : m_type(type)
    , m_lexeme(std::move(lexeme))
    , m_const_type({})
{ }

Token::Token(TokenConstType const_type, std::string&& lexeme)
    : m_type(TokenType::Const)
    , m_lexeme(std::move(lexeme))
    , m_const_type(const_type)
{ }

std::string_view Token::lexeme() const
{
    if (m_lexeme)
        return *m_lexeme;

    assert(m_type != TokenType::Identifier);
    assert(m_type != TokenType::Keyword);
    assert(m_type != TokenType::Const);

    return get_lexeme_from_type();
}

TokenConstType Token::token_const_type() const
{
    assert(m_type == TokenType::Const);
    return *m_const_type;
}

std::string_view Token::get_lexeme_from_type() const
{
    switch (m_type) {
        case TokenType::LogicalAnd:
            return "&&";
        case TokenType::LogicalOr:
            return "||";
        case TokenType::NotEqual:
            return "!=";
        case TokenType::Not:
            return "!";
        case TokenType::Assignment:
            return ":=";
        case TokenType::Equal:
            return "=";
        case TokenType::Comma:
            return ",";
        case TokenType::Plus:
            return "+";
        case TokenType::Minus:
            return "-";
        case TokenType::Times:
            return "*";
        case TokenType::Semicolon:
            return ";";
        case TokenType::OpeningParen:
            return "(";
        case TokenType::ClosingParen:
            return ")";
        case TokenType::OpeningSquareBracket:
            return "[";
        case TokenType::ClosingSquareBracket:
            return "]";
        case TokenType::OpeningCurlyBracket:
            return "{";
        case TokenType::ClosingCurlyBracket:
            return "}";
        case TokenType::Less:
            return "<";
        case TokenType::LessEqual:
            return "<=";
        case TokenType::Greater:
            return ">";
        case TokenType::GreaterEqual:
            return ">=";
        case TokenType::Division:
            return "/";
        case TokenType::Identifier:
            break;
        case TokenType::Keyword:
            break;
        case TokenType::Const:
            break;
    }
    assert(0 && "Should never be reached.");
}

