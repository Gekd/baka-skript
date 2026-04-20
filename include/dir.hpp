#pragma once
#include "types.hpp"
#include "csv.hpp"

std::vector<DirInfo> dirStructure(fs::path dirPath);

std::vector<std::string> dir(const fs::path &path);

std::chrono::seconds dirNameToS(std::string name);
