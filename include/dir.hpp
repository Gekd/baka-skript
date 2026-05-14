#pragma once
#include "types.hpp"
#include "csv.hpp"

std::vector<DirInfo> scanDir(fs::path dirPath);

std::chrono::seconds dirNameToS(std::string name);
