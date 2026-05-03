#pragma once
#include <string>
#include "types.hpp"
#include "opencv2/opencv.hpp"

#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp" 


const std::string WILDNAV_QUERY_CSV_NAME = "photo_metadata.csv";
const std::string WILDNAV_MAP_CSV_NAME = "map.csv";
const std::string WILDNAV_QUERY_DIR_NAME = "query";
const std::string WILDNAV_MAP_DIR_NAME = "map";
const std::string WILDNAV_IMAGE_PREFIX = "drone_image_";

const std::string DEFAULT_WMS_URL = "https://kaart.maaamet.ee/wms/fotokaart";
const std::string DEFAULT_LAYER = "EESTIFOTO";
const int DEFAULT_MAX_TILE_SIZE = 4096;
const float DEFAULT_RESOLUTION_M = 0.10;


using namespace NS_PROJ::crs;
using namespace NS_PROJ::io;
using namespace NS_PROJ::operation;
using namespace NS_PROJ::util;

void minMaxLatLong(double centerLat, double centerLong, double &minX, double &minY, double &maxX, double &maxY);
void convertLatLong(double &lat, double &lon, double &x, double &y);
cv::Mat downloadOrthophoto(const double &minX, const double &minY, const double &maxX, const double &maxY);
void wildnavOutput(FileGroup fileGroup, const std::vector<FrameDataPairCSV> &data, fs::path outputPath);