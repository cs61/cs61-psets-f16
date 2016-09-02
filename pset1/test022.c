#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// Invalid realloc.

int main() {
    char* ptr = (char*) malloc(2000);
    ptr = realloc(&ptr[1], 2001);
    m61_printstatistics();
}

//! MEMORY BUG???: invalid ??{realloc|free}?? of pointer ???
//! ???
