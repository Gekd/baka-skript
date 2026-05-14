#include "file.hpp"
#include "timeparse.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include "wildnav.hpp"
#include "opencv2/opencv.hpp"


// Returns list of frames where lat or long has changed compared to previous frames
std::vector<FrameData> extractGpsFrames(const fs::path &path, int timezone) {
  std::vector<FrameData> result{};

  if (!fs::is_regular_file(path) || path.extension() != ".SRT") {
    std::cout << "Invalid SRT file path: " << path << '\n';
    return result;
  }

  std::ifstream is;

  is.open(path);
  if (!is) {
    std::cout << "Failed to open SRT file: " << path << '\n';
    return result;
  }

  std::string row = "";

  std::regex regexpCoord("latitude: (-?[0-9]+\\.[0-9]+)\\] \\[longitude: (-?[0-9]+\\.[0-9]+)");
  std::smatch matchCoord;

  std::regex regexpFrame("FrameCnt: ([0-9]+)\\,");
  std::smatch matchFrame;
  
  std::regex regexpTime("[0-9]{2}\\:[0-9]{2}\\:[0-9]{2}\\.[0-9]{3}");
  std::smatch matchTime;
  
  std::regex regexpDateTime("[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}\\:[0-9]{2}\\:[0-9]{2}\\.[0-9]{3}");
  std::smatch matchDateTime;
  
  std::regex regexpAlt("rel_alt: (-?[0-9]+\\.[0-9]+) abs_alt: (-?[0-9]+\\.[0-9]+)");
  std::smatch matchAlt;

  double previousLat = 0;
  double previousLong = 0;
  long frame = 0;
  std::string time = "";
  std::string dateTime = "";
  float relAlt = 0;
  float absAlt = 0;

  const double latLongTolerance = 0.0000001;

  while (getline(is, row)) {
    std::regex_search(row, matchCoord, regexpCoord);
    std::regex_search(row, matchFrame, regexpFrame);
    std::regex_search(row, matchTime, regexpTime);
    std::regex_search(row, matchDateTime, regexpDateTime);
    std::regex_search(row, matchAlt, regexpAlt);

    if (!matchFrame.empty()) {
      frame = stol(matchFrame[1]);
    }

    if (!matchTime.empty()) {
      time = matchTime[0];
    }
    if (!matchDateTime.empty()) {
      dateTime = matchDateTime[0];
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
        auto toMsI = toMs(dateTime, timezone);
        result.push_back(FrameData{frame, lat, lon, time, toMsI, relAlt, absAlt});
      }
    }    
  }
  return result;
}

std::vector<FrameDataPair> matchImages(const std::vector<FrameData> &rgb, const std::vector<FrameData> &thermal) {
  bool rgbSmaller = rgb.size() <= thermal.size();
  const auto smallerVector = rgbSmaller ? &rgb : &thermal;
  const auto largerVector = rgbSmaller ? &thermal : &rgb;

  std::vector<FrameDataPair> result{};
  result.reserve(smallerVector->size());
  
  size_t index = 0;

  for (const auto &frame : *smallerVector) {
    auto bestTimeDifference = std::numeric_limits<long>::max();
    const FrameData *bestMatch{};

    while (index < largerVector->size() && largerVector->at(index).timeMS <= frame.timeMS - VIDEO_FRAME_TOLERANCE_MS) {
      index++;
    }

    size_t tempIndex = index;
    while (tempIndex < largerVector->size() && largerVector->at(tempIndex).timeMS < frame.timeMS + VIDEO_FRAME_TOLERANCE_MS)  {
      auto timeDifference = std::labs(frame.timeMS.count() - largerVector->at(tempIndex).timeMS.count());
      if (timeDifference < bestTimeDifference) {
        bestTimeDifference = timeDifference;
        bestMatch = &largerVector->at(tempIndex);
      } 
      tempIndex++;
    }

    if (bestMatch) {
      if (rgbSmaller) {
        result.push_back(FrameDataPair{frame, *bestMatch});
      } else {
        result.push_back(FrameDataPair{*bestMatch, frame});
      }
    }
  }
  return result;
}

