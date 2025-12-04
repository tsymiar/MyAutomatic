#ifndef TIMESEEK
#include "seek/SeekTime.h"
typedef SeekTime TimeSeek;
#else
#include "time/TimeSeek.h"
#endif

using namespace std;
#define FRAGMENT_TIME_DURATION 10

int main(int argc, char* argv[])
{
    std::string fileName = "test.dat";
    if (argc > 1) {
        fileName = argv[1];
    } else {
        printf("Usage:\n\t%s [data file]\n\tconfig file is ./%s.csv\n", argv[0], argv[1]);
        return 0;
    }
    TimeSeek seekTime;
    SelectTime time;
    seekTime.init(fileName);
#ifndef TIMESEEK
    seekTime.getFileTime(fileName, time);
#endif
    printf("getFileTime start=%ld, last=%ld\n", time.first, time.last);
    std::vector<SelectTime> times;
    size_t pos = fileName.rfind('.');
    string csvpre = fileName.substr(0, pos);
    std::vector<std::string> cfgs = splitLines(getFileAsString(csvpre + ".csv"));
    if (cfgs.size() == 0) {
        printf("Error read config file!\n");
        return 0;
    }
    time = {};
    if (cfgs.size() >= 2) {
        time.first = atoll(cfgs[0].c_str());
        time.last = atoll(cfgs[1].c_str());
    }
    times.push_back(time);
    seekTime.setFilesTime(times, fileName);
    std::vector<SeekTimeContent> vecContent{};
#ifndef TIMESEEK
    seekTime.seekFileDataTime(FRAGMENT_TIME_DURATION, vecContent);
#else
    seekTime.seekFileDataTime(vecContent);
#endif
    if (vecContent.size() == 0) {
        printf("Not found seek times!\n");
    } else
        for (auto content : vecContent) {
            printf("filename[%s], len=%ld, offset=%ld, time=%ld\n", content.fileName, content.value.size, content.value.offset, content.value.timestamp);
        }
    seekTime.uninit();
}
