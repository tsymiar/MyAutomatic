
#include "kmp.h"
#include <stdio.h>
#include <stdlib.h>

void buildLPS(KMPSearch* kmp) {
    size_t len = 0;
    kmp->lps[0] = 0;
    size_t i = 1;
    while (i < kmp->patLen) {
        if (kmp->pattern[i] == kmp->pattern[len]) {
            kmp->lps[i++] = ++len;
        } else if (len != 0) {
            len = kmp->lps[len - 1];
        } else {
            kmp->lps[i++] = 0;
        }
    }
}

KMPSearch* kmp_create(const char* pattern) {
    KMPSearch* kmp = (KMPSearch*)malloc(sizeof(KMPSearch));
    kmp->patLen = strlen(pattern);
    kmp->pattern = (char*)malloc(kmp->patLen + 1);
    strcpy(kmp->pattern, pattern);
    kmp->lps = (int*)malloc(sizeof(int) * kmp->patLen);
    buildLPS(kmp);
    return kmp;
}

void kmp_free(KMPSearch* kmp) {
    if (kmp) {
        free(kmp->pattern);
        free(kmp->lps);
        free(kmp);
    }
}

// offsets: output array, maxResults: its capacity, returns number of matches
size_t kmp_search(KMPSearch* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;

    size_t patLen = kmp->patLen;
    if (patLen == 0) {
        fclose(file);
        return 0;
    }

    size_t overlapSize = patLen > 1 ? patLen - 1 : 0;
    char* buffer = (char*)malloc(chunkSize + overlapSize);
    size_t globalPos = 0;
    size_t found = 0;

    size_t prevOverlap = 0;
    while (1) {
        // Read chunk
        size_t bytesRead = fread(buffer + overlapSize, 1, chunkSize, file);
        if (bytesRead == 0) break;

        // KMP search
        int j = 0;
        size_t i = 0;
        size_t total = bytesRead + overlapSize;
        while (i < total) {
            if (buffer[i] == kmp->pattern[j]) {
                ++i; ++j;
                if (j == patLen) {
                    if (found < maxResults)
                        offsets[found++] = globalPos + i - j;
                    j = kmp->lps[j - 1];
                }
            } else if (j != 0) {
                j = kmp->lps[j - 1];
            } else {
                ++i;
            }
        }

        // Prepare overlap for next chunk
        if (bytesRead == chunkSize && overlapSize > 0) {
            memmove(buffer, buffer + chunkSize, overlapSize);
        }
        globalPos += bytesRead;
        if (bytesRead < chunkSize) break;
    }

    free(buffer);
    fclose(file);
    return found;
}
