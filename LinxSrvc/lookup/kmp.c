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

/* kmp_get_frame / chunkSize==0 时使用的默认读取块大小 */
#define KMP_CHUNK_SIZE (1024 * 1024)

size_t kmp_match(KMPDetail* kmp, const char* filename, size_t chunkSize, size_t* offsets, size_t maxResults)
{
    if (kmp == NULL || filename == NULL || kmp->size == 0) return 0;
    if (offsets == NULL || maxResults == 0) return 0;
    if (chunkSize == 0) chunkSize = KMP_CHUNK_SIZE;

    FILE* file = fopen(filename, "rb");
    if (!file) return 0;

    size_t pattlen = kmp->size;
    /* 匹配可能跨越块边界，因此每块末尾要保留 pattlen-1 字节带到下一块 */
    size_t overlapSize = pattlen > 1 ? pattlen - 1 : 0;
    unsigned char* buffer = (unsigned char*)malloc(chunkSize + overlapSize);
    if (!buffer) {
        fclose(file);
        return 0;
    }

    size_t globalPos = 0;   /* buffer[0] 对应的文件绝对偏移 */
    size_t carry = 0;       /* buffer 开头已有的有效字节数（上一块留下的 overlap） */
    size_t found = 0;

    for (;;) {
        size_t bytesRead = fread(buffer + carry, 1, chunkSize, file);
        if (bytesRead == 0) break;
        size_t total = carry + bytesRead;

        size_t i = 0;
        int j = 0;
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

        if (bytesRead < chunkSize) break;   /* 已读到 EOF，无需再保留 overlap */
        if (overlapSize == 0) {
            globalPos += total;
            continue;
        }
        /* 把本块末尾 overlapSize 字节挪到 buffer 开头，globalPos 同步前移 */
        memmove(buffer, buffer + total - overlapSize, overlapSize);
        globalPos += total - overlapSize;
        carry = overlapSize;
    }

    free(buffer);
    fclose(file);
    return found;
}

size_t kmp_get_frame(KMPDetail* kmp, const char* filename, MatchedFrame* matches, size_t maxResults)
{
    if (kmp == NULL || filename == NULL || matches == NULL || maxResults == 0) return 0;

    /* 全文件流式扫描（原来这里是二分采样，只扫了 ~log2(filesize) 个窗口，
       绝大多数匹配会漏掉）。先把偏移收进临时数组，再填 MatchedFrame。 */
    size_t* offsets = (size_t*)malloc(sizeof(size_t) * maxResults);
    if (!offsets) return 0;

    size_t found = kmp_match(kmp, filename, KMP_CHUNK_SIZE, offsets, maxResults);
    for (size_t i = 0; i < found; ++i) {
        matches[i].offset = offsets[i];
        matches[i].length = kmp->size;
        /* 指向 KMP 内部的 pattern 副本，kmp_free() 之后失效 */
        matches[i].pattern = kmp->pattern;
        matches[i].reserved = NULL;
    }
    free(offsets);
    return found;
}
