#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <vector>
#include <thread>
#include <atomic>
#include <cinttypes>
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
    /* 主线程累加、进度线程读取，必须原子 */
    std::atomic<uint64_t> g_current{ 0 };
    /* 置位后进度线程退出 */
    std::atomic<bool> g_done{ false };
    std::vector<uint32_t> g_vecStart{};
    bool g_addRandFrame = true;
} // namespace

void usageExit(int exitcode, char** argv);
void parseArgs(int argc, char** argv);
bool isBigEndian();
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

struct OutputFiles {
    FILE* dat = NULL;
    FILE* csv = NULL;
    char csvName[256] = { 0 };
};

struct GenResult {
    bool ok = false;
    uint64_t frames = 0;
    uint64_t lastUtcTime = 0;
};

static size_t bytesOfBits(uint32_t bits)
{
    switch (bits) {
    case 8:  return sizeof(uint8_t);
    case 16: return sizeof(uint16_t);
    case 24: return 3 * sizeof(uint8_t);
    case 32: return sizeof(uint32_t);
    case 64: return sizeof(uint64_t);
    /* 非法位宽返回 0，由调用方报错，避免静默按 32 位写出错误数据 */
    default: return 0;
    }
}

static bool openOutputs(const char* filename, OutputFiles& out)
{
    makeDirExist(filename);
    out.dat = fopen(filename, "wb+");
    if (out.dat == NULL) {
        fprintf(stderr, "fwrite open failed: %s.\n", filename);
        return false;
    }
    std::string base = filename;
    /* 只在最后一段路径里找扩展名，目录名中的 '.' 不能被当成后缀切断 */
    std::string::size_type slash = base.find_last_of("/\\");
    std::string::size_type dot = base.find_last_of('.');
    if ((dot != std::string::npos) && (slash != std::string::npos) && (dot < slash)) {
        dot = std::string::npos;
    }
    std::string stem = base.substr(0, dot);
    int len = snprintf(out.csvName, sizeof(out.csvName), "%s.csv", stem.c_str());
    if (len < 0 || (size_t)len >= sizeof(out.csvName)) {
        fprintf(stderr, "csv name too long (max %zu): %s\n", sizeof(out.csvName) - 1, filename);
        fclose(out.dat);
        out.dat = NULL;
        return false;
    }
    out.csv = fopen(out.csvName, "wb+");
    if (out.csv == NULL) {
        fprintf(stderr, "fwrite open failed: %s.\n", out.csvName);
        fclose(out.dat);
        out.dat = NULL;
        return false;
    }
    return true;
}

/* fclose 失败意味着缓冲没落盘，必须当成写失败上报 */
static bool closeFile(FILE*& fp, const char* name)
{
    if (fp == NULL) {
        return true;
    }
    FILE* tmp = fp;
    fp = NULL;
    if (fclose(tmp) != 0) {
        fprintf(stderr, "fclose(fileName=%s) failed!\n", name);
        return false;
    }
    return true;
}

static bool writeUtcTime(FILE* csv, const char* csvName, uint64_t utctime)
{
    char time[64] = { 0 };
    int len = snprintf(time, sizeof(time), "%" PRIu64 "\n", utctime);
    /* snprintf 返回的是"想写多长"，超过缓冲就截断，照它写会越界 */
    if (len < 0 || (size_t)len >= sizeof(time)) {
        fprintf(stderr, "snprintf failed\n");
        return false;
    }
    if (fwrite(time, 1, (size_t)len, csv) != (size_t)len) {
        fprintf(stderr, "fwrite(fileName=%s) failed!\n", csvName);
        return false;
    }
    printf("fwrite(fileName=%s) utctime=%" PRIu64 " success!\n", csvName, utctime);
    return true;
}

