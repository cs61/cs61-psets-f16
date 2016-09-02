#include "m61.h"
#include <stdio.h>
// Trivial check: no allocations == zero statistics.

int main() {
    m61_printstatistics();
}

// Lines starting with "//!" define the expected output for this test.

//! malloc count: active          0   total          0   fail          0
//! malloc size:  active          0   total          0   fail          0
