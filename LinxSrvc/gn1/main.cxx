#if (defined _WIN32) && (!defined __GNUC__)
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#ifdef small
#undef small
#endif
#define required_argument NULL
#define no_argument NULL
#define optarg NULL
#define optind NULL
#define getopt_long() {}
struct option { const char* _1; void* _2; void* _3; char _4; };
#else
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#ifdef OpenMP
#include <omp.h>
#endif

#if (defined _WIN32) && (!defined __GNUC__)
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);
static void gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;
    tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
}
static void waitUs(unsigned long usec)
{
    HANDLE timer;
    LARGE_INTEGER interval;
    interval.QuadPart = -long(10 * usec);
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &interval, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

struct Runtime {
    bool kmg;
    float prog;
    uint64_t bytes;
    uint64_t total;
};

struct FrameField {
    std::string name;
    int length;  // size of the field in bytes
    bool is_size; // is this field a size field?
    bool has_fixed_value; // is this field a fixed value?
    uint64_t fixed_value; // constant value if has_fixed_value is true
};

namespace {
    const char* g_file = "test";
    uint64_t g_total = 1048576;
    int g_bits = 32;
    bool g_decrease = 0;
    int g_endian = 0;
    int g_interval = 1;
    std::vector<uint64_t> g_begins;
    std::string g_frame_desc; // frame header description
    std::vector<FrameField> g_frame_fields; // parsed frame fields
    size_t g_header_len = 0; // total length of the frame header fields
    Runtime g_runtime = {};
};

union Number {
    // uint8_t _8v;
    uint16_t _16v;
    uint32_t _32v;
    uint64_t _64v;
};

void usage_exit(const char* argv0 = "");
uint64_t size2bytes(const std::string& value);
void parse_args(int argc, char** argv);
void msleep(unsigned long ms);
bool isSmallEndian();
void byteSwap16(uint16_t* val);
void byteSwap32(uint32_t* val);
void byteSwap64(uint64_t* val);
uint64_t gettime4usec();
std::vector<std::string> split_string(const std::string& s, char delimiter);
std::vector<FrameField> parse_frame_fields(const std::string& frame_desc);
void write_field_value(FILE* fp, uint64_t value, int length);

int main(int argc, char* argv[])
{
#if (defined _WIN32) && (!defined __GNUC__)
    fprintf(stdout, "\nOptions: file '%s' size=%lld, %d bits, interval=%d, %s, %s.\n", g_file, g_total, g_bits, g_interval,
        g_decrease ? "decrease" : "increase", g_endian ? "bigendian" : "small-endian");
#else
    if (argc <= 1) {
        usage_exit(argv[0]);
    }
    parse_args(argc, argv);
#endif

    // 解析帧头描述
    if (!g_frame_desc.empty()) {
        g_frame_fields = parse_frame_fields(g_frame_desc);
        for (const auto& field : g_frame_fields) {
            g_header_len += field.length;
        }
    }

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
                if (g_runtime.prog >= 100.0f) {
                    break;
                }
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

    // calculate frame size
    size_t payload_len = length * size; // incremental numbers part
    size_t frame_len = g_header_len + payload_len; // total frame length
    if (g_total < frame_len) {
        fprintf(stderr, "Total size must be at least one frame (%zu bytes), actually: %lu.\n",
            frame_len, g_total);
        usage_exit(argv[0]);
    }

    // total frame count
    uint64_t frame_count = g_total / frame_len;
    g_runtime.total = frame_count * frame_len;
    g_runtime.kmg = true;

    uint64_t start = gettime4usec();
    int status = 0;

    for (uint64_t frame_idx = 0; frame_idx < frame_count; frame_idx++) {
        // write frame header
        if (!g_frame_fields.empty()) {
            for (const auto& field : g_frame_fields) {
                uint64_t value = 0;
                if (field.is_size) {
                    value = frame_len; // dynamic size of the frame
                } else if (field.has_fixed_value) {
                    value = field.fixed_value;
                }
                write_field_value(fp, value, field.length);
                g_runtime.bytes += field.length;
            }
        }

        // write incremental values
        for (size_t i = 0; i < length; i++) {
            Number number{};
            number._64v = values[i];
            if (byteswap) {
                switch (size) {
                    case sizeof(uint16_t) :
                        byteSwap16(reinterpret_cast<uint16_t*>(&number._16v));
                        break;
                        case sizeof(uint64_t) :
                            byteSwap64(reinterpret_cast<uint64_t*>(&number._64v));
                            break;
                            case sizeof(uint32_t) :
                            default:
                                byteSwap32(reinterpret_cast<uint32_t*>(&number._32v));
                                break;
                }
            }
            size_t wroteSize = fwrite(&number, size, 1, fp);
            if (wroteSize != 1) {
                fprintf(stderr, "fwrite(file=%s) failed: %s\n", g_file, strerror(errno));
                status = -2;
                break;
            }
            g_runtime.bytes += size;
            if (!g_decrease) {
                values[i] += g_interval;
            } else {
                values[i] -= g_interval;
            }
        }
        if (status < 0) break;
    }

    fclose(fp);
    if (status < 0) return status;
    uint64_t total_bytes = frame_count * frame_len;
    uint64_t elapsed = gettime4usec() - start;
    double speed = (elapsed > 0) ? (total_bytes * 1.0 / elapsed) * 1000000.0 / (1024 * 1024) : 0;
    fprintf(stdout, "\n%lu bytes write done, average speed %.3f MB/s.\n",
        total_bytes, speed);
    return 0;
}

// divide a string by a delimiter
std::vector<std::string> split_string(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

// parse command line arguments
std::vector<FrameField> parse_frame_fields(const std::string& frame_desc)
{
    std::vector<FrameField> fields;
    std::vector<std::string> field_descs = split_string(frame_desc, ',');
    for (const auto& desc : field_descs) {
        FrameField field;
        size_t pos1 = desc.find(':');
        if (pos1 == std::string::npos) {
            fprintf(stderr, "Invalid frame field: %s\n", desc.c_str());
            continue;
        }
        field.name = desc.substr(0, pos1);
        std::string rest = desc.substr(pos1 + 1);
        size_t pos2 = rest.find('=');
        if (pos2 != std::string::npos) {
            std::string len_str = rest.substr(0, pos2);
            std::string value_str = rest.substr(pos2 + 1);
            try {
                field.length = std::stoi(len_str);
            } catch (...) {
                field.length = 0;
            }
            if (value_str.substr(0, 2) == "0x") {
                field.fixed_value = std::stoull(value_str.substr(2), nullptr, 16);
            } else {
                field.fixed_value = std::stoull(value_str);
            }
            field.has_fixed_value = true;
        } else {
            try {
                field.length = std::stoi(rest);
            } catch (...) {
                field.length = 0;
            }
            field.has_fixed_value = false;
            field.fixed_value = 0;
        }
        field.is_size = (field.name == "size");
        if (field.length <= 0) {
            fprintf(stderr, "Invalid field length: %d for field '%s'\n", field.length, field.name.c_str());
            continue;
        }
        fields.push_back(field);
    }
    return fields;
}

// write a field value to the file
void write_field_value(FILE* fp, uint64_t value, int length)
{
    // check if value exceeds the maximum for the given length
    uint64_t max_value = (length == 8) ? 0xFFFFFFFFFFFFFFFFULL : (1ULL << (length * 8)) - 1;
    if (value > max_value) {
        value = value & max_value; // Truncate the overflow part
    }

    bool need_swap = (g_endian == 1) != isSmallEndian();
    if (length == 1) {
        uint8_t v = static_cast<uint8_t>(value);
        fwrite(&v, 1, 1, fp);
    } else if (length == 2) {
        uint16_t v = static_cast<uint16_t>(value);
        if (need_swap) byteSwap16(&v);
        fwrite(&v, 1, 2, fp);
    } else if (length == 4) {
        uint32_t v = static_cast<uint32_t>(value);
        if (need_swap) byteSwap32(&v);
        fwrite(&v, 1, 4, fp);
    } else if (length == 8) {
        uint64_t v = static_cast<uint64_t>(value);
        if (need_swap) byteSwap64(&v);
        fwrite(&v, 1, 8, fp);
    } else {
        // Unsupported length, write byte by byte
        for (int i = 0; i < length; i++) {
            uint8_t byte = (value >> (i * 8)) & 0xFF;
            fwrite(&byte, 1, 1, fp);
        }
    }
}

template<class T>
std::vector<T> string2vector(const std::string& str, const char* split = ",")
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
    return (v.c == 1);
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
    *val = (((v & 0x000000FF) << 24) |
        ((v & 0x0000FF00) << 8) |
        ((v & 0x00FF0000) >> 8) |
        ((v & 0xFF000000) >> 24));
}

void byteSwap64(uint64_t* val)
{
    uint64_t v = *val;
    *val = (((v & 0x00000000000000FFULL) << 56) |
        ((v & 0x000000000000FF00ULL) << 40) |
        ((v & 0x0000000000FF0000ULL) << 24) |
        ((v & 0x00000000FF000000ULL) << 8) |
        ((v & 0x000000FF00000000ULL) >> 8) |
        ((v & 0x0000FF0000000000ULL) >> 24) |
        ((v & 0x00FF000000000000ULL) >> 40) |
        ((v & 0xFF00000000000000ULL) >> 56));
}

uint64_t size2bytes(const std::string& value)
{
    uint64_t u64_size = 0;
    size_t pos = value.find_first_of("KMGTP");
    std::string num_part = value.substr(0, pos);
    std::string unit_part = (pos != std::string::npos) ? value.substr(pos, 1) : "";
    double dsize = 0;
    try {
        dsize = std::stod(num_part);
    } catch (...) {
        dsize = 0;
    }
    if (unit_part == "T") {
        dsize *= 1024 * 1024 * 1024 * 1024ULL;
    } else if (unit_part == "G") {
        dsize *= 1024 * 1024 * 1024ULL;
    } else if (unit_part == "M") {
        dsize *= 1024 * 1024ULL;
    } else if (unit_part == "K") {
        dsize *= 1024ULL;
    }
    u64_size = static_cast<uint64_t>(dsize);
    return u64_size;
}

void usage_exit(const char* argv0)
{
    fprintf(stderr,
        "\nUsage: %s [options] ARGUMENT\n"
        "\n"
        "-f | --file      FILENAME        Name of the file to save, required.\n"
        "-n | --total     SIZE(K/M/G)     Number of total size to write, required.\n"
        "-b | --bits      8/16/32/64      Bit width of every number. (default: 32)\n"
        "-d | --decrease  0/1             Number to be increasing(0) or decreasing(1). (default: 0)\n"
        "-e | --endian    1/0             Big endian(1) or small endian(0). (default: 0)\n"
        "-i | --interval  VALUE           Interval value between next number. (default: 1)\n"
        "-s | --start     HEX             Start number value 0x123. (default: 0x0)\n"
        "                                 Multi-channels if separate by ','. (eg.: 0x0,0x321,0xff)\n"
        "-F | --frame     DESC            Frame header description (eg: \"magic:2=0xAA55,size:2,seq:1\")\n"
        "                                 Fields: name:length[=fixed_value], 'size' field will be auto-filled\n"
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
            { "frame",    no_argument,       NULL, 'F' },
            { 0 }
    };
    while (1) {
        int idx;
        char* tail;
        int c = getopt_long(argc, argv, "f:n:b:d:e:i:s:F:", opts, &idx);
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
            g_begins = string2vector<uint64_t>(optarg);
            break;
        case 'F':
            g_frame_desc = optarg;
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
#if (defined _WIN32) && (!defined __GNUC__)
    waitUs(1000 * ms);
#else
    struct timespec ts = {
        .tv_sec = static_cast<long>(ms / 1000),
        .tv_nsec = static_cast<long>((ms % 1000) * 1000000ul)
    };
    nanosleep(&ts, 0);
#endif // _WIN32
}