static int64_t msecOfDayStart(int64_t nowMsec)
{
    time_t now = nowMsec / 1000; // s
    struct tm* currentTime = localtime(&now);
    if (currentTime == NULL) {
        return 0;
    }
    return now * 1000 - 3600000 * (currentTime->tm_hour % 24)
        - 60000 * currentTime->tm_min - 1000 * currentTime->tm_sec;
}

static std::thread startProgress(uint64_t total)
{
    return std::thread([total]() -> void {
        while (!g_done.load(std::memory_order_relaxed)) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 100000000; // 100 ms
            nanosleep(&ts, NULL);
            if (total == 0) {
                continue;
            }
            fprintf(stdout, "\rprogress: %.2f %% ",
                g_current.load(std::memory_order_relaxed) * 100.f / total);
            fflush(stdout);
        }
        printf("\n");
    });
}

static void appendNumber(std::vector<uint8_t>& cache, uint64_t value, size_t size, bool byteswap)
{
    Number number;
    number._64v = value;
    if (byteswap) {
        switch (size) {
        /* 单字节没有字节序，按 32 位交换会把高位字节(恒为 0)当成数据写出去 */
        case sizeof(uint8_t): break;
        case sizeof(uint16_t): byteSwap16((uint16_t*)&number._16v); break;
        case 3 * sizeof(uint8_t): byteSwap24((uint32_t*)&number._24v); break;
        case sizeof(uint64_t): number._64v = __builtin_bswap64(number._64v); break;
        case sizeof(uint32_t): byteSwap32((uint32_t*)&number._32v); break;
        default: break;
        }
    }
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&number);
    cache.insert(cache.end(), bytes, bytes + size);
}

/* 攒满一块再落盘，减少 fwrite 调用次数 */
static bool flushCache(std::vector<uint8_t>& cache, FILE* dat)
{
    if (cache.empty()) {
        return true;
    }
    bool ok = (fwrite(cache.data(), 1, cache.size(), dat) == cache.size());
    cache.clear();
    return ok;
}

/* 落盘与发布进度必须成对：进度线程读的是 g_current，只清缓存不发布它就看不到进展 */
static bool flushAndPublish(std::vector<uint8_t>& cache, FILE* dat, uint64_t& pending)
{
    bool ok = flushCache(cache, dat);
    if (pending != 0) {
        g_current.fetch_add(pending, std::memory_order_relaxed);
        pending = 0;
    }
    return ok;
}

static bool writeFrameHeader(OutputFiles& out, std::vector<uint8_t>& cache,
    ProjectFrame& header, size_t rand0, size_t size, int64_t startOfDayTime,
    bool first, uint64_t& pending)
{
    header.size = rand0 * size;
    header.utctime = getMsecTime() - startOfDayTime;
    if (first && !writeUtcTime(out.csv, out.csvName, header.utctime)) {
        return false;
    }
    /* 帧头必须紧跟上一帧的数据，先把缓存冲掉再写头 */
    if (!flushAndPublish(cache, out.dat, pending)
        || fwrite(&header, sizeof(ProjectFrame), 1, out.dat) != 1) {
        printf("fwrite(fileName=%s) failed!\n", g_filename);
        return false;
    }
    return true;
}

