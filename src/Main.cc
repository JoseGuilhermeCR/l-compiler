#include "Lexer.h"
#include "SymbolTable.h"
#include "Utils.h"

#include <iostream>

static std::string
read_file_from_stdin()
{
    std::string file;

    std::string line;
    while (std::getline(std::cin, line))
        file += line + '\n';

    return file;
}

int
main()
{
    const std::string file = read_file_from_stdin();
    if (file.empty())
        return -1;

    SymbolTable table;
    Lexer lexer(file, table);

    auto maybe_token = lexer.get_next_token();
    while (std::holds_alternative<Token>(maybe_token))
        maybe_token = lexer.get_next_token();

    if (std::holds_alternative<std::monostate>(maybe_token)) {
        std::cout << lexer.current_line_number() << " linhas compiladas" << '\n';
        return 0;
    }

    std::cout << std::get<LexerError>(maybe_token) << '\n';

    return 0;
}
