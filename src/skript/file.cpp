#include "file.hpp"
#include "time.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include "wildnav.hpp"
#include "opencv2/opencv.hpp"


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

  double previousLat = 0;
  double previousLong = 0;
  long frame = 0;
  std::string time = "";
  float relAlt = 0;
  float absAlt = 0;

  const double latLongTolerance = 0.0000001;

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
      double lat = stod(matchCoord[1]);
      double lon = stod(matchCoord[2]);

      if (abs(lat - previousLat) > latLongTolerance || abs(lon - previousLong) > latLongTolerance) {
        previousLat = lat;
        previousLong = lon;
        FrameData frameData = FrameData{frame, lat, lon, time, toMs(time).count(), relAlt, absAlt};
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
  const long toleranceMS = 500;

  bool rgbSmaller = rgb.size() <= thermal.size();
  auto smallerVector = rgbSmaller ? &rgb : &thermal;
  auto largerVector = rgbSmaller ? &thermal : &rgb;

  std::vector<FrameDataPair> result{};
  result.reserve(smallerVector->size());
  
  long unsigned int index = 0;

  for (const auto &frame : *smallerVector) {
    auto bestTimeDifference = std::numeric_limits<long>::max();
    const FrameData *bestMatch{};

    for (;index < largerVector->size(); index++) {
      const FrameData &LFrame = (*largerVector)[index];
      auto timeDifference = abs(frame.timeMs - LFrame.timeMs);
      if (timeDifference <= bestTimeDifference) {
        bestTimeDifference = timeDifference;
        bestMatch = &LFrame;
      } else {
        break;
      }
    }

    if (index > 0) {
      index--;
    }

    if (bestMatch && bestTimeDifference <= toleranceMS ) {
      if (rgbSmaller) {
        result.push_back(FrameDataPair{frame, *bestMatch});
      } else {
        result.push_back(FrameDataPair{*bestMatch, frame});
      }
    }
  }
  return result;
}

bool compare(const FrameData &frameData, const CSVRow &csvRow, int timezone) {
  const double maxDifference = 6.5;
  std::chrono::milliseconds maxMSDifference{1000};

  if (abs(haversine(frameData.lat, frameData.lon, csvRow.latitude, csvRow.longitude)) > maxDifference) {
    return false;
  } 
  if (abs(frameData.timeMs - csvRow.updateTimeMs) > maxMSDifference.count()) {
    return false;
  }
  return true;
}

std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows, int timezone) {
  std::vector<FrameDataPairCSV> result{};
  long unsigned int matchedCSVIndex = 0;

  for (const auto &framePair : frameDataPairs) {
    const FrameData &rgbFrame = framePair.first;
    for (; matchedCSVIndex < csvRows.size(); matchedCSVIndex++) {
      if (compare(rgbFrame, csvRows[matchedCSVIndex], timezone)) {
        result.push_back(FrameDataPairCSV{framePair, csvRows[matchedCSVIndex]});
        break;
      }
    }
  }
  return result;
}

void upsertJson(ordered_json &json, const ordered_json &newEntry) {
  for (auto &row : json) {
    if (row["name"] == newEntry["name"]) {
      row = newEntry;
      return;
    }
  }
  json.push_back(newEntry);
}

// saves given frames 
void images(FileGroup fileGroup, bool rgb, bool thermal, const std::vector<FrameDataPairCSV> &data, fs::path outputPath, long startingIndex) {
  if (!rgb && !thermal) return;

  cv::VideoCapture vCap;
  cv::VideoCapture tCap;

  if (rgb) {
    vCap = cv::VideoCapture(fileGroup.mp4VPath);

    if(!vCap.isOpened()) {
      std::cout << "Can't open file: " << fileGroup.mp4VPath << '\n';
    }
  }

  if (thermal) {
    tCap = cv::VideoCapture(fileGroup.mp4TPath);

    if(!tCap.isOpened()) {
      std::cout << "Can't open file: " << fileGroup.mp4TPath << '\n';
    }
  }
  
  fs::path jsonPath = outputPath / "data.json";
  ordered_json root;


  if (fs::exists(jsonPath) && fs::file_size(jsonPath) > 0) {
    std::ifstream input(jsonPath);
    if (input) {
      input >> root;
    }
  }

  if (!fs::exists(outputPath)) {
    fs::create_directories(outputPath);
  }
  
  if (rgb) {
    if (!fs::exists(outputPath / "rgb")) {
      fs::create_directory(outputPath / "rgb");
    }
    if (!root.contains("rgb")) {
      root["rgb"] = json::array();
    }
  }
  if (thermal) {
    if (!fs::exists(outputPath / "thermal")) {
      fs::create_directory(outputPath / "thermal");
    }
    if (!root.contains("thermal")) {
      root["thermal"] = json::array();
    }
  }

  cv::Mat frame;

  for (long unsigned int i = 0; i < 10; i++) {  //data.size()
    if (data.at(i).second.gimbalPitchDown != true) {
      continue;
    }

    int currentRGBFrame = 0;
    int currentThermalFrame = 0;    
    
    if (rgb) {
      int targetFrame = data[i].first.first.frame;

      while (currentRGBFrame < targetFrame) {
        vCap >> frame;
        currentRGBFrame++;
      }
      upsertJson(root["rgb"], dataEntry(std::to_string(startingIndex + i)+".jpg", data.at(i).first.first, data.at(i).second));
    }

    if (thermal) {
      int targetFrame = data[i].first.second.frame;

      while (currentThermalFrame < targetFrame) {
        tCap >> frame;
        currentThermalFrame++;
      }
      upsertJson(root["thermal"], dataEntry(std::to_string(startingIndex + i)+".jpg", data.at(i).first.second, data.at(i).second));
    }
  }
  std::ofstream jsonFile(jsonPath);
  jsonFile << root.dump(2) << std::endl;
}

void output(std::vector<CSVRow> &csv, FileGroup fileGroup, const fs::path outputPath, int timezone) {
  //long startingIndex = 0;
  std::cout << "Processing file group: " << fileGroup.mp4TPath << " and " << fileGroup.mp4VPath << '\n';
  std::vector<FrameData> framesT = gpsUpdateFrames(fs::path(fileGroup.srtTPath));
  std::cout << "T: " << framesT.size() << '\n';
  std::vector<FrameData> framesV = gpsUpdateFrames(fs::path(fileGroup.srtVPath));
  std::cout << "V: " << framesV.size() << '\n';
  std::vector<FrameDataPairCSV> data = matchPairsToCSV(matchImages(framesV, framesT), csv, timezone);
  std::cout << "Matched pairs: " << data.size() << '\n';
  wildnavOutput(fileGroup, data, outputPath);
    //images(fileGroup, true, true, data, outputPath, startingIndex);
    //startingIndex += data.size();
}

// Formula used and modified for meters: https://www.geeksforgeeks.org/dsa/haversine-formula-to-find-distance-between-two-points-on-a-sphere/
double haversine(double lat1, double lon1, double lat2, double lon2) {
  // distance between latitudes and longitudes
  double dLat = (lat2 - lat1) *M_PI / 180.0;
  double dLon = (lon2 - lon1) * M_PI / 180.0;
  // convert to radians
  lat1 = (lat1) * M_PI / 180.0;
  lat2 = (lat2) * M_PI / 180.0;

  // apply formula
  double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
  double rad = 6371000;
  double c = 2 * asin(sqrt(a));
  return rad * c;
}