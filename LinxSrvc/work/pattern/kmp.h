
#pragma once
#include <string.h>

typedef struct {
    char* pattern;
    int* lps;
    size_t patLen;
} KMPSearch;

KMPSearch* kmp_create(const char* pattern);
void kmp_free(KMPSearch* kmp);
size_t kmp_search(KMPSearch* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults);

// usage
/*
int main() {
    KMPSearch* kmp = kmp_create("pattern");
    size_t offset[1000];
    size_t found = kmp_searchFile(kmp, "test.txt", 4096, offset, 1000);
    for (size_t i = 0; i < found; ++i) {
        printf("Found at: %zu\n", offset[i]);
    }
    kmp_free(kmp);
    return 0;
}
*/
