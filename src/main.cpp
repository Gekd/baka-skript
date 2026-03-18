#include <iostream>
#include <filesystem>
#include "skript/file.hpp"



int main(int argc, char** argv)
{
  /*
  if (argc < 2) {
    std::cout << "This program needs to have at least one param: 0 - RGB, 1 - thermal, 2 - both" << '\n';
    return 0;
  }
  */ 
  fs::path path = fs::path("../data");
  std::vector<std::string> file = dir(path);

  std::vector<FrameData> framesT = gpsUpdateFrames(fs::path(file[0] + "T.SRT"));
  std::cout << "T: " << framesT.size() << '\n';
  std::vector<FrameData> framesV = gpsUpdateFrames(fs::path(file[0] + "V.SRT"));
  std::cout << "V: " << framesV.size() << '\n';
  std::vector<FrameDataPair> data = match(framesV, framesT);
  std::cout << "Both: " << data.size() << '\n';

  images(file[0], true, true, data, "output");
}
