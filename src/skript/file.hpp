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

using FrameDataPair = std::pair<FrameData, FrameData>;


ordered_json dataEntry(auto name, FrameData data) {
    return {
        {"name", name},
        {"frame", data.frame},
        {"lat", data.lat},
        {"lon", data.lon},
        {"time", data.time},
        {"rel_alt", data.rel_alt},
        {"abs_alt", data.abs_alt}
    };
}

std::vector<std::string> dir(fs::path path);

std::vector<FrameDataPair> match(std::vector<FrameData> rgb, std::vector<FrameData> thermal);
std::chrono::milliseconds toMs(std::string time);

void images(std::string path, bool rgb, bool thermal, std::vector<FrameDataPair> data, fs::path outputPath);

std::vector<FrameData> gpsUpdateFrames(fs::path path);
void writeJson(fs::path path);