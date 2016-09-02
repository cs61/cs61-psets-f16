#include "m61.h"
#include <stdio.h>
// Total allocation counts.

int main() {
    for (int i = 0; i < 10; ++i)
        (void) malloc(1);
    m61_printstatistics();
}

// In expected output, "???" can match any number of characters.

//! malloc count: active        ???   total         10   fail        ???
//! malloc size:  active        ???   total        ???   fail        ???
