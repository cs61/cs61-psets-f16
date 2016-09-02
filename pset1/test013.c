#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// Realloc into a smaller buffer.

const char data[10] = "1234567890";

int main() {
    char* p = (char*) realloc(NULL, 10);
    assert(p != NULL);
    memcpy(p, data, 10);
    p = (char*) realloc(p, 8);
    assert(p != NULL);
    assert(memcmp(data, p, 8) == 0);
    free(p);
    m61_printstatistics();
}

//! malloc count: active          0   total          2   fail          0
//! malloc size:  active          0   total         18   fail          0
