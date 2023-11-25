#include <stdlib.h>
#include <string>
#include <cstdint>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#include <vector>

struct Runtime {
    bool kmg;
    float prog;
    uint64_t bytes;
    uint64_t total;
};

namespace {
    const char* g_file = "test";
    uint64_t g_total = 1048576;
    int g_bits = 32;
    bool g_decrease = 0;
    int g_endian = 0;
    int g_interval = 1;
    std::vector<uint64_t> g_begins;
    Runtime g_runtime = {};
};

union Number {
    // uint8_t _8v;
    uint16_t _16v;
    uint32_t _32v;
    uint64_t _64v;
};

void parse_args(int argc, char** argv);
void usage_exit(const char* argv0 = "");
uint64_t gettime4usec();
bool isSmallEndian();
void byteSwap16(uint16_t* val);
void byteSwap321(uint32_t* val);
void byteSwap32(uint32_t* val);
uint64_t size2bytes(const std::string& value);
void msleep(unsigned long ms);

template<class T>
std::vector<T> string2Vector(const std::string& str, const char* split = ",")
{
    std::vector<T> vec;
    char* s = const_cast<char*>(str.c_str());
    char* p = strtok(s, split);
    T a;
    while (p != nullptr) {
        sscanf(p, "%lx", &a);
        vec.push_back(a);
        p = strtok(nullptr, split);
    }
    return vec;
}

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        usage_exit(argv[0]);
    }
    parse_args(argc, argv);
    std::thread task(
        [&]()->void {
            while (true) {
                msleep(100);
                if (!g_runtime.kmg) {
                    continue;
                }
                g_runtime.prog = g_runtime.bytes * 100.f / g_runtime.total;
                fprintf(stdout, "\r%.3f %%", g_runtime.prog);
                fflush(stdout);
            }
        }
    );
    if (task.joinable()) {
        task.detach();
    }
    size_t size = 0;
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
    case 64:
        size = sizeof(uint64_t);
        break;
    case 32:
    default:
        size = sizeof(uint32_t);
        break;
    }
    FILE* fp = fopen(g_file, "wb+");
    if (fp == nullptr) {
        fprintf(stderr, "fwrite open '%s' failed: %s\n", g_file, strerror(errno));
        return -1;
    }
    size_t length = g_begins.size();
    std::vector<uint64_t> values = g_begins;
    if (length == 0) {
        values.push_back(0);
        length++;
    }
    g_runtime.total = g_total * length;
    if (g_total < size) {
        fprintf(stderr, "Total size must upper than unit size, actually: %lu < %zu.\n", g_total, size);
        usage_exit(argv[0]);
    } else {
        g_runtime.kmg = true;
    }
    uint64_t count = g_total / size;
    uint64_t start = gettime4usec();
    for (uint64_t i = 0; i < count; i++) {
        for (size_t i = 0; i < length; i++) {
            Number number;
            number._64v = values[i];
            if (byteswap) {
                switch (size) {
                    case sizeof(uint16_t) :
                        byteSwap16(reinterpret_cast<uint16_t*>(&number._16v));
                        break;
                        case sizeof(uint64_t) :
#ifdef __GNUC__
                            number._64v = __builtin_bswap64(reinterpret_cast<uint64_t>(number._64v));
#else
                            if (small) {
                                number._64v = htonll(number._64v);
                            } else {
                                number._64v = ntohll(number._64v);
                            }
#endif
                        break;
                        case sizeof(uint32_t) :
                        default:
                            byteSwap32(reinterpret_cast<uint32_t*>(&number._32v));
                            break;
                }
            }
            int64_t wroteSize = fwrite(&number, size, 1, fp);
            if (wroteSize != 1) {
                fprintf(stderr, "fwrite(file=%s) failed: %s\n", g_file, strerror(errno));
                return -2;
            }
            g_runtime.bytes += size;
            if (!g_decrease) {
                values[i] += g_interval;
            } else {
                values[i] -= g_interval;
            }
        }
    }
    fclose(fp);
    fprintf(stdout, "\n%lu bytes write done, average speed %.3f M/s\n", reinterpret_cast<uint64_t>(length * count * size),
        (g_runtime.total * 1.f) / (gettime4usec() - start) * 0x100000 / 1000000.f);
}

uint64_t gettime4usec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000ULL + tv.tv_usec);
}

bool isSmallEndian()
{
    union {
        int i;
        char c;
    } v;
    v.i = 1;
    return (!(v.c == 1));
}

