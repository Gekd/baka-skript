#pragma once
#include <string>
#include "types.hpp"
#include "opencv2/opencv.hpp"
#include <curl/curl.h>
#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp" 


const std::string WILDNAV_QUERY_CSV_NAME = "photo_metadata.csv";
const std::string WILDNAV_MAP_CSV_NAME = "map.csv";
const std::string WILDNAV_RGB_QUERY_DIR_NAME = "query";
const std::string WILDNAV_THERMAL_QUERY_DIR_NAME = "query_thermal";
const std::string WILDNAV_MAP_DIR_NAME = "map";
const std::string WILDNAV_IMAGE_PREFIX = "drone_image_";

const std::string DEFAULT_WMS_URL = "https://kaart.maaamet.ee/wms/fotokaart";
const std::string DEFAULT_LAYER = "EESTIFOTO";
const int DEFAULT_MAX_TILE_SIZE = 4096;
const float DEFAULT_RESOLUTION_M = 0.12;

const int MAP_CACHE_GRID_METERS = 100;
const int JPEG_QUALITY = 90;
const int DIRECT_SEEK_THRESHOLD_FRAMES = 50; 
const int MAX_PARALLEL_DOWNLOADS = 4;

const std::string WILDNAV_QUERY_CSV_HEADER = "Filename,Latitude,Longitude,Altitude,Gimball_Roll,Gimball_Yaw,Gimball_Pitch,Flight_Roll,Flight_Yaw,Flight_Pitch\n";
const std::string WILDNAV_MAP_CSV_HEADER = "Filename,Top_left_lat,Top_left_lon,Bottom_right_lat,Bottom_right_lon\n";


using namespace NS_PROJ::crs;
using namespace NS_PROJ::io;
using namespace NS_PROJ::operation;
using namespace NS_PROJ::util;

class ProjTransformer {
  PJ_CONTEXT *ctx = nullptr;
public:
  PJ *transform = nullptr;

  ProjTransformer() {
    ctx = proj_context_create();

    PJ *tmp = proj_create_crs_to_crs(
        ctx,
        "EPSG:4326",
        "EPSG:3301",
        nullptr);

    if (!tmp) {
      throw std::runtime_error("Failed to create CRS transform");
    }

    transform = proj_normalize_for_visualization(ctx, tmp);
    proj_destroy(tmp);

    if (!transform) {
      proj_context_destroy(ctx);
      throw std::runtime_error("Failed to normalize CRS transform");
    }
  }

  ~ProjTransformer() {
    if (transform) {
      proj_destroy(transform);
    }

    if (ctx) {
      proj_context_destroy(ctx);
    }
  }
};

void minMaxLatLong(ProjTransformer &transformer, double centerLat, double centerLong, double &minX, double &minY, double &maxX, double &maxY);
void convertLatLong(ProjTransformer &transformer, double &lat, double &lon, double &x, double &y);
cv::Mat downloadOrthophoto(CURL *curl, const double &minX, const double &minY, const double &maxX, const double &maxY);
void wildnavOutput(const FileGroup &fileGroup, const std::vector<FrameDataPairCSV> &data, fs::path outputPath, bool rgbOutput, bool thermalOutput);