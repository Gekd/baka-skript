#include <iostream>
#include <filesystem>
#include <bits/stdc++.h>
#include <CLI/CLI.hpp>
#include "file.hpp"
#include "dir.hpp"
#include "threadPool.hpp"

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
    fs::path path = fs::path("../data");

    std::vector<DirInfo> dirs = dirStructure(path);

    int maximumThreads = std::thread::hardware_concurrency();
    ThreadPool pool(maximumThreads);

    for (const auto &dir : dirs) {        
      auto csvData = std::make_shared<std::vector<CSVRow>>(readCsv(dir.csvPath, timezone));

      for (const auto &fileGroup : dir.fileGroups) {
    
        pool.enqueue([csvData, fileGroup, dir, timezone](){
          output(*csvData, fileGroup, fs::path("../output/" + dir.name), timezone);
        });
      }
    
    }
  }
  return 0;
}

