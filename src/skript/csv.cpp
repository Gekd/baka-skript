#include "csv.hpp"
#include <fstream>


std::vector<CSVRow> readCsv(fs::path path, int timezone) {
  std::vector<CSVRow> result{};
  std::ifstream is;

  is.open(path);
  if (!is) return result;

  std::string row = "";
  std::string cell = "";

  CSVRowIndex columnIndexes{};
  
  for (size_t i = 0; getline(is, row); i++) {
    if (i == 0) continue;
    if (i == 1) {
      columnIndexes = columnMap(row);  
      continue;
    }

    std::vector<std::string> rowElements = rowSplit(row);
    CSVRow csvRow{};
    

    if (columnIndexes.cameraIsVideo != -1) {
      const auto &cameraIsVideo = rowElements[columnIndexes.cameraIsVideo];
      if (cameraIsVideo != "True") {
        continue;    
      } else {
        csvRow.cameraIsVideo = cameraIsVideo;
      }
    }

    if (columnIndexes.gimbalPitch != -1) {
      const auto &gimbalPitch = rowElements[columnIndexes.gimbalPitch];
      if (gimbalPitch.empty()){
        continue;
      }
      float gimbalPitchF = std::stof(gimbalPitch);
      if (gimbalPitchF <= -87.5) {
        csvRow.gimbalPitchDown = true;
      }
      csvRow.gimbalPitch = gimbalPitchF;
    }

    if (columnIndexes.updateTime != -1) {
      const auto &updateTimeStr = rowElements[columnIndexes.updateTime];
      if (!updateTimeStr.empty()) {
        csvRow.updateTime = updateTimeStr;    
        csvRow.updateTimeMs = csvTimeToMS(updateTimeStr, timezone).count();
      }
    }

    if (columnIndexes.flyTime != -1) {
      const auto &flyTimeStr = rowElements[columnIndexes.flyTime];
      if (!flyTimeStr.empty()) {
        csvRow.flyTime = flyTimeStr;
      }
    }

    if (columnIndexes.flyTimeS != -1) {
      const auto &flyTimeSStr = rowElements[columnIndexes.flyTimeS];
      if (!flyTimeSStr.empty()) {
        csvRow.flyTimeS = std::stof(flyTimeSStr);
      }
    }

    if (columnIndexes.latitude != -1) {
      const auto &latitudeStr = rowElements[columnIndexes.latitude];
      if (!latitudeStr.empty()) {
        csvRow.latitude = std::stod(latitudeStr);
      }
    }

    if (columnIndexes.longitude != -1) {
      const auto &longitudeStr = rowElements[columnIndexes.longitude];
      if (!longitudeStr.empty()) {
        csvRow.longitude = std::stod(longitudeStr);
      }
    }

    if (columnIndexes.heightFT != -1) {
      const auto &heightFTStr = rowElements[columnIndexes.heightFT];
      if (!heightFTStr.empty()) {
        csvRow.heightFT = std::stoi(heightFTStr);
      }
    }

    if (columnIndexes.altitudeFT != -1) {
      const auto &altitudeFTStr = rowElements[columnIndexes.altitudeFT];
      if (!altitudeFTStr.empty()) {
        csvRow.altitudeFT = std::stof(altitudeFTStr);
      }
    }

    if (columnIndexes.pitch != -1) {
      const auto &pitchStr = rowElements[columnIndexes.pitch];
      if (!pitchStr.empty()) {
        csvRow.pitch = std::stof(pitchStr);
      }
    }

    if (columnIndexes.roll != -1) {
      const auto &rollStr = rowElements[columnIndexes.roll];
      if (!rollStr.empty()) {
        csvRow.roll = std::stof(rollStr);
      }
    }

    if (columnIndexes.yaw != -1) {
      const auto &yawStr = rowElements[columnIndexes.yaw];
      if (!yawStr.empty()) {
        csvRow.yaw = std::stof(yawStr);
      }
    }

    if (columnIndexes.yaw360 != -1) {
      const auto &yaw360Str = rowElements[columnIndexes.yaw360];
      if (!yaw360Str.empty()) {
        csvRow.yaw360 = std::stof(yaw360Str);
      }
    }

    if (columnIndexes.directionOfTravel != -1) {
      const auto &directionOfTravelStr = rowElements[columnIndexes.directionOfTravel];
      if (!directionOfTravelStr.empty()) {
        csvRow.directionOfTravel = std::stof(directionOfTravelStr);
      }
    }

    if (columnIndexes.gimbalMode != -1) {
      const auto &gimbalModeStr = rowElements[columnIndexes.gimbalMode];
      if (!gimbalModeStr.empty()) {
        csvRow.gimbalMode = gimbalModeStr;
      }
    }

    if (columnIndexes.gimbalRoll != -1) {
      const auto &gimballRollStr = rowElements[columnIndexes.gimbalRoll];
      if (!gimballRollStr.empty()) {
        csvRow.gimbalRoll = std::stof(gimballRollStr);
      }
    }

    if (columnIndexes.gimbalYaw != -1) {
      const auto &gimbalYawStr = rowElements[columnIndexes.gimbalYaw];
      if (!gimbalYawStr.empty()) {
        csvRow.gimbalYaw = std::stof(gimbalYawStr);
      }
    }

    if (columnIndexes.gimbalYaw360 != -1) {
      const auto &gimbalYaw360Str = rowElements[columnIndexes.gimbalYaw360];
      if (!gimbalYaw360Str.empty()) {
        csvRow.gimbalYaw360 = std::stof(gimbalYaw360Str);
      }
    }

    result.push_back(csvRow);
  }
  return result;
}

CSVRowIndex columnMap(std::string row) {
  CSVRowIndex result{};
  std::stringstream ss{row};

  std::string cell = "";
  int i = 0;

  while (getline(ss, cell, ',')) {
    auto it = csvLookup.find(cell);
    if (it != csvLookup.end()) {
      result.*(it->second) = i;
    }
    i++;
  }
  return result;
}


std::vector<std::string> rowSplit(std::string row) {
  std::vector<std::string> result{};
  std::stringstream ss{row};

  std::string cell = "";

  while (getline(ss, cell, ',')) {
    result.push_back(cell);
  }
  return result;
}

std::chrono::seconds csvNameToS(std::string name) {
  std::string timeStr = name.substr(name.size()-13, 8);
  std::string hours = timeStr.substr(0,2); 
  std::string minutes = timeStr.substr(3,2); 
  std::string seconds = timeStr.substr(6,2);
  return std::chrono::seconds{stoi(hours)*3600 + stoi(minutes)*60 + stoi(seconds)};
}