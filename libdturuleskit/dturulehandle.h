#ifndef _DTU_RULE_HANDLE_H_
#define _DTU_RULE_HANDLE_H_

#include <vector>
#include <mutex>
#include <tuple>
#include <map>

#include "dtustructs.h"
#include "dtubuffer.h"
#include "dtucommon.h"

namespace DTU
{
    class dtuRuleHandler
    {
        public:
            static int readParam(uint16_t &group, std::vector<uint32_t> &vecfix, std::vector<DTU::buffer> &result, RemoteCtrlInfo &rinfo);
            static int PresetParam(const DTU::buffer& setInfo, uint16_t group);
            // 撤销预置区域
            static int revertPreset();
            // 保存预置区域
            static int savePreset();
            static int readFileDirectory(const std::string &dir, std::vector<DTU::buffer> &result, uint8_t flag, uint64_t tbegin, uint64_t tend);
            static int readFileActive(const std::string &fileName, DTU::buffer &result);
            static int readFileContent(uint32_t fileID, DTU::buffer &content);
            static int writeFileActive(uint32_t fileID, const std::string &fileName, uint32_t filesize);
            static int writeFileContent(int32_t fileID, uint32_t offset, uint8_t mod, DTU::buffer &content, uint8_t more);
            static int changeCurrentGroup(uint32_t group, uint32_t current = 0);
            static int readCurrentGroup(DTU::buffer &result, RemoteCtrlInfo& rinfo);
            static int remoteControl(uint16_t fix, uint16_t operate, RemoteCtrlInfo info);
            static int queryTime(DTU::buffer &result);

        private:
            static DTU::buffer MakeFileinfoToBuffer(FILEINFO &sf);
        
        private:
            // 读锁
            static std::mutex _read_lock;
            // 写锁
            static std::mutex _write_lock;
            // 保存读文件的Handle
            static std::map<uint32_t, DTU::buffer> _read_file_map;
            // 保存写文件的Handle
            static std::map<uint32_t, std::tuple<std::string, DTU::buffer>> _write_file_map;
    };
}

#endif /* _DTU_RULE_HANDLE_H_ */