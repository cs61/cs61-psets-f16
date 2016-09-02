#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// Advanced error message for freeing data inside another heap block.

int main() {
    void* ptr = malloc(2001);
    free((char*) ptr + 100);
    m61_printstatistics();
}

//! MEMORY BUG: test???.c:9: invalid free of pointer ???, not allocated
//!   test???.c:8: ??? is 100 bytes inside a 2001 byte region allocated here
//! ???
