#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// Diabolical calloc.

int main() {
    size_t very_large_nmemb = (size_t) -1 / 8 + 2;
    char* p = (char*) calloc(very_large_nmemb, 16);
    assert(p == NULL);
    m61_printstatistics();
}

//! malloc count: active          0   total          0   fail          1
//! malloc size:  active          0   total          0   fail        ???
