#pragma once
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;
struct FrameLatLongTime {
    long frame;
    float lat;
    float lon;
    std::string time;
};
using FlltPair = std::pair<FrameLatLongTime, FrameLatLongTime>;


std::vector<std::string> dir(fs::path path);

std::vector<FlltPair> match(std::vector<FrameLatLongTime> rgb, std::vector<FrameLatLongTime> thermal);
std::chrono::milliseconds toMs(std::string time);

void images(std::string path, bool rgb, bool thermal, std::vector<FlltPair> data, fs::path outputPath);

std::vector<FrameLatLongTime> gpsUpdateFrames(fs::path path);
