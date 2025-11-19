#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include "common.h"
#include "SeekDbMgr.h"

#ifdef __cplusplus
extern "C"
{
#endif

    class SeekTime {
    public:
        SeekTime();
        virtual ~SeekTime();

        int init(const std::string& dbName = "./seekTime");
        void uninit();
        int seekFileDataTime(uint32_t duration, std::vector<SeekTimeContent>& fileinfos, uint32_t offset = PROJECT_FILE_OFFSET);
        void setFilesTime(const std::vector<SelectTime>& times, const std::string& file);
        int getFileTime(const std::string& file, SelectTime& time, uint32_t offset = PROJECT_FILE_OFFSET);

        SelectTime getTimeDuration(std::vector<std::string> files);

    private:
        int findFileFragmentDetail(FILE* file, FileDataFrame& startframe, FileDataFrame& tailframe);

    private:
        std::map<std::string, std::vector<SelectTime> > m_seekTimeMap{};
        SeekDbMgr* m_dbMgr = NULL;
        FileDataFrame m_dataFrame{};
        uint32_t m_windSize = 0x10000;
        uint32_t m_maxFrameSize = 0x100000;
        FILE* m_pfile = NULL;
        bool m_uninit = false;
        SelectTime m_timeDuration{};
    };

#ifdef __cplusplus
}
#endif