static GenResult generateData(OutputFiles& out, size_t size, bool byteswap,
    uint64_t value, int64_t startOfDayTime)
{
    GenResult res;
    const size_t cacheLimit = 64 * 1024;
    std::vector<uint8_t> cache{};
    cache.reserve(cacheLimit + size);
    unsigned int seed = (unsigned int)time(NULL);
    ProjectFrame header{};
    bool failed = false;
    /* 进度只用于显示，本地累加后整块发布，省掉每个数字一次原子加 */
    uint64_t pending = 0;
    while (!failed && ((g_current.load(std::memory_order_relaxed) + pending) < g_total)) {
        /* rand0 每轮都要算：它还负责推进 g_current，漏算则循环不终止 */
        uint32_t tmp = rand_r(&seed);
        size_t rand0 = tmp % 2048 + 1;
        if (g_addRandFrame) {
            if (!writeFrameHeader(out, cache, header, rand0, size,
                    startOfDayTime, res.frames == 0, pending)) {
                failed = true;
                break;
            }
            res.frames++;
            pending += sizeof(ProjectFrame);
        }
        for (size_t j = 0; j < rand0 && !failed; j++) {
            appendNumber(cache, value, size, byteswap);
            pending += size;
            if (g_digress == 0) {
                value += g_interval;
            } else {
                value -= g_interval;
            }
            if (cache.size() >= cacheLimit
                && !flushAndPublish(cache, out.dat, pending)) {
                printf("fwrite(fileName=%s) failed!\n", g_filename);
                failed = true;
            }
        }
    }
    if (!flushAndPublish(cache, out.dat, pending)) {
        printf("fwrite(fileName=%s) failed!\n", g_filename);
        failed = true;
    }
    res.ok = !failed;
    res.lastUtcTime = header.utctime;
    return res;
}

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        usageExit(0, argv);
    }
    parseArgs(argc, argv);
    size_t size = bytesOfBits(g_bits);
    if (size == 0) {
        fprintf(stderr, "invalid bits %" PRIu32 ", only 8/16/24/32/64 allowed.\n", g_bits);
        usageExit(1, argv);
    }
    bool byteswap = (g_endian == 0) ? isBigEndian() : !isBigEndian();

    OutputFiles out;
    if (!openOutputs(g_filename, out)) {
        return -1;
    }

    if (g_vecStart.size() == 0) {
        g_vecStart.push_back(0);
    }
    if (g_vecStart.size() != 1) {
        fprintf(stderr, "start number count=%zu not allow, only allow one member\n",
            g_vecStart.size());
        closeFile(out.dat, g_filename);
        closeFile(out.csv, out.csvName);
        return -1;
    }
    uint64_t value = g_vecStart[0];

    std::thread progress = startProgress(g_total);

    printf("fwrite(fileName=%s) begin!\n", g_filename);
    int64_t begin = getMsecTime();
    int64_t startOfDayTime = msecOfDayStart(begin);
    GenResult res = generateData(out, size, byteswap, value, startOfDayTime);
    g_done = true;
    if (progress.joinable()) {
        progress.join();
    }
    if (!res.ok) {
        closeFile(out.dat, g_filename);
        closeFile(out.csv, out.csvName);
        return -1;
    }
    /* 先收 dat 再掐表：fclose 的回写时间也是写入耗时的一部分 */
    bool okDat = closeFile(out.dat, g_filename);
    uint64_t delta = getMsecTime() - begin;
    double speed = (delta == 0) ? 0.0
        : ((double)g_current.load() * 1000 / (1048576.0 * (double)delta));
    fprintf(stdout, "\n%" PRIu64 " bytes number written done, average speed was %.3f MB/s.\n",
        g_current.load(), speed);
    if (g_addRandFrame && res.frames > 0) {
        printf("--- frame size %zu, written 0x%" PRIx64 " frames\n",
            (sizeof(ProjectFrame) * res.frames), (res.frames - 1));
        /* 没有帧头就没有时间戳可记，写了只会落一个无意义的 0 */
        writeUtcTime(out.csv, out.csvName, res.lastUtcTime);
    }
    bool okCsv = closeFile(out.csv, out.csvName);
    return (okDat && okCsv) ? 0 : -1;
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
    /* 低字节与高字节对调，中间字节必须原样保留，否则 24 位数据被清零 */
    *val = (((v & 0xff) << 16) | (v & 0xff00) | ((v & 0xff0000) >> 16)) & 0xffffff;
}

