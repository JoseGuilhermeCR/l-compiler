#ifndef UTILS_H_
#define UTILS_H_

#include <assert.h>

#define UNREACHABLE()                                                          \
    do {                                                                       \
        assert(0 && "Should never be reached.");                               \
    } while (0)

#endif