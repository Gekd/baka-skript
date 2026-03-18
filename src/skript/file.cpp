#include "file.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>


#include "opencv2/opencv.hpp"



// Finds the path/name of the files
std::vector<std::string> dir(fs::path path) {
  std::vector<std::string> result{};

  for (const fs::path& dir : fs::directory_iterator(path)) {
    if (!fs::is_directory(dir)) {
      continue;
    }

    for (const fs::path& file : fs::directory_iterator(dir)) {
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
std::vector<FrameLatLongTime> gpsUpdateFrames(fs::path path) {
  std::vector<FrameLatLongTime> result{};

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

  float previousLat = 0;
  float previousLong = 0;
  long frame = 0;
  std::string time = "";


  while (getline(is, row)) {
    std::regex_search(row, matchCoord, regexpCoord);
    std::regex_search(row, matchFrame, regexpFrame);
    std::regex_search(row, matchTime, regexpTime);


    if (!matchFrame.empty()) {
      frame = stol(matchFrame[1]);
    }

    if (!matchTime.empty()) {
      time = matchTime[0];
    }

    if (!matchCoord.empty()) {
      float lat = stof(matchCoord[1]);
      float lon = stof(matchCoord[2]);

      if (lat != previousLat || lon != previousLong) {
        previousLat = lat;
        previousLong = lon;
        FrameLatLongTime frameLatLongTime = FrameLatLongTime{frame, lat, lon, time};
        result.push_back(frameLatLongTime);
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


std::vector<FlltPair> match(std::vector<FrameLatLongTime> rgb, std::vector<FrameLatLongTime> thermal) {
  const std::chrono::milliseconds tolerance(31);
  
  std::vector<FlltPair> result{};

  for (long unsigned int indexT = 0; indexT < thermal.size(); indexT++) {
    for (long unsigned int indexRgb = 0; indexRgb < rgb.size(); indexRgb++) {
      if (abs((toMs(rgb.at(indexRgb).time) - toMs(thermal.at(indexT).time))) <= tolerance ) {
        result.push_back(FlltPair{rgb.at(indexRgb), thermal.at(indexT)});
      }
    }
  }
  return result;
}




// saves given frames 
void images(std::string path, bool rgb, bool thermal, std::vector<FlltPair> data, fs::path outputPath) {
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

  if (!fs::exists(outputPath)) {
    fs::create_directory(outputPath);

    if (rgb) {
      fs::create_directory(outputPath / "rgb");
    }
    if (thermal) {
      fs::create_directory(outputPath / "thermal");
    }
  }

  cv::Mat frame;

  for (long unsigned int i = 0; i < data.size(); i++) {

    fs::path lPath = fs::path(outputPath.string() + '/');

    fs::create_directory(lPath);


    if (rgb) {
      vCap.set(cv::CAP_PROP_POS_FRAMES, data.at(i).first.frame);
      vCap >> frame;
      cv::imwrite(lPath.string() + "rgb/" + std::to_string(i) + ".jpg", frame);

    }

    if (thermal) {
      tCap.set(cv::CAP_PROP_POS_FRAMES, data.at(i).second.frame);
      tCap >> frame;
      cv::imwrite(lPath.string() + "thermal/" + std::to_string(i) + ".jpg", frame);
    }
  }
}