bool isBigEndian()
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
    if (dir == NULL) {
        fprintf(stderr, "dir is null.\n");
        return;
    }
    char szPath[260];
    char szMkDir[260];
    size_t szPos = 1;
    int copied = snprintf(szPath, sizeof(szPath), "%s", dir);
    if (copied < 0 || (size_t)copied >= sizeof(szPath)) {
        /* 截断后逐级 mkdir 会建错目录，真正的上级目录反而没建 */
        fprintf(stderr, "dir path too long (max %zu): %s\n", sizeof(szPath) - 1, dir);
        return;
    }
    size_t len = strnlen(szPath, sizeof(szPath) - 1);
    while (szPos < len) {
        if ((szPath[szPos] == '/') || (szPath[szPos] == '\\')) {
            if (szPos >= sizeof(szMkDir)) {
                szPos++;
                continue;
            }
            memcpy(szMkDir, szPath, szPos);
            szMkDir[szPos] = '\0';
#ifdef _WIN32
            mkdir(szMkDir);
#else
            mkdir(szMkDir, 0700);
#endif
        }
        szPos++;
    }
}

/* 判乘法溢出 */
static bool sizeMulOverflow(uint64_t value, uint64_t mul)
{
    return (mul != 0) && (value > (UINT64_MAX / mul));
}

uint64_t sizeConvert(const std::string& sizeVal)
{
    uint64_t u64_size = 0;
    /* 将TB，GB，或MB等换算成B */
    for (size_t i = 0; i < sizeVal.size(); ++i) {
        uint64_t mul = 0;
        uint64_t add = 0;
        if (sizeVal[i] >= '0' && sizeVal[i] <= '9') {
            mul = 10;
            add = (uint64_t)(sizeVal[i] - '0');
        } else if (sizeVal[i] == 'T') {
            mul = 1024ULL * 1024 * 1024 * 1024;
        } else if (sizeVal[i] == 'G') {
            mul = 1024ULL * 1024 * 1024;
        } else if (sizeVal[i] == 'M') {
            mul = 1024ULL * 1024;
        } else if (sizeVal[i] == 'K') {
            mul = 1024;
        } else {
            continue; // 非法字符原样忽略
        }
        /* 回绕会让 g_total 变成离谱的值，直接报错返回 0 */
        if (sizeMulOverflow(u64_size, mul) || (u64_size > (UINT64_MAX - add))) {
            fprintf(stderr, "total size overflow: %s\n", sizeVal.c_str());
            return 0;
        }
        u64_size = u64_size * mul + add;
    }
    return u64_size;
}

void conStringToVec(std::string str, std::vector<uint32_t>& vec)
{
    vec.clear();
    while (!str.empty()) {
        std::string::size_type pos = str.find(",");
        std::string val = str.substr(0, pos);
        char* tail = NULL;
        errno = 0;
        unsigned long long tmp = strtoull(val.c_str(), &tail, 16);
        /* 越界就报错，静默截断成 UINT32_MAX 会写出和用户输入不符的数据 */
        if ((tail == val.c_str()) || (errno == ERANGE) || (tmp > UINT32_MAX)) {
            fprintf(stderr, "invalid start number: %s\n", val.c_str());
        } else {
            vec.push_back((uint32_t)tmp);
        }
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
        "software by sy, list supported options below:\n\n"
        "-f | --file         FILENAME      name of the file to write, required.\n"
        "-t | --total        VALUE[K/M/G]  total size to write, required.\n"
        "-e | --endian       0/1           big endian(1) or small endian(0). (default 0)\n"
        "-s | --start        HEX           start number(0x123) to write. (default 0x0)\n"
        "                                  multi-channels if split by ','. (eg. 0x108,0x109,0x10a)\n"
        "-i | --interval     VALUE         interval value between next number. (default 1)\n"
        "-d | --digress      0/1           increasing(0) or decreasing(1). (default 0)\n"
        "-b | --bits         8/16/24/32/64 bit width to every number. (default 32)\n"
        "-F | --frame        [0/1]         add random frame header(1) or not(0). (default 1)\n"
        "\n",
        argv[0]);

    exit(exitcode);
}

