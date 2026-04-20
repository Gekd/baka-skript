#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

struct FrameData {
  long frame;
  double lat;
  double lon;
  std::string time;
  long timeMs = -1;
  float rel_alt;
  float abs_alt;
};

struct CSVRow {
  std::string updateTime;
  long updateTimeMs = -1;
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
  bool gimbalPitchDown = false;
  float gimbalRoll;
  float gimbalYaw;
  float gimbalYaw360;
  std::string cameraIsVideo;
};

struct CSVRowIndex {
  int updateTime = -1;
  int flyTime = -1;
  int flyTimeS = -1;
  int latitude = -1;
  int longitude = -1;
  int heightFT = -1;
  int altitudeFT = -1;
  int pitch = -1;
  int roll = -1;
  int yaw = -1;
  int yaw360 = -1;
  int directionOfTravel = -1;
  int gimbalMode = -1;
  int gimbalPitch = -1;
  int gimbalRoll = -1;
  int gimbalYaw = -1;
  int gimbalYaw360 = -1;
  int cameraIsVideo = -1;
};

const std::string csvCellValues[]{"CUSTOM.updateTime [local]", "OSD.flyTime", "OSD.flyTime [s]", "OSD.latitude", "OSD.longitude", "OSD.height [ft]", "OSD.altitude [ft]", "OSD.pitch", "OSD.roll", "OSD.yaw", "OSD.yaw [360]", "OSD.directionOfTravel", "GIMBAL.mode", "GIMBAL.pitch", "GIMBAL.roll", "GIMBAL.yaw", "GIMBAL.yaw [360]", "CAMERA.isVideo"};


const std::unordered_map<std::string, int CSVRowIndex::*> csvLookup {
  {"CUSTOM.updateTime [local]", &CSVRowIndex::updateTime},
  {"OSD.flyTime", &CSVRowIndex::flyTime},
  {"OSD.flyTime [s]", &CSVRowIndex::flyTimeS},
  {"OSD.latitude", &CSVRowIndex::latitude},
  {"OSD.longitude", &CSVRowIndex::longitude},
  {"OSD.height [ft]", &CSVRowIndex::heightFT},
  {"OSD.altitude [ft]", &CSVRowIndex::altitudeFT},
  {"OSD.pitch", &CSVRowIndex::pitch},
  {"OSD.roll", &CSVRowIndex::roll},
  {"OSD.yaw", &CSVRowIndex::yaw},
  {"OSD.yaw [360]", &CSVRowIndex::yaw360},
  {"OSD.directionOfTravel", &CSVRowIndex::directionOfTravel},
  {"GIMBAL.mode", &CSVRowIndex::gimbalMode},
  {"GIMBAL.pitch", &CSVRowIndex::gimbalPitch},
  {"GIMBAL.roll", &CSVRowIndex::gimbalRoll},
  {"GIMBAL.yaw", &CSVRowIndex::gimbalYaw},
  {"GIMBAL.yaw [360]", &CSVRowIndex::gimbalYaw360},
  {"CAMERA.isVideo", &CSVRowIndex::cameraIsVideo}
};


using FrameDataPair = std::pair<FrameData, FrameData>;
using FrameDataPairCSV = std::pair<FrameDataPair, CSVRow>;

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


struct FileGroup {
  fs::path mp4TPath;
  fs::path mp4VPath;
  fs::path srtTPath;
  fs::path srtVPath;
};

struct DirInfo {
  fs::path path;
  std::string name;
  fs::path csvPath;
  std::vector<FileGroup> fileGroups;
};