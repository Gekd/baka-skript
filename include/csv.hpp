#pragma once
#include "types.hpp"
#include "time.hpp"

std::vector<CSVRow> readCsv(fs::path path, int timezone);

CSVRowIndex columnMap(std::string row);

std::vector<std::string> rowSplit(std::string row);

std::chrono::seconds csvNameToS(std::string name);