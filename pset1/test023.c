#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// Another invalid realloc.

int main() {
    char* ptr = (char*) malloc(2002);
    ptr = realloc(&ptr, 2003);
    m61_printstatistics();
}

//! MEMORY BUG???: invalid ??{realloc|free}?? of pointer ???
//! ???
