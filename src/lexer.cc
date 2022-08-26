#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <array>
#include <cctype>
#include <algorithm>
#include <optional>
#include <cassert>

constexpr char END_OF_FILE = 0x1A;

enum class TokenType {
	Identifier,
	LogicalAnd, // &&
	LogicalOr,  // ||
	NotEqual,   // !=
	Not,        // !
	Assignment, // :=
	Equal,      // =
	Comma,      // ,
	Plus,       // +
	Minus,      // -
	Times,      // *
	Semicolon, // ;
	OpeningParen, // (
	ClosingParen, // )
	OpeningSquareBracket, // [
	ClosingSquareBracket, // ]
	OpeningCurlyBracket, // {
	ClosingCurlyBracket, // }
};

enum class LexerState {
	Initial,
	KeywordOrIdentifier, // 1
	LogicalAnd, // 3
	LogicalOr, // 4
	NotOrNotEqual, // 5
	Assignment,    // 6
};

struct LexerError {
	std::string msg;
};

class Token {
public:
	Token(TokenType type);
	std::string_view lexeme() const;
	std::string_view token_type_str() const;

private:
	std::string_view get_lexeme_from_type() const;

	TokenType m_type;
	std::optional<std::string> m_lexeme;
};

class Lexer {
public:
	Lexer(const std::string& file);

	std::variant<std::monostate, Token, LexerError> get_next_token();

private:
	bool is_in_alphabet(char c) const;

	const std::string& m_file;
	uint64_t m_cursor;
};

static LexerError make_non_existent_lexeme_error(const std::string& lexeme)
{
	return LexerError { .msg = std::string("Non existent lexeme found: ") + lexeme };
}

Token::Token(TokenType type) : m_type(type), m_lexeme({}) { }

std::string_view Token::lexeme() const
{
	if (m_lexeme)
		return *m_lexeme;

	assert(m_type != TokenType::Identifier);
	return get_lexeme_from_type();
}

std::string_view Token::token_type_str() const
{
	switch (m_type) {
	case TokenType::Identifier:
		return "Identifier";
	case TokenType::LogicalAnd:
		return "LogicalAnd";
	case TokenType::LogicalOr:
		return "LogicalOr";
	case TokenType::NotEqual:
		return "NotEqual";
	case TokenType::Not:
		return "Not";
	case TokenType::Assignment:
		return "Assignment";
	case TokenType::Equal:
		return "Equal";
	case TokenType::Comma:
		return "Comma";
	case TokenType::Plus:
		return "Plus";
	case TokenType::Minus:
		return "Minus";
	case TokenType::Times:
		return "Times";
	case TokenType::Semicolon:
		return "Semicolon";
	case TokenType::OpeningParen:
		return "OpeningParen";
	case TokenType::ClosingParen:
		return "ClosingParen";
	case TokenType::OpeningSquareBracket:
		return "OpeningSquareBracket";
	case TokenType::ClosingSquareBracket:
		return "ClosingSquareBracket";
	case TokenType::OpeningCurlyBracket:
		return "OpeningCurlyBracket";
	case TokenType::ClosingCurlyBracket:
		return "ClosingCurlyBracket";
	}
	assert(0 && "Should never be reached.");
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
	case TokenType::Identifier:
		break;
	}
	assert(0 && "Should never be reached.");
}

Lexer::Lexer(const std::string& file)
	: m_file(file)
	, m_cursor(0)
{
}

std::variant<std::monostate, Token, LexerError> Lexer::get_next_token()
{
	if (m_cursor == m_file.size()) {
		return std::monostate{};
	}

	std::string lexeme;
	
	LexerState state = LexerState::Initial;
	while (m_cursor < m_file.size()) {
		const char c = m_file[m_cursor++];
#if 0
		if (std::isprint(c))
			std::cout << "Read: " << c << '\n';
		else
			std::cout << "Read: " << static_cast<int32_t>(c) << '\n';
#endif

		if (!is_in_alphabet(c)) {
			return LexerError { .msg = std::string("Unexpected character.") };
		}

		switch (state) {
		case LexerState::Initial:
			if (c == ' ')
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
				return Token(TokenType::ClosingCurlyBracket);
			case '{':
				return Token(TokenType::OpeningCurlyBracket);
			case '}':
				return Token(TokenType::ClosingSquareBracket);
			}

			break;
		case LexerState::KeywordOrIdentifier:
			if (std::isalnum(c) || c == '_') {
				lexeme += c;
			} else {
				--m_cursor;
				// TODO(José): Actually check whether this
				// is a Keyword or an Identifier.
				// We must check the symbol table for that :D
				return Token(TokenType::Identifier);
			}
			break;
		case LexerState::LogicalAnd:
			lexeme += c;
			if (c == '&') {
				// TODO(José): Make token and return.
				return Token(TokenType::LogicalAnd);
			} else {
				return make_non_existent_lexeme_error(lexeme);
			}
			break;
		case LexerState::LogicalOr:
			if (c == '|') {
				return Token(TokenType::LogicalOr);
			} else {
				lexeme += c;
				return make_non_existent_lexeme_error(lexeme);
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
				return make_non_existent_lexeme_error(lexeme);
			}
			break;
		}
	}

	return LexerError { .msg = std::string("Unexpected EOF") };
}

bool Lexer::is_in_alphabet(char c) const
{
	if (std::isalnum(c))
		return true;

	constexpr std::array<char, 28> special = {
		' ',
		'_',
		'.',
		',',
		';',
		':',
		'(',
		')',
		'[',
		']',
		'{',
		'}',
		'+',
		'*',
		'-',
		'"',
		'\'',
		'/',
		'|',
		'@',
		'&',
		'%',
		'!',
		'?',
		'>',
		'<',
		'=',
		'\n'
	};

	return std::find(special.cbegin(), special.cend(), c) != special.cend();
}

static std::string read_file_from_stdin()
{
	std::string file;

	std::string line;
	while (std::getline(std::cin, line)) {
		if (!line.empty())
			file += line + '\n';
	}

	// Remove the extra new line.
	if (!file.empty())
		file.pop_back();

	return file;
}

int main()
{
	const auto file = read_file_from_stdin();
	if (file.empty()) {
		std::cerr << "Empty input..." << '\n';
		return -1;
	}

	Lexer lexer(file);

	auto maybe_token = lexer.get_next_token();
	while (std::holds_alternative<Token>(maybe_token)) {
		const auto& t = std::get<Token>(maybe_token);
		std::cout << "Type: " << t.token_type_str() << " Lexeme: " << t.lexeme() << '\n';
		maybe_token = lexer.get_next_token();
	}

	if (std::holds_alternative<std::monostate>(maybe_token)) {
		std::cout << "all tokens read" << '\n';
		return 0;
	}

	std::cout << std::get<LexerError>(maybe_token).msg << '\n';

	return 0;
}

