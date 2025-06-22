
#ifndef KMP_H
#define KMP_H
#include <string.h>
#include <stdint.h>

typedef struct {
    unsigned char* pattern;
    int* lps;
    size_t size;
} KMPDetail;

typedef struct {
    size_t offset;
    size_t length;
    unsigned char* pattern;
    void* reserved;
} MatchedFrame;

#ifdef __cplusplus
extern "C" {
#endif

    KMPDetail* kmp_create(const unsigned char* pattern, size_t pattlen);
    void kmp_free(KMPDetail* kmp);
    size_t kmp_match(KMPDetail* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults);
    size_t kmp_get_frame(KMPDetail* kmp, const char* filename, MatchedFrame* matches, size_t maxResults);

#ifdef __cplusplus
}
#endif

#endif // KMP_H
