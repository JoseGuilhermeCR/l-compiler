/* Compiladores - Ciência da Computação - Coração Eucarístico - 2022/2
 * José Guilherme de Castro Rodrigues - 651201
 * */
#ifndef UTILS_H_
#define UTILS_H_

#include <assert.h>
#include <stdint.h>

#if defined(NDEBUG)

#define UNREACHABLE() __builtin_unreachable()

#else

#define UNREACHABLE()                                                          \
    do {                                                                       \
        assert(0 && "Should never be reached.");                               \
    } while (0)

#endif

/*
 * Performs a comparison between two strings without taking
 * case into consideration.
 * */
uint8_t
is_case_insensitive_equal(const char *lhs, const char *rhs);

#endif
