#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <vector>
#include <thread>
#include <fcntl.h>
#include "common.h"

namespace
{
    const char* g_filename = "./test.dat";
    uint64_t g_total = 1048576;
    uint32_t g_endian = 0;
    uint32_t g_bits = 32;
    uint32_t g_interval = 1;
    uint32_t g_digress = 0;
    uint64_t g_current = 0;
    std::vector<uint32_t> g_vecStart{};
    bool g_addRandFrame = true;
} // namespace

void usageExit(int exitcode, char** argv);
void parseArgs(int argc, char** argv);
bool isSmallEndian();
void byteSwap16(uint16_t* val);
void byteSwap32(uint32_t* val);
void byteSwap24(uint32_t* val);
void makeDirExist(const char* dir);
uint64_t getMsecTime();
uint64_t sizeConvert(const std::string& sizeVal);
void conStringToVec(std::string str, std::vector<uint32_t>& vec);

union Number {
    uint8_t _8v;
    uint16_t _16v;
    uint32_t _24v;
    uint32_t _32v;
    uint64_t _64v;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        usageExit(0, argv);
    }
    parseArgs(argc, argv);
    size_t size = sizeof(uint32_t);
    bool small = isSmallEndian();
    bool byteswap = !small;
    if (g_endian == 0) {
        byteswap = small;
    }
    switch (g_bits) {
    case 8:
        size = sizeof(uint8_t);
        break;
    case 16:
        size = sizeof(uint16_t);
        break;
    case 24:
        size = 3 * sizeof(uint8_t);
        break;
    case 64:
        size = sizeof(uint64_t);
        break;
    case 32:
    default:
        size = sizeof(uint32_t);
        break;
    }
    // uint64_t count = g_total / size;
    makeDirExist(g_filename);
    FILE* pdat = fopen(g_filename, "wb+");
    if (pdat == NULL) {
        fprintf(stderr, "fwrite open failed: %s.\n", g_filename);
        return -1;
    }

    char timefile[256];
    std::string fileName = g_filename;
    size_t pos = fileName.rfind('.');
    std::string csvpre = fileName.substr(0, pos);
    sprintf(timefile, "%s.csv", csvpre.c_str());
    FILE* tmf = fopen(timefile, "wb+");
    if (tmf == NULL) {
        fprintf(stderr, "fwrite open failed: %s.\n", timefile);
        return -1;
    }

    // fcntl(fileno(pdat), F_SETFL, O_NONBLOCK);
    if (g_vecStart.size() == 0) {
        g_vecStart.push_back(0);
    }
    size_t len = g_vecStart.size();
    if (len != 1) {
        printf(" g_vecStart.size()=%zu not allow, only allow one member", len);
        return -1;
    };
    uint64_t value;
    value = g_vecStart[0];

    std::thread progress(
        [&](uint64_t total) -> void {
            while (g_current <= total) {
                usleep(100000);
                fprintf(stdout, "\rprogress: %.2f %% ", g_current * 100.f / total);
                fflush(stdout);
            }
            printf("\n");
        },
        (g_total));

    if (progress.joinable()) {
        progress.detach();
    }
    printf("fwrite(fileName=%s) begin!\n", g_filename);
    int64_t begin = getMsecTime();
    unsigned int seed = time(NULL);
    ProjectFrame header{};
    uint64_t idx = 0;
    size_t rand0 = 0;
    time_t now = begin / 1000; // s
    struct tm* currentTime = localtime(&now);
    int64_t startOfDayTime = now * 1000 - 3600000 * (currentTime->tm_hour % 24) - 60000 * currentTime->tm_min - 1000 * currentTime->tm_sec;
    while (g_current < g_total) {
        if (g_addRandFrame) {
            uint32_t tmp = rand_r(&seed);
            rand0 = tmp % 2048 + 1;
            header.size = rand0 * size;
            header.utctime = getMsecTime() - startOfDayTime;
            if (g_current == 0) {
                char time[128] = { 0 };
                sprintf(time, "%ld\n", header.utctime);
                int64_t wroteSize = fwrite(&time, strlen(time), 1, tmf);
                if (wroteSize != 1) {
                    fprintf(stderr, "fwrite(fileName=%s) failed!\n", timefile);
                } else {
                    printf("fwrite(fileName=%s) utctime=%ld success!\n", timefile, header.utctime);
                }
            }
            int64_t wroteSize = fwrite(&header, sizeof(ProjectFrame), 1, pdat);
            if (wroteSize != 1) {
                printf("fwrite(fileName=%s) failed!\n", g_filename);
            } else {
                idx++;
                g_current += sizeof(ProjectFrame);
            }
        }
        for (size_t j = 0; j < rand0; j++) {
            Number number;
            number._64v = value;
            if (byteswap) {
                switch (size) {
                    case sizeof(uint16_t) :
                        byteSwap16((uint16_t*)&number._16v);
                        break;
                        case sizeof(uint64_t) :
                            number._64v = __builtin_bswap64((uint64_t)number._64v);
                            break;
                            case 3 * sizeof(uint8_t) :
                                byteSwap24((uint32_t*)&number._24v);
                                break;
                                case sizeof(uint32_t) :
                                default:
                                    byteSwap32((uint32_t*)&number._32v);
                                    break;
                }
            }
            int64_t wroteSize = fwrite(&number, size, 1, pdat);
            if (wroteSize != 1) {
                printf("fwrite(fileName=%s) failed!\n", g_filename);
                return -1;
            }
            g_current += size;
            if (g_digress == 0) {
                value += g_interval;
            } else {
                value -= g_interval;
            }
        }
        // usleep(200);
    }
    uint64_t delta = getMsecTime() - begin;
    fclose(pdat);
    double speed = (g_current * 1.0f) * 1000 / (1048576 * delta * 1.0f);
    fprintf(stdout, "\n%lu bytes number written done, average speed was %.3f MB/s.\n", g_current, speed);
    if (g_addRandFrame) {
        printf("--- frame size %zu, written 0x%lx frames\n", (sizeof(ProjectFrame)* idx), (idx - 1));
    }

    char time[128] = { 0 };
    sprintf(time, "%ld\n", header.utctime);
    if (tmf != NULL) {
        int64_t wroteSize = fwrite(&time, strlen(time), 1, tmf);
        if (wroteSize != 1) {
            fprintf(stderr, "fwrite(fileName=%s) failed!\n", timefile);
        } else {
            printf("fwrite(fileName=%s) utctime=%ld success!\n", timefile, header.utctime);
        }
        fclose(tmf);
    }
}

