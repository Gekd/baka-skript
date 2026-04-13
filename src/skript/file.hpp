#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>


namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

struct FrameData {
  long frame;
  float lat;
  float lon;
  std::string time;
  float rel_alt;
  float abs_alt;
};

struct CSVRow {
  std::string updateTime;
  std::string flyTime;
  float flyTimeS;
  double latitude;
  double longitude;
  int heightFT;
  float altitudeFT;
  float pitch;
  float roll;
  float yaw;
  float yaw360;
  float directionOfTravel;
  std::string gimbalMode;
  float gimbalPitch;
  float gimbalRoll;
  float gimbalYaw;
  float gimbalYaw360;
  std::string cameraIsVideo;
};

const std::string csvCellValues[]{"CUSTOM.updateTime [local]", "OSD.flyTime", "OSD.flyTime [s]", "OSD.latitude", "OSD.longitude", "OSD.height [ft]", "OSD.altitude [ft]", "OSD.pitch", "OSD.roll", "OSD.yaw", "OSD.yaw [360]", "OSD.directionOfTravel", "GIMBAL.mode", "GIMBAL.pitch", "GIMBAL.roll", "GIMBAL.yaw", "GIMBAL.yaw [360]", "CAMERA.isVideo"};

using FrameDataPair = std::pair<FrameData, FrameData>;
using FrameDataPairCSV = std::pair<FrameDataPair, CSVRow>;
//using FrameMetadataPair = std::pair<FrameCSVDataPair, FrameCSVDataPair>;

ordered_json dataEntry(auto name, FrameData frame, CSVRow row) {
  return {
    {"name", name},
    {"frame", frame.frame},
    {"lat", frame.lat},
    {"lon", frame.lon},
    {"time", frame.time},
    {"rel_alt", frame.rel_alt},
    {"abs_alt", frame.abs_alt},
    {"updateTime", row.updateTime},
    {"flyTime", row.flyTime},
    {"flyTimeS", row.flyTimeS},
    {"latitude", row.latitude},
    {"longitude", row.longitude},
    {"heightFT", row.heightFT},
    {"altitudeFT", row.altitudeFT},
    {"pitch", row.pitch},
    {"roll", row.roll},
    {"yaw", row.yaw},
    {"yaw360", row.yaw360},
    {"directionOfTravel", row.directionOfTravel},
    {"gimbalMode", row.gimbalMode},
    {"gimbalPitch", row.gimbalPitch},
    {"gimbalRoll", row.gimbalRoll},
    {"gimbalYaw", row.gimbalYaw},
    {"gimbalYaw360", row.gimbalYaw360},
    {"cameraIsVideo", row.cameraIsVideo}
  };
}

std::vector<std::string> dir(const fs::path &path);
std::vector<FrameData> gpsUpdateFrames(const fs::path &path);
std::chrono::milliseconds toMs(std::string time);
std::vector<FrameDataPair> matchImages(const std::vector<FrameData> &rgb, const std::vector<FrameData> &thermal);
std::chrono::milliseconds csvTimeToMS(std::string time, int timezone);
bool compare(FrameData &frameData, CSVRow &csvRow, int timezone);
std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows, int timezone);
void images(std::string path, bool rgb, bool thermal, const std::vector<FrameDataPairCSV> &data, fs::path outputPath);
std::vector<std::string> rowSplit(std::string row);
std::map<std::string, int> columnMap(std::string row);
std::vector<CSVRow> readCsv(fs::path path);