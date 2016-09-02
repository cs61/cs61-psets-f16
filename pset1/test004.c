#include "m61.h"
#include <stdio.h>
// Total allocation sizes.

int main() {
    void* ptrs[10];
    for (int i = 0; i < 10; ++i)
        ptrs[i] = malloc(i + 1);
    for (int i = 0; i < 5; ++i)
        free(ptrs[i]);
    m61_printstatistics();
}

//! malloc count: active          5   total         10   fail        ???
//! malloc size:  active        ???   total         55   fail        ???