void byteSwap16(uint16_t* val)
{
    uint16_t v1 = (*val & 0xff00) >> 8;
    uint16_t v0 = (*val & 0x00ff) << 8;
    *val = (v1 | v0);
}

void byteSwap32(uint32_t* val)
{
    uint32_t v = *val;
    *val = (((v & 0xff) << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | ((v >> 24) & 0xff));
}

void byteSwap24(uint32_t* val)
{
    uint32_t v = *val;
    *val = (((v & 0xff) << 16) | ((v & 0xff0000) >> 16)) & 0xffffff;
}

bool isSmallEndian()
{
    union {
        int n;
        char c;
    } v;
    v.n = 1;
    if (v.c == 1) {
        return false;
    }
    return true;
}

uint64_t getMsecTime()
{
    uint64_t msec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    msec = tv.tv_sec * 1000ULL + tv.tv_usec / 1000;
    return msec;
}

void makeDirExist(const char* dir)
{
    char szTmpPath[260];
    char szPath[260];

    memset(szPath, 0, sizeof(szPath));
    sprintf(szPath, "%s", dir);

    int len = strlen(szPath);
    int index = 1;
    while (index < len) {
        if ((szPath[index] == '/') || (szPath[index] == '\\')) {
            memset(szTmpPath, 0, sizeof(szTmpPath));
            strncpy(szTmpPath, szPath, index);
            umask(0);
#ifdef _WIN32
            mkdir(szTmpPath);
#else
            mkdir(szTmpPath, 0755);
#endif
            if (memcmp(szTmpPath, ".", 2) != 0) {
                fprintf(stdout, "mkdir [%s] successfully.\n", szTmpPath);
            }
        }
        index++;
    }
}

uint64_t sizeConvert(const std::string& sizeVal)
{
    uint64_t u64_size = 0;
    /* 将TB，GB，或MB等换算成B */
    for (size_t i = 0; i < sizeVal.size(); ++i) {
        if (sizeVal[i] >= '0' && sizeVal[i] <= '9') {
            u64_size = u64_size * 10 + sizeVal[i] - '0';
        } else if (sizeVal[i] == 'T') {
            u64_size = u64_size * 1024 * 1024 * 1024 * 1024;
        } else if (sizeVal[i] == 'G') {
            u64_size *= 1024 * 1024 * 1024;
        } else if (sizeVal[i] == 'M') {
            u64_size *= 1024 * 1024;
        } else if (sizeVal[i] == 'K') {
            u64_size *= 1024;
        }
    }
    return u64_size;
}

void conStringToVec(std::string str, std::vector<uint32_t>& vec)
{
    vec.clear();
    while (!str.empty()) {
        std::string::size_type pos = str.find(",");
        std::string val = str.substr(0, pos);
        uint32_t tmp = strtol(val.c_str(), NULL, 16);
        vec.push_back(tmp);
        if (pos == std::string::npos) {
            break;
        }
        str = str.substr(pos + 1);
    }
}

void usageExit(int exitcode, char** argv)
{
    fprintf(stderr,
        "\nVersion 1.0.7\nUsage: %s [options] [VALUE]\n"
        "software by sy, list supported options below:\n"
        "\n"
        "-f | --file         FILENAME      name of the file to write, required.\n"
        "-t | --total        VALUE[K/M/G]  total size to write, required.\n"
        "-e | --endian       0/1           big endian(1) or small endian(0). (default 0)\n"
        "-s | --start        HEX           start number(0x123) to write. (default 0x0)\n"
        "                                  multi-channels if split by ','. (eg. 0x108,0x109,0x10a)\n"
        "-i | --interval     VALUE         interval value between next number. (default 1)\n"
        "-d | --digress      0/1           increasing(0) or decreasing(1). (default 0)\n"
        "-b | --bits         8/16/32/64    bit width to every number. (default 32)\n"
        "-Fx| --frame                      add random frame header.\n"
        "\n",
        argv[0]);

    exit(exitcode);
}

void parseArgs(int argc, char** argv)
{
    char* tail = NULL;
    static struct option opts[] =
    {
        {"file", required_argument, NULL, 'f'},
        {"total", required_argument, NULL, 't'},
        {"endian", no_argument, NULL, 'e'},
        {"start", no_argument, NULL, 's'},
        {"interval", no_argument, NULL, 'i'},
        {"digress", no_argument, NULL, 'd'},
        {"bits", no_argument, NULL, 'b'},
        {"frame", no_argument, NULL, 'F'},
        {0} };

    while (1) {
        int idx = 0;
        std::string chans = "";
        int c = getopt_long(argc, argv, "f:t:e:s:i:d:b:F:", opts, &idx);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            g_filename = optarg;
            break;
        case 't':
            g_total = sizeConvert(optarg);
            break;
        case 'e':
            g_endian = strtoul(optarg, &tail, 10);
            if (*tail) {
                fprintf(stderr, "invalid argument to endian: %s\n", optarg);
                usageExit(1, argv);
            }
            break;
        case 's':
            g_vecStart.clear();
            chans = optarg;
            conStringToVec(chans, g_vecStart);
            break;
        case 'i':
            g_interval = strtoul(optarg, &tail, 10);
            if (*tail) {
                fprintf(stderr, "invalid argument to interval: %s\n", optarg);
                usageExit(1, argv);
            }
            break;
        case 'd':
            g_digress = strtoul(optarg, &tail, 10);
            if (*tail) {
                fprintf(stderr, "invalid argument to increding or decreasing: %s\n", optarg);
                usageExit(1, argv);
            }
            break;
        case 'b':
            g_bits = strtoul(optarg, &tail, 10);
            if (*tail) {
                fprintf(stderr, "invalid argument to number bits: %s\n", optarg);
                usageExit(1, argv);
            }
            break;
        case 'F':
            g_addRandFrame = true;
            break;
        case 'h':
            usageExit(0, argv);
        case '?':
            usageExit(1, argv);
        default:
            abort();
        }
    }

    argv += optind;
    argc -= optind;

    if (argc > 1) {
        fprintf(stderr, "[ERROR] invalid arguments.\n");
        usageExit(0, argv);
    }
}
