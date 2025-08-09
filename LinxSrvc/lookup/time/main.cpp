#include "SeekTime.h"
#include "Common.h"

int main(int argc, char* argv[])
{
    std::string file = "test.dat";
    if (argc > 1) {
        file = argv[1];
    }
    SeekTime seekTime;
    seekTime.init(file + ".db");
    std::vector<SelectTime> times;
    std::vector<std::string> cfgs = splitLines(getFileAsString("./time.cfg"));
    if (cfgs.size() == 0) {
        printf("Error reading config file!\n");
        return 0;
    }
    for (int i = 0; i < cfgs.size(); i++) {
        std::vector<std::string> stTimes{};
        stringToVector(cfgs[i], stTimes);
        uint64_t first = atoll(stTimes[1].c_str()) - 500;
        uint64_t last = atoll(stTimes[1].c_str()) + 500;
        SelectTime time = { first, last };
        time.fix();
        times.push_back(time);
    }
    seekTime.setFilesTime(times, file);
    std::vector<SeekTimeContent> vecContent{};
    seekTime.seekFileDataTime(vecContent);
    if (vecContent.size() == 0) {
        printf("Not found seek times!\n");
    } else
        for (auto content : vecContent) {
            if (content.found)
                printf("Found filename[%s], len=%lu, total=%lu, offset=%lu, time=%lu, delta=[%lu]\n",
                    content.fileName, content.value.size, content.totalSize, content.value.offset, content.value.timestamp, content.param - content.value.timestamp);
            else
                printf("Not Found filename[%s], expect time=%lu\n", content.fileName, content.param);
        }
    seekTime.uninit();
}
