#include "Utils.h"

#include <cctype>

namespace utils {

std::string to_lowercase(std::string_view other)
{
    std::string lowercase;
    for (char c : other)
        lowercase += std::tolower(static_cast<int>(c));
    return lowercase;
}

}
