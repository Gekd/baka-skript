#pragma once
#include <vector>
#include "types.hpp"

const double MAX_FRAME_CSVROW_DIFFERENCE_METERS{6.5};
const std::chrono::milliseconds MAX_FRAME_CSVROW_DIFFERENCE_MS{2000};
const std::chrono::milliseconds VIDEO_FRAME_TOLERANCE_MS{67};


std::vector<FrameData> extractGpsFrames(const fs::path &path, int timezone);

std::vector<FrameDataPair> matchImages(const std::vector<FrameData> &rgb, const std::vector<FrameData> &thermal);

std::vector<FrameDataPairCSV> matchPairsToCSV(const std::vector<FrameDataPair> &frameDataPairs, const std::vector<CSVRow> &csvRows);

void upsertJson(ordered_json &json, const ordered_json &newEntry);

void output(const std::vector<CSVRow> &csv, const FileGroup &fileGroup, const fs::path outputPath, int timezone, bool rgbOutput, bool thermalOutput);

double haversine(double lat1, double lon1, double lat2, double lon2);