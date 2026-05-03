#pragma once
#include "types.hpp"
#include "time.hpp"

std::vector<CSVRow> readCsv(fs::path path, int timezone);

CSVRowIndex columnMap(const std::string &row);

std::vector<std::string> rowSplit(const std::string &row);

std::chrono::seconds csvNameToS(const std::string &name);