
#ifndef KMP_H
#define KMP_H
#include <string.h>

typedef struct {
    char* pattern;
    int* lps;
    size_t patLen;
} KMPSearch;

#ifdef __cplusplus
extern "C" {
#endif

    KMPSearch* kmp_create(const char* pattern);
    void kmp_free(KMPSearch* kmp);
    size_t kmp_search(KMPSearch* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults);

#ifdef __cplusplus
}
#endif

#endif // KMP_H