CSVRow findGPSMatch(const FrameData &frameData, const std::vector<CSVRow> &csvRows) {  
  if (csvRows.empty()) {
    return CSVRow{};
  }

  int bestIndex{};
  double bestDistance = std::numeric_limits<double>::max();

  for (size_t i = 0; i < csvRows.size(); i++) {
    const auto &csvRow = csvRows[i];

    double distance = haversine(frameData.lat, frameData.lon, csvRow.latitude, csvRow.longitude);
    
    if (distance < bestDistance) {
      bestDistance = distance;
      bestIndex = i;
    }
  }

  if (bestDistance <= MAX_FRAME_CSVROW_DIFFERENCE_METERS) {
    return csvRows[bestIndex];
  } 
  return CSVRow{};
}


std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows) {
  std::vector<FrameDataPairCSV> result{};

  for (const auto &framePair : frameDataPairs) {
    const FrameData &rgbFrame = framePair.first;

    auto bestMatch = findGPSMatch(rgbFrame, csvRows);

    if (!bestMatch.dateTimeMS.count()) {
      std::cout << "No matching GPS data found for frame: " << rgbFrame.timeMS.count() << '\n';
      continue;
    }


    long diff = std::labs((long long)rgbFrame.timeMS.count() - (long long)bestMatch.dateTimeMS.count());

    if (diff <= MAX_FRAME_CSVROW_DIFFERENCE_MS.count()) {
      result.push_back(FrameDataPairCSV{framePair, bestMatch});
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

void output(const std::vector<CSVRow> &csv, const FileGroup &fileGroup, const fs::path outputPath, int timezone, bool rgbOutput, bool thermalOutput) {
  std::string fileGroupName = fileGroup.mp4VPath.stem().string();
  fileGroupName = fileGroup.parentDirName + "/" + fileGroupName.substr(0, fileGroupName.size() - 2);
  
  if (!rgbOutput && !thermalOutput) {
    std::cout << "No output selected. Skipping file group: " << fileGroupName << '\n';
    return;
  }

  std::cout << "Processing file group: " << fileGroupName << '\n';

  std::vector<FrameDataPair> framePairs{};

  if (rgbOutput && thermalOutput) {
    std::vector<FrameData> framesT = extractGpsFrames(fileGroup.srtTPath, timezone);
    std::vector<FrameData> framesV = extractGpsFrames(fileGroup.srtVPath, timezone);

    framePairs = matchImages(framesV, framesT);

    std::cout << fileGroupName << ":" << '\n' 
      << '\t' << "Total thermal frames in "  << ": " << framesT.size() << '\n' 
      << '\t' << "Total RGB frames in "  << ": " << framesV.size() << '\n' 
      << '\t' << "Total matched " << framePairs.size() << '\n';


  } else {
    const bool rgbOnly = rgbOutput;
    const fs::path selectedSrtPath = rgbOnly ? fileGroup.srtVPath : fileGroup.srtTPath;
    std::vector<FrameData> selectedFrames = extractGpsFrames(selectedSrtPath, timezone);
    std::cout << (rgbOnly ? "Total RGB frames in " : "Total thermal frames in ") << fileGroupName << ": " << selectedFrames.size() << '\n';
    framePairs.reserve(selectedFrames.size());
    for (const auto &frame : selectedFrames) {
      framePairs.push_back(FrameDataPair{frame, frame});
    }
  }

  std::vector<FrameDataPairCSV> data = matchPairsToCSV(framePairs, csv);

  if (data.size() == 0) {
    std::cout << "No matching data to process for " << fileGroupName << '\n';  
    return;
  }

  std::cout << fileGroupName << ":" << '\n'
    << '\t' << "Total matched frames with GPS data: " << data.size() << '\n';

  wildnavOutput(fileGroup, data, outputPath, rgbOutput, thermalOutput);
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