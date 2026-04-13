#include <iostream>
#include <filesystem>
#include "skript/file.hpp"

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cout << "This program needs to have at least one param for timezone: +2" << '\n';
    return 0;
  }
  
  fs::path path = fs::path("../data");
  std::vector<std::string> file = dir(path);

  std::vector<CSVRow> csvData = readCsv(fs::path(path / "DJI_202511241027_002/DJIFlightRecord_2025-11-24_[10-29-04].csv"));

  std::vector<FrameData> framesT = gpsUpdateFrames(fs::path(file[0] + "T.SRT"));
  std::cout << "T: " << framesT.size() << '\n';
  std::vector<FrameData> framesV = gpsUpdateFrames(fs::path(file[0] + "V.SRT"));
  std::cout << "V: " << framesV.size() << '\n';

  std::vector<FrameDataPair> matchedImages = matchImages(framesV, framesT);

  framesT.clear();
  framesV.clear();

  for (const auto& pair : matchedImages) {
    framesV.push_back(pair.first);
    framesT.push_back(pair.second);
  }

  std::vector<FrameDataPairCSV> data = matchPairsToCSV(matchedImages, csvData, std::stoi(argv[1]));
  std::cout << "Matched pairs: " << data.size() << '\n';
  images(file[0], true, true, data, "output");
}