void byteSwap16(uint16_t* val)
{
    uint16_t v1 = (*val & 0xff00) >> 8;
    uint16_t v0 = (*val & 0x00ff) << 8;
    *val = (v1 | v0);
}

void byteSwap321(uint32_t* val)
{
    uint32_t v3 = ((uint32_t)(*val) & 0xff000000) >> 24;
    uint32_t v2 = ((uint32_t)(*val) & 0x00ff0000) >> 8;
    uint32_t v1 = ((uint32_t)(*val) & 0x0000ff00) << 8;
    uint32_t v0 = ((uint32_t)(*val) & 0x000000ff) << 24;
    *val = (uint32_t)(v0 | v1 | v2 | v3);
}

void byteSwap24(uint32_t* val)
{
    uint32_t v1 = ((uint32_t)(*val) & 0xff) << 16;
    uint32_t v0 = ((uint32_t)(*val) & 0xff0000) >> 16;
    *val = (uint32_t)(v0 | v1);
}

void byteSwap32(uint32_t* val)
{
    uint32_t v = *val;
    *val = (((v & 0xff00) << 24)
        | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8)
        | ((v >> 24) & 0xff));
}

uint64_t size2bytes(const std::string& value)
{
    uint64_t u64_size = 0;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] >= '0' && value[i] <= '9') {
            u64_size = u64_size * 10 + value[i] - '0';
        } else if (value[i] == 'T') {
            u64_size = u64_size * 1024 * 1024 * 1024 * 1024;
        } else if (value[i] == 'G') {
            u64_size *= 1024 * 1024 * 1024;
        } else if (value[i] == 'M') {
            u64_size *= 1024 * 1024;
        } else if (value[i] == 'K') {
            u64_size *= 1024;
        } else {
            continue;
        }
    }
    return u64_size;
}

void usage_exit(const char* argv0)
{
    fprintf(stderr,
        "\nUsage: %s [option] ARGUMENT\n"
        "\n"
        "-f --file      FILENAME        Name of the file to save, required.\n"
        "-n --total     SIZE(K/M/G)     Number of total size to write, required.\n"
        "-b --bits      8/16/32/64      Bit width of every number. (default: 32)\n"
        "-d --decrease  0/1             Number to be increasing(0) or decreasing(1). (default: 0)\n"
        "-e --endian    1/0             Big endian(1) or small endian(0). (default: 0)\n"
        "-i --interval  VALUE           Interval value between next number. (default: 1)\n"
        "-s --start     HEX             Start number value 0x123. (default: 0x0)\n"
        "\n",
        argv0
    );
    exit(0);
}

void parse_args(int argc, char** argv)
{
    static struct option opts[] = {
            { "file",     required_argument, NULL, 'f' },
            { "total",    required_argument, NULL, 'n' },
            { "bits",     no_argument,       NULL, 'b' },
            { "decrease", no_argument,       NULL, 'd' },
            { "endian",   no_argument,       NULL, 'e' },
            { "interval", no_argument,       NULL, 'i' },
            { "start",    no_argument,       NULL, 's' },
            { 0 }
    };
    while (1) {
        int idx;
        char* tail;
        int c = getopt_long(argc, argv, "f:n:b:d:e:i:s:", opts, &idx);
        if (c == -1) break;

        switch (c) {
        case 'f':
            g_file = optarg;
            break;
        case 'n':
            if (std::string(optarg).find("0x") == 0) {
                g_total = strtoul(optarg, &tail, 16);
                if (*tail) {
                    fprintf(stderr,
                        "invalid argument to start: %s\n",
                        optarg);
                    usage_exit(argv[0]);
                }
            } else {
                g_total = size2bytes(optarg);
            }
            break;
        case 'b':
            g_bits = atoi(optarg);
            break;
        case 'd':
            g_decrease = (atoi(optarg) > 0 ? 1 : 0);
            break;
        case 'e':
            g_endian = (atoi(optarg) > 0 ? 1 : 0);
            break;
        case 'i':
            g_interval = atoi(optarg);
            break;
        case 's':
            g_begins = string2Vector<uint64_t>(optarg);
            break;
        case '?':
            usage_exit(argv[0]);
        case 0:
            break;
        default:
            abort();
        }
    }

    argv += optind;
    argc -= optind;

    if (argc > 1) {
        fprintf(stderr, "Too many arguments.\n");
        usage_exit(argv[0]);
    }
}

void msleep(unsigned long ms)
{
    struct timespec ts = {
        .tv_sec = static_cast<long>(ms / 1000),
        .tv_nsec = static_cast<long>((ms % 1000) * 1000000ul)
    };
    nanosleep(&ts, 0);
}
