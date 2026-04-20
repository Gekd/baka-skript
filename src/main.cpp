#include <iostream>
#include <filesystem>
#include "file.hpp"
#include "dir.hpp"
#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
  CLI::App app{"Skript"};

  bool wildnav{false};
  app.add_flag("-w,--wildnav", wildnav, "Use WILDNAV format");

  bool thermal{false};
  app.add_flag("-t,--thermal", thermal, "Output contains thermal images");

  bool rgb{false};
  app.add_flag("-r,--rgb", rgb, "Output contains RGB images");

  int timezone{};
  app.add_option("-z,--timezone", timezone, "Timezone offset")->required();

  CLI11_PARSE(app, argc, argv);

  if (wildnav) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  
    fs::path path = fs::path("../data");

    std::vector<DirInfo> dirs = dirStructure(path);

    for (const auto &dir : dirs) {
      std::cout << "Processing: " << dir.csvPath << '\n';
      std::vector<CSVRow> csvData = readCsv(fs::path(dir.csvPath), timezone);
      output(csvData, dir.fileGroups, fs::path("../output/" + dir.name), timezone);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;

  }
  return 0;
}

