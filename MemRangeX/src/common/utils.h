#ifndef MEMRANGEX_UTILS_H
#define MEMRANGEX_UTILS_H

#include "common_types.h"
#include <random>

namespace Utils
{
    TimeNs GetCurrentNanosecond();
    TimeMs GetCurrentMillisecond();
    void SleepMs(TimeMs ms);
    float RandomFloat0To1();
    void BindThreadToNuma(int numa_id);
    std::vector<std::string> SplitString(const std::string& str, char delim);
    uint32 HashKey(KeyType key);
}

#endif