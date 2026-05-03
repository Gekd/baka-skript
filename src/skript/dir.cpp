#include "dir.hpp"

// Finds the path/name of the files
std::vector<std::string> dir(const fs::path &path) {
  std::vector<std::string> result{};

  for (const fs::path &dir : fs::directory_iterator(path)) {
    if (!fs::is_directory(dir)) {
      continue;
    }

    for (const fs::path &file : fs::directory_iterator(dir)) {
      std::string pathExtension = file.extension().generic_string();
      std::transform(pathExtension.begin(), pathExtension.end(), pathExtension.begin(), ::tolower);
      
      if (fs::is_regular_file(file) && (pathExtension == ".mp4" || pathExtension == ".srt")) {
        std::string fileString = file.generic_string();
        std::string name = fileString.substr(0, fileString.size()-5);
        result.push_back(name);
        break;
      }
    } 
  }
  return result;
}

// Scans through given path and creates map of the dir+files
std::vector<DirInfo> dirStructure(fs::path dirPath) {
  std::vector<DirInfo> result{};
  std::vector<fs::path> csvs{};

  for (const auto &dir : fs::directory_iterator(dirPath)) {
    const auto &dirPath = dir.path();
    if (!dir.is_directory()) {
      if (dirPath.extension() == ".csv") {
        csvs.push_back(dir.path());
      }
    } else {
      DirInfo dirInfo{};
      dirInfo.path = dirPath;
      dirInfo.name = dirPath.filename().string();
      dirInfo.fileGroups = std::vector<FileGroup>{};

      std::unordered_map <std::string, FileGroup> fileGroup{};

      for (const auto &file : fs::directory_iterator(dir.path())) {
        if (!file.is_regular_file()) {
          continue;
        }

        const auto &filePath = file.path();
        const auto fileExtension = filePath.extension().generic_string();

        if (fileExtension != ".mp4" && fileExtension != ".srt" && fileExtension != ".MP4" && fileExtension != ".SRT") {
          continue;
        }

        const auto fileNameWithoutExtension = filePath.stem().generic_string();

        bool thermalFile = fileNameWithoutExtension.ends_with("_T");
        bool rgbFile = fileNameWithoutExtension.ends_with("_V");

        std::string fileKey = fileNameWithoutExtension.substr(0, fileNameWithoutExtension.size() - 2);

        // If the file group already exists, add the file to it, otherwise create a new file group
        auto &group = fileGroup[fileKey];
        if (fileExtension == ".srt" || fileExtension == ".SRT") {
          if (thermalFile) {
            group.srtTPath = file;
          } else if (rgbFile) {
            group.srtVPath = file;
          }
        } else if (fileExtension == ".mp4" || fileExtension == ".MP4") {
          if (thermalFile) {
            group.mp4TPath = file;
          } else if (rgbFile) {
            group.mp4VPath = file;
          }
        }
      }
      for (const auto &[name, group] : fileGroup) {
        dirInfo.fileGroups.push_back(group);
      }
      result.push_back(dirInfo);
    }
  }
  for (auto csv : csvs) {
    std::string csvNameWithExtension = csv.filename().generic_string();
    std::chrono::seconds csvTime = csvNameToS(csvNameWithExtension);

    for (auto &dirInfo : result) {
      std::chrono::seconds dirTime = dirNameToS(dirInfo.name);

      if ((dirTime <= csvTime) && (abs(csvTime - dirTime) < std::chrono::seconds(360))) {
        dirInfo.csvPath = csv;
      }
    }
  }
 return result;
}

std::chrono::seconds dirNameToS(std::string name) {
  std::string timeStr = name.substr(name.size()-8, 4);
  std::string hours = timeStr.substr(0,2); 
  std::string minutes = timeStr.substr(2,2); 
  return std::chrono::seconds{stoi(hours)*3600 + stoi(minutes)*60};
}
