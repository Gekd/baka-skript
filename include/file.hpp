#pragma once
#include <vector>
#include "types.hpp"

std::vector<FrameData> gpsUpdateFrames(const fs::path &path);

std::vector<FrameDataPair> matchImages(const std::vector<FrameData> &rgb, const std::vector<FrameData> &thermal);

bool compare(FrameData &frameData, CSVRow &csvRow);

std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows, int timezone);

void upsertJson(ordered_json &json, const ordered_json &newEntry);

void images(FileGroup fileGroup, bool rgb, bool thermal, const std::vector<FrameDataPairCSV> &data, fs::path outputPath, long startingIndex);

void output(std::vector<CSVRow> csv, std::vector<FileGroup> fileGroups, fs::path outputPath, int timezone);

double haversine(double lat1, double lon1, double lat2, double lon2);