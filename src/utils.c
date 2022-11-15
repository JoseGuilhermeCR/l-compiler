/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#include "utils.h"

#include <ctype.h>

uint8_t
is_case_insensitive_equal(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *rhs != '\0') {
        if (tolower((unsigned char)*lhs) != tolower((unsigned char)*rhs))
            return 0;

        ++lhs;
        ++rhs;
    }

    return *lhs == '\0' && *rhs == '\0';
}
