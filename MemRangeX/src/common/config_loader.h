#ifndef MEMRANGEX_CONFIG_LOADER_H
#define MEMRANGEX_CONFIG_LOADER_H

#include "common_types.h"
#include <string>

class ConfigLoader
{
public:
    static bool LoadConfig(const std::string& file_path, GlobalConfig& cfg);
};

#endif