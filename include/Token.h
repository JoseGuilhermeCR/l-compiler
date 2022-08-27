#pragma once

#include <optional>
#include <string>
#include <string_view>

enum class TokenType
{
    Identifier,           /* a */
    Keyword,              /* while */
    Const,                /* const */
    LogicalAnd,           /* && */
    LogicalOr,            /* || */
    NotEqual,             /* != */
    Not,                  /* !  */
    Assignment,           /* := */
    Equal,                /* =  */
    Comma,                /* ,  */
    Plus,                 /* +  */
    Minus,                /* -  */
    Times,                /* *  */
    Semicolon,            /* ; */
    OpeningParen,         /* ( */
    ClosingParen,         /* ) */
    OpeningSquareBracket, /* [ */
    ClosingSquareBracket, /* ] */
    OpeningCurlyBracket,  /* { */
    ClosingCurlyBracket,  /* } */
    Less,                 /* < */
    LessEqual,            /* <= */
    Greater,              /* > */
    GreaterEqual,         /* >= */
    Division,             /* / */
};

enum class TokenConstType
{
    Boolean, /* True / False */
    Char,    /* 'a' */
    String,  /* "Hello, World" */
};

class Token
{
  public:
    /*
     * Constructor for tokens that have default lexemes, e.g.:
     * ( ) { } ;
     * */
    Token(TokenType type);

    /*
     * Constructor for Keyword / Identifier tokens, e.g.:
     * TokenType::Keyword, int
     * TokenType::Identifier a
     * */
    Token(TokenType type, std::string&& lexeme);

    /*
     * Constructor for Const tokens, e.g.:
     * TokenType::Const, ConstType::Boolean, False
     * TokenType::Const, ConstType::Integer, 1
     */
    Token(TokenConstType const_type, std::string&& lexeme);

    TokenType type() const { return m_type; }
    std::string_view lexeme() const;

    /*
     * Should only be called if TokenType is Const.
     * Returns the kind of constant this token represents.
     * */
    TokenConstType token_const_type() const;

  private:
    /**
     * Simple tokens don't need their lexeme stored.
     * In that case, when we want to get the lexeme, this
     * function will do it, e.g.:
     * TokenType::OpeningCurlyBracket gives us {
     * */
    std::string_view get_lexeme_from_type() const;

    TokenType m_type;

    /* Used only for keyword, identifier and const tokens. */
    std::optional<std::string> m_lexeme;

    /* Used only for const tokens. */
    std::optional<TokenConstType> m_const_type;
};