/* --opt v 的值 getopt 不消费，补一步前看；返回 NULL 表示没给值，取默认值 */
static const char* getValue(int argc, char** argv)
{
    if (optarg != NULL)
        return optarg;
    if (optind < argc && argv[optind] != NULL && argv[optind][0] != '-')
        return argv[optind++];
    return NULL;
}

static struct option g_opts[] =
{
    {"file", required_argument, NULL, 'f'},
    {"total", required_argument, NULL, 't'},
    { "endian", optional_argument, NULL, 'e' },
    { "start", optional_argument, NULL, 's' },
    { "interval", optional_argument, NULL, 'i' },
    { "digress", optional_argument, NULL, 'd' },
    { "bits", optional_argument, NULL, 'b' },
    {"frame", optional_argument, NULL, 'F'},
    {0} };

/* 取 --opt 的可选值并转成 uint32_t；没给值时保持原默认值 */
static bool parseOptUInt(int argc, char** argv, uint32_t& out, const char* what)
{
    const char* val = getValue(argc, argv);
    if (val == NULL) {
        return true;
    }
    if (val[0] == '\0') {
        fprintf(stderr, "empty argument to %s\n", what);
        return false;
    }
    char* tail = NULL;
    errno = 0;
    /* 空串、负数、超范围一律拒绝：静默截断会写出与用户输入不符的数据 */
    unsigned long long num = strtoull(val, &tail, 10);
    if ((tail == val) || (*tail != '\0') || (errno == ERANGE) || (num > UINT32_MAX)) {
        fprintf(stderr, "invalid argument to %s: %s\n", what, val);
        return false;
    }
    out = (uint32_t)num;
    return true;
}

/* 取 --start 的可选值并解析成起始值列表；没给值时保持原默认值 */
static void parseOptStart(int argc, char** argv)
{
    const char* val = getValue(argc, argv);
    if (val == NULL) {
        return;
    }
    g_vecStart.clear();
    conStringToVec(std::string(val), g_vecStart);
}

static void handleOption(int c, int argc, char** argv)
{
    switch (c) {
    case 'f':
        g_filename = optarg;
        break;
    case 't':
        g_total = sizeConvert(optarg);
        break;
    case 'e':
        if (!parseOptUInt(argc, argv, g_endian, "endian")) {
            usageExit(1, argv);
        }
        break;
    case 's':
        parseOptStart(argc, argv);
        break;
    case 'i':
        if (!parseOptUInt(argc, argv, g_interval, "interval")) {
            usageExit(1, argv);
        }
        break;
    case 'd':
        if (!parseOptUInt(argc, argv, g_digress, "digress")) {
            usageExit(1, argv);
        }
        break;
    case 'b':
        if (!parseOptUInt(argc, argv, g_bits, "number bits")) {
            usageExit(1, argv);
        }
        break;
    case 'F':
        /* -F/--frame 等价于开；显式给 0(-F0/--frame=0)才关 */
        g_addRandFrame = (optarg == NULL) || (strcmp(optarg, "0") != 0);
        break;
    case 'h':
        usageExit(0, argv);
    case '?':
        usageExit(1, argv);
    default:
        abort();
    }
}

void parseArgs(int argc, char** argv)
{
    char** argvBase = argv;

    while (1) {
        int idx = 0;
        int c = getopt_long(argc, argv, "f:t:e:s:i:d:b:F::", g_opts, &idx);
        if (c == -1)
            break;
        handleOption(c, argc, argv);
    }

    /* total 为 0 说明没给或解析失败，继续跑只会写出空文件 */
    if (g_total == 0) {
        fprintf(stderr, "[ERROR] invalid or missing total size.\n");
        usageExit(1, argvBase);
    }

    argc -= optind;

    if (argc > 1) {
        fprintf(stderr, "[ERROR] invalid arguments.\n");
        usageExit(1, argvBase);
    }
}
