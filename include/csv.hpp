#pragma once
#include "types.hpp"
#include "timeparse.hpp"

std::vector<CSVRow> readCsv(fs::path path);

CSVRowIndex columnMap(const std::string &row);

std::vector<std::string> rowSplit(const std::string &row);

std::chrono::seconds csvNameToS(const std::string &name);