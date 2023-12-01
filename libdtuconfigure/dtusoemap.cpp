#include "dtusoemap.h"
#include "dtudbmanager.h"

using namespace DTU;

void SOEMap::init()
{
    auto& ret = DBManager::instance().GetSOEIndex();
    int count = 1;

    for (const auto& item : ret)
    {
        HIOA hioa = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.innerno);

        if (item.second.innerno < 32) {
            // 硬件遥信不在这里进行管理
            // 这里只管理软遥信
            continue;
        }

        soemap.insert({hioa, false});
        soestrmap.insert({hioa, item.second.desc});
        // printf("[%02d]HIOA [0x%04X] desc [%s]\n", count++, hioa, item.second.desc.c_str());
    }

}

bool SOEMap::updateYXStateByDevno(uint16_t devioa, bool state)
{
    bool result = false;

    std::lock_guard<std::mutex> LOCK(lock);

    HIOA hioa = DBManager::instance().FixidMapIntoout(MAP_YX, devioa);

    auto ita = soemap.find(hioa);

    if (ita != soemap.end()) {
        ita->second = state;
        result = true;
    }

    return result;
}

bool SOEMap::updateYXStateByHIOA(HIOA hioa, bool state)
{
    bool result = false;

    std::lock_guard<std::mutex> LOCK(lock);

    auto ita = soemap.find(hioa);

    if (ita != soemap.end()) {
        ita->second = state;
        result = true;
    }

    return result;
}

bool SOEMap::getYXState(HIOA hioa, bool& state)
{
    bool result = false;

    std::lock_guard<std::mutex> LOCK(lock);

    auto ita = soemap.find(hioa);

    if (ita != soemap.end()) {
        state = ita->second;
        result = true;
    }

    return result;
}

std::string SOEMap::getYXDesc(HIOA hioa)
{
    std::string result;

    std::lock_guard<std::mutex> LOCK(lock);

    auto ita = soestrmap.find(hioa);

    if (ita != soestrmap.end()) {
        result = ita->second;
    }

    return result;
}

std::map<HIOA, bool> &SOEMap::getAllState()
{
    return this->soemap;
}