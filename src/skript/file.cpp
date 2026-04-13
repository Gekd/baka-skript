#include "file.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>

#include "opencv2/opencv.hpp"

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

// Returns list of frames where lat or long has changed compared to previous frames
std::vector<FrameData> gpsUpdateFrames(const fs::path &path) {
  std::vector<FrameData> result{};

  if (!fs::is_regular_file(path) || path.extension() != ".SRT") {
    return result;
  }

  std::ifstream is;
  std::stringstream ss;

  is.open(path);
  if (!is) return result;

  std::string row = "";
  std::regex regexpCoord("latitude: (-?[0-9]+\\.[0-9]+)\\] \\[longitude: (-?[0-9]+\\.[0-9]+)");
  std::smatch matchCoord;
  std::regex regexpFrame("FrameCnt: ([0-9]+)\\,");
  std::smatch matchFrame;
  std::regex regexpTime("[0-9]{2}\\:[0-9]{2}\\:[0-9]{2}\\.[0-9]{3}");
  std::smatch matchTime;
  std::regex regexpAlt("rel_alt: (-?[0-9]+\\.[0-9]+) abs_alt: (-?[0-9]+\\.[0-9]+)");
  std::smatch matchAlt;

  float previousLat = 0;
  float previousLong = 0;
  long frame = 0;
  std::string time = "";
  float relAlt = 0;
  float absAlt = 0;


  while (getline(is, row)) {
    std::regex_search(row, matchCoord, regexpCoord);
    std::regex_search(row, matchFrame, regexpFrame);
    std::regex_search(row, matchTime, regexpTime);
    std::regex_search(row, matchAlt, regexpAlt);

    if (!matchFrame.empty()) {
      frame = stol(matchFrame[1]);
    }

    if (!matchTime.empty()) {
      time = matchTime[0];
    }

    if (!matchAlt.empty()) {
      relAlt = stof(matchAlt[1]);
      absAlt = stof(matchAlt[2]);
    }

    if (!matchCoord.empty()) {
      float lat = stof(matchCoord[1]);
      float lon = stof(matchCoord[2]);

      if (lat != previousLat || lon != previousLong) {
        previousLat = lat;
        previousLong = lon;
        FrameData frameData = FrameData{frame, lat, lon, time, relAlt, absAlt};
        result.push_back(frameData);
      }
    }    
  }
  return result;
}

std::chrono::milliseconds toMs(std::string time) {
  if (time.size() < 12) {
    std::cout << "time is: " << time << '\n';
    return std::chrono::milliseconds(0);
  }
  std::string hours = time.substr(0,2);
  std::string minutes = time.substr(3,2);
  std::string seconds = time.substr(6,2);
  std::string ms = time.substr(9,3);
  
  return std::chrono::milliseconds{stoi(hours)*3600000 + stoi(minutes)*60000 + stoi(seconds)*1000 + stoi(ms)};
}

std::vector<FrameDataPair> matchImages(const std::vector<FrameData> &rgb, const std::vector<FrameData> &thermal) {
  const std::chrono::milliseconds tolerance(500);
  
  std::vector<FrameDataPair> result{};
  long unsigned int rgbIndex = 0;

  for (const auto &TFrame : thermal) {
    auto bestTimeDifference = std::chrono::milliseconds::max();
    const FrameData *bestMatch{};

    for (;rgbIndex < rgb.size(); rgbIndex++) {
      const FrameData &RGBFrame = rgb[rgbIndex];
      auto timeDifference = abs(toMs(RGBFrame.time) - toMs(TFrame.time));
      if (timeDifference <= bestTimeDifference) {
        bestTimeDifference = timeDifference;
        bestMatch = &RGBFrame;
      } else {
        break;
      }
    }

    if (rgbIndex > 0) {
      rgbIndex--;
    }

    if (bestMatch && bestTimeDifference <= tolerance ) {
      result.push_back(FrameDataPair{*bestMatch, TFrame});
    }
  }
  return result;
}

std::chrono::milliseconds csvTimeToMS(std::string time, int timezone) {
  int hours{};
  int minutes{};
  int seconds{};
  std::string msStr{};
  std::string ampm{};
  char colon{};
  char dot{};

  std::stringstream ss{time};

  ss >> hours >> colon >> minutes >> colon >> seconds >> dot >> msStr >> ampm;
  
  int milliseconds = stoi(msStr);
  if (msStr.size() == 2) {
    milliseconds *= 10;
  } else if (msStr.size() == 1) {
    milliseconds *= 100;
  } 

  if (ampm == "PM" && hours != 12) {
    hours += 12;
  }
  if (ampm == "AM" && hours == 12) {
    hours = 0;
  }
  hours += timezone;
  hours = (hours % 24 + 24) % 24;

  return std::chrono::milliseconds{hours*3600000 + minutes*60000 + seconds*1000 + milliseconds};
}

bool compare(const FrameData &frameData, const CSVRow &csvRow, int timezone) {
  double maxLatDifference = 0.00002;
  double maxLongDifference = 0.00002;
  std::chrono::milliseconds maxMSDifference{750};

  if (abs(frameData.lat - csvRow.latitude) > maxLatDifference) {
    return false;
  } 
  if (abs(frameData.lon - csvRow.longitude) > maxLongDifference) {
    return false;
  } 
  if (abs(toMs(frameData.time) - csvTimeToMS(csvRow.updateTime, timezone)) > maxMSDifference) {
    return false;
  }
  return true;
}

std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows, int timezone) {
  std::vector<FrameDataPairCSV> result{};
  long unsigned int matchedCSVIndex = 0;

  for (const auto &framePair : frameDataPairs) {
    const FrameData &rgbFrame = framePair.first;
    //const FrameData &thermalFrame = framePair.second;
    
    for (; matchedCSVIndex < csvRows.size(); matchedCSVIndex++) {
      if (compare(rgbFrame, csvRows[matchedCSVIndex], timezone)) { //&& compare(thermalFrame, csvRow, timezone)
        result.push_back(FrameDataPairCSV{framePair, csvRows[matchedCSVIndex]});
        break;
      }
    }
  }
  return result;
}

// saves given frames 
void images(std::string path, bool rgb, bool thermal, const std::vector<FrameDataPairCSV> &data, fs::path outputPath) {
  if (!rgb && !thermal) return;

  cv::VideoCapture vCap;
  cv::VideoCapture tCap;

  if (rgb) {
    fs::path vSRTPath = fs::path(path + 'V' + ".SRT");
    fs::path vMP4Path = fs::path(path + 'V' + ".MP4");
    vCap = cv::VideoCapture(vMP4Path);

    if(!vCap.isOpened()) {
      std::cout << "Can't open file: " << vMP4Path << '\n';
    }
  }

  if (thermal) {
    fs::path tSRTPath = fs::path(path + 'T' + ".SRT");
    fs::path tMP4Path = fs::path(path + 'T' + ".MP4");
    tCap = cv::VideoCapture(tMP4Path);

    if(!tCap.isOpened()) {
      std::cout << "Can't open file: " << tMP4Path << '\n';
    }
  }
  
  std::ofstream jsonFile(outputPath / "data.json");
  ordered_json root;

  if (!fs::exists(outputPath)) {
    fs::create_directory(outputPath);
  }
  
  if (rgb) {
    if (!fs::exists(outputPath / "rgb")) {
      fs::create_directory(outputPath / "rgb");
    }
    root["rgb"] = json::array();
  }
  if (thermal) {
    if (!fs::exists(outputPath / "thermal")) {
      fs::create_directory(outputPath / "thermal");
    }
    root["thermal"] = json::array();
  }

  cv::Mat frame;

  if (!fs::exists(outputPath / "data.json")) {
    std::ofstream output(outputPath / "data.json");
  }

  for (long unsigned int i = 0; i < data.size(); i++) {

    fs::create_directory(outputPath);

    if (rgb) {
      vCap.set(cv::CAP_PROP_POS_FRAMES, data.at(i).first.first.frame);
      vCap >> frame;
      cv::imwrite(outputPath.string() + "/rgb/" + std::to_string(i) + ".jpg", frame);
      root["rgb"].push_back(dataEntry(std::to_string(i)+".jpg", data.at(i).first.first, data.at(i).second));
    }

    if (thermal) {
      tCap.set(cv::CAP_PROP_POS_FRAMES, data.at(i).first.second.frame);
      tCap >> frame;
      cv::imwrite(outputPath.string() + "/thermal/" + std::to_string(i) + ".jpg", frame);
      root["thermal"].push_back(dataEntry(std::to_string(i)+".jpg", data.at(i).first.second, data.at(i).second));
    }
  }

  jsonFile << root.dump(2) << std::endl;
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

std::map<std::string, int> columnMap(std::string row) {
  std::map<std::string, int> result{};
  std::stringstream ss{row};

  std::string cell = "";
  int i = 0;

  while (getline(ss, cell, ',')) {
    result.insert({cell, i++});
  }
  return result;
}

std::vector<CSVRow> readCsv(fs::path path) {
  std::vector<CSVRow> result{};
  std::ifstream is;

  is.open(path);
  if (!is) return result;

  std::string row = "";
  std::string cell = "";

  std::map<std::string, int> columns{};
  
  for (size_t i = 0; getline(is, row); i++) {
    if (i == 0) continue;
    if (i == 1) {
      columns = columnMap(row);  
      continue;
    }

    std::vector<std::string> rowElements = rowSplit(row);
    CSVRow csvRow{};
    
    auto it = columns.find("CAMERA.isVideo");
    if (it != columns.end() && rowElements[it->second] != "True") {
      continue;    
    }

    it = columns.find("CUSTOM.updateTime [local]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.updateTime = rowElements[it->second];    
    }

    it = columns.find("OSD.flyTime");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.flyTime = rowElements[it->second];    
    }

    it = columns.find("OSD.flyTime [s]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.flyTimeS = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.latitude");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.latitude = std::stod(rowElements[it->second]);    
    }

    it = columns.find("OSD.longitude");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.longitude = std::stod(rowElements[it->second]);    
    }

    it = columns.find("OSD.height [ft]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.heightFT = std::stoi(rowElements[it->second]);    
    }

    it = columns.find("OSD.altitude [ft]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.altitudeFT = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.pitch");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.pitch = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.roll");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.roll = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.yaw");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.yaw = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.yaw [360]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.yaw360 = std::stof(rowElements[it->second]);    
    }

    it = columns.find("OSD.directionOfTravel");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.directionOfTravel = std::stof(rowElements[it->second]);
    }
     
    it = columns.find("GIMBAL.mode");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.gimbalMode = rowElements[it->second];    
    }

    it = columns.find("GIMBAL.pitch");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.gimbalPitch = std::stof(rowElements[it->second]);    
    }

    it = columns.find("GIMBAL.roll");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.gimbalRoll = std::stof(rowElements[it->second]);    
    }

    it = columns.find("GIMBAL.yaw");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.gimbalYaw = std::stof(rowElements[it->second]);    
    }

    it = columns.find("GIMBAL.yaw [360]");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.gimbalYaw360 = std::stof(rowElements[it->second]);    
    }

    it = columns.find("CAMERA.isVideo");
    if (it != columns.end() && rowElements[it->second] != "") {
      csvRow.cameraIsVideo = rowElements[it->second];    
    }

    result.push_back(csvRow);
  }
  return result;
}