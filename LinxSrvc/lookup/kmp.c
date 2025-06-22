#include "kmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buildLPS(KMPDetail* kmp)
{
    size_t len = 0;
    kmp->lps[0] = 0;
    size_t i = 1;
    while (i < kmp->size) {
        if (kmp->pattern[i] == kmp->pattern[len]) {
            kmp->lps[i++] = ++len;
        } else if (len != 0) {
            len = kmp->lps[len - 1];
        } else {
            kmp->lps[i++] = 0;
        }
    }
}

KMPDetail* kmp_create(const unsigned char* pattern, size_t pattlen)
{
    if (pattlen == 0 || pattern == NULL) {
        return NULL;
    }
    KMPDetail* kmp = (KMPDetail*)malloc(sizeof(KMPDetail));
    if (!kmp) return NULL;
    kmp->size = pattlen;
    kmp->pattern = (unsigned char*)malloc(kmp->size);
    if (!kmp->pattern) {
        free(kmp);
        return NULL;
    }
    memcpy(kmp->pattern, pattern, kmp->size);
    kmp->lps = (int*)malloc(sizeof(int) * kmp->size);
    if (!kmp->lps) {
        free(kmp->pattern);
        free(kmp);
        return NULL;
    }
    buildLPS(kmp);
    return kmp;
}

void kmp_free(KMPDetail* kmp)
{
    if (kmp != NULL) {
        free(kmp->pattern);
        free(kmp->lps);
        free(kmp);
    }
}

size_t kmp_match(KMPDetail* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;

    size_t pattlen = kmp->size;
    if (pattlen == 0) {
        fclose(file);
        return 0;
    }

    size_t overlapSize = pattlen > 1 ? pattlen - 1 : 0;
    unsigned char* buffer = (unsigned char*)malloc(chunkSize + overlapSize);
    size_t globalPos = 0;
    size_t found = 0;

    while (1) {
        size_t bytesRead = fread(buffer + overlapSize, 1, chunkSize, file);
        if (bytesRead == 0) break;

        int j = 0;
        size_t i = 0;
        size_t total = bytesRead + overlapSize;
        while (i < total) {
            if (buffer[i] == kmp->pattern[j]) {
                ++i; ++j;
                if ((size_t)j == pattlen) {
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

size_t kmp_get_frame(KMPDetail* kmp, const char* filename, MatchedFrame* matches, size_t maxResults)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    size_t left = 0, right = fileSize;
    size_t found = 0;

    while (left < right && found < maxResults) {
        size_t mid = left + (right - left) / 2;
        size_t chunkSize = kmp->size * 2;
        if (mid + chunkSize > fileSize) chunkSize = fileSize - mid;

        unsigned char* buffer = (unsigned char*)malloc(chunkSize);
        fseek(fp, mid, SEEK_SET);
        size_t bytesRead = fread(buffer, 1, chunkSize, fp);
        if (bytesRead == 0) {
            free(buffer);
            break;
        }

        for (size_t i = 0; i + kmp->size <= chunkSize; ++i) {
            if (memcmp(buffer + i, kmp->pattern, kmp->size) == 0) {
                matches[found].offset = mid + i;
                matches[found].length = kmp->size;
                matches[found].pattern = kmp->pattern;
                found++;
                if (found >= maxResults) break;
            }
        }
        free(buffer);

        right = mid;
        if (found < maxResults) {
            left = mid + 1;
            right = fileSize;
        }
    }
    fclose(fp);
    return found;
}
