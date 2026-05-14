#include <fstream>
#include "wildnav.hpp"

void minMaxLatLong(ProjTransformer &transformer, double centerLat, double centerLong, double &minX, double &minY, double &maxX, double &maxY) {
  
  double x{};
  double y{};
  convertLatLong(transformer, centerLat, centerLong, x, y);

  double halfSize = DEFAULT_MAX_TILE_SIZE * DEFAULT_RESOLUTION_M / 2;

  minX = x - halfSize;
  maxX = x + halfSize;
  minY = y - halfSize;
  maxY = y + halfSize;
}


// EPSG:4326 to EPSG:3301 conversion
// https://stackoverflow.com/questions/75300513/using-proj-c-api-for-crs-to-crs-transformations
void convertLatLong(ProjTransformer &transformer, double &lat, double &lon, double &x, double &y) {
  
  
  PJ_COORD input = proj_coord(lon, lat, 0, 0);
  PJ_COORD output = proj_trans(transformer.transform, PJ_FWD, input);

  x = output.xy.x;
  y = output.xy.y;
}


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


// https://stackoverflow.com/questions/51317221/how-to-use-libcurl-in-c-to-send-a-post-request-and-receive-it
cv::Mat downloadOrthophoto(CURL *curl, const double &minX, const double &minY, const double &maxX, const double &maxY) {
    
  std::string bbox = std::to_string(minY) + "," + std::to_string(minX) + "," + std::to_string(maxY) + "," + std::to_string(maxX);

  std::string readBuffer;

  std::string url =
        DEFAULT_WMS_URL +
        "?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
        "&LAYERS=" + DEFAULT_LAYER +
        "&STYLES="
        "&FORMAT=image/jpeg"
        "&CRS=EPSG:3301"
        "&WIDTH=" + std::to_string(DEFAULT_MAX_TILE_SIZE) +
        "&HEIGHT=" + std::to_string(DEFAULT_MAX_TILE_SIZE) +
        "&BBOX=" + bbox;

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << '\n';
    }
  }

  std::vector<uchar> data(readBuffer.begin(), readBuffer.end());
  cv::Mat img = cv::imdecode(data, cv::IMREAD_COLOR);

  if (img.empty()) {
      std::cerr << "WMS returned invalid image (likely CRS/BBOX error)\n";
      std::cerr << readBuffer.substr(0, 300) << "\n";
  }
  return img;
}


void snapToGridStep(double &value) {
  value = std::llround(value / MAP_CACHE_GRID_METERS) * MAP_CACHE_GRID_METERS;
}


void wildnavOutput(const FileGroup &fileGroup, const std::vector<FrameDataPairCSV> &data, fs::path outputPath, bool rgbOutput, bool thermalOutput) {
  std::string fileGroupName = fileGroup.mp4VPath.stem().string();
  fileGroupName = fileGroup.parentDirName + "/" + fileGroupName.substr(0, fileGroupName.size() - 2);
  
  if (data.size() == 0) {
    std::cout << "No data to process for " << fileGroupName << "\n";  
    return;
  }

  outputPath /= fileGroupName;

  cv::VideoCapture rgbCap{};
  cv::VideoCapture thermalCap{};

  if (rgbOutput) {
    rgbCap = cv::VideoCapture(fileGroup.mp4VPath);
    if(!rgbCap.isOpened()) {
      std::cout << "Can't open file: " << fileGroup.mp4VPath << '\n';
      return;
    }
  }

  if (thermalOutput) {
    thermalCap = cv::VideoCapture(fileGroup.mp4TPath);
    if(!thermalCap.isOpened()) {
      std::cout << "Can't open file: " << fileGroup.mp4TPath << '\n';
      return;
    }
  }

  if (!fs::exists(outputPath)) {
    fs::create_directories(outputPath);
  }

  if (!fs::exists(outputPath / WILDNAV_RGB_QUERY_DIR_NAME) && rgbOutput) {
    fs::create_directory(outputPath / WILDNAV_RGB_QUERY_DIR_NAME);
  }

  if (!fs::exists(outputPath / WILDNAV_THERMAL_QUERY_DIR_NAME) && thermalOutput) {
    fs::create_directory(outputPath / WILDNAV_THERMAL_QUERY_DIR_NAME);
  }

  if (!fs::exists(outputPath / WILDNAV_MAP_DIR_NAME)) {
    fs::create_directory(outputPath / WILDNAV_MAP_DIR_NAME);
  }

  fs::path mapCacheDir = fs::path(outputPath / MAP_CACHE_DIR_NAME);
  if (!fs::exists(mapCacheDir)) {
    fs::create_directories(mapCacheDir);
  }

  std::ofstream csvRgbFileQuery{};
  if (rgbOutput) {
    csvRgbFileQuery.open(outputPath / WILDNAV_RGB_QUERY_DIR_NAME / WILDNAV_QUERY_CSV_NAME);
    csvRgbFileQuery << std::fixed << std::setprecision(12);
    csvRgbFileQuery << WILDNAV_QUERY_CSV_HEADER;
  }

  std::ofstream csvThermalFileQuery{};
  if (thermalOutput) {
    csvThermalFileQuery.open(outputPath / WILDNAV_THERMAL_QUERY_DIR_NAME / WILDNAV_QUERY_CSV_NAME);
    csvThermalFileQuery << std::fixed << std::setprecision(12);
    csvThermalFileQuery << WILDNAV_QUERY_CSV_HEADER;
  }

  std::ofstream csvFileMap;
  csvFileMap.open(outputPath / WILDNAV_MAP_DIR_NAME / WILDNAV_MAP_CSV_NAME);
  csvFileMap << std::fixed << std::setprecision(12);
  csvFileMap << WILDNAV_MAP_CSV_HEADER;

  cv::Mat rgbFrame{};
  int currentRgbFrame = 0;

  cv::Mat thermalFrame{};
  int currentThermalFrame = 0;

  ProjTransformer transformer;
  curl_global_init(CURL_GLOBAL_DEFAULT);

  CURL *curl = curl_easy_init();

  for (size_t i = 0; i < data.size(); i++) {

    const auto item = data[i];

    // If gimball is not pointing down
    if (item.second.gimbalPitch > -87.5) {
      continue;
    }

    if (rgbOutput) {
      int targetRgbFrame = item.first.first.frame;
      int frameJump = targetRgbFrame - currentRgbFrame;
      bool frameOk = true;
      if (frameJump > DIRECT_SEEK_THRESHOLD_FRAMES) {
        rgbCap.set(cv::CAP_PROP_POS_FRAMES, targetRgbFrame);
        frameOk = rgbCap.read(rgbFrame);
        currentRgbFrame = targetRgbFrame + 1;
      } else {
        while (currentRgbFrame < targetRgbFrame) {
          if (!rgbCap.grab()) { 
            frameOk = false; 
            break; 
          }
          currentRgbFrame++;
        }
        if (frameOk) {
          frameOk = rgbCap.retrieve(rgbFrame);
        }
      }
    
      if (!frameOk || rgbFrame.empty()) {
        std::cout << "Failed to retrieve/read rgb frame " << targetRgbFrame << " from " << fileGroup.mp4VPath << '\n';
        continue;
      }
    }

    if (thermalOutput) {
      int targetThermalFrame = item.first.second.frame;
      int thermalFrameJump = targetThermalFrame - currentThermalFrame;
      bool thermalFrameOk = true;

      if (thermalFrameJump > DIRECT_SEEK_THRESHOLD_FRAMES) {
        thermalCap.set(cv::CAP_PROP_POS_FRAMES, targetThermalFrame);
        thermalFrameOk = thermalCap.read(thermalFrame);
        currentThermalFrame = targetThermalFrame + 1;
      } else {
        while (currentThermalFrame < targetThermalFrame) {
          if (!thermalCap.grab()) { 
            thermalFrameOk = false; 
            break; 
          }
          currentThermalFrame++;
        }
        if (thermalFrameOk) {
          thermalFrameOk = thermalCap.retrieve(thermalFrame);
        }
      }

      if (!thermalFrameOk || thermalFrame.empty()) {
        std::cout << "Failed to retrieve/read thermal frame " << targetThermalFrame << " from " << fileGroup.mp4TPath << '\n';
        continue;
      }
    }

    std::string filenameQuery = WILDNAV_IMAGE_PREFIX + std::to_string(i+1) + ".jpg";
    std::string filenameMap = "sat_map_" + std::to_string(i+1) + ".jpg";

    double minX{};
    double minY{};
    double maxX{};
    double maxY{};
    double halfSize = DEFAULT_MAX_TILE_SIZE * DEFAULT_RESOLUTION_M / 2.0;
    
    minMaxLatLong(transformer, item.first.first.lat, item.first.first.lon, minX, minY, maxX, maxY);

    double centerX = (minX + maxX) / 2.0;
    double centerY = (minY + maxY) / 2.0;
    
    snapToGridStep(centerX);
    snapToGridStep(centerY);

    minX = centerX - halfSize;
    maxX = centerX + halfSize;
    minY = centerY - halfSize;
    maxY = centerY + halfSize;

    std::string cacheKey = std::to_string(centerX) + "_" + std::to_string(centerY);
    fs::path cachedImagePath = fs::path(outputPath / MAP_CACHE_DIR_NAME / (cacheKey + ".jpg"));
    bool isCached = false;

    if (fs::exists(cachedImagePath)) {
      isCached = true;
    }

    if (!isCached) {
      cv::Mat orthoPhoto = downloadOrthophoto(curl, minX, minY, maxX, maxY);
      cv::imwrite(cachedImagePath.string(), orthoPhoto);
    }

    if (rgbOutput) {
      cv::imwrite(outputPath.string() + "/" + WILDNAV_RGB_QUERY_DIR_NAME + "/" + filenameQuery, rgbFrame);
      csvRgbFileQuery << filenameQuery << "," << item.second.latitude << "," << item.second.longitude << "," << item.first.first.rel_alt << "," << item.second.gimbalRoll << "," << item.second.gimbalYaw << "," << item.second.gimbalPitch << "," << item.second.roll << "," << item.second.yaw360 << "," << item.second.pitch << '\n';
    }

    if (thermalOutput) {
      cv::imwrite(outputPath.string() + "/" + WILDNAV_THERMAL_QUERY_DIR_NAME + "/" + filenameQuery, thermalFrame);
      csvThermalFileQuery << filenameQuery << "," << item.second.latitude << "," << item.second.longitude << "," << item.first.second.rel_alt << "," << item.second.gimbalRoll << "," << item.second.gimbalYaw << "," << item.second.gimbalPitch << "," << item.second.roll << "," << item.second.yaw360 << "," << item.second.pitch << '\n';
    }
    cv::imwrite(outputPath.string() + "/" + WILDNAV_MAP_DIR_NAME + "/" + filenameMap, cv::imread(cachedImagePath.string()));
    csvFileMap << filenameMap << "," << minX << "," << minY << "," << maxX << "," << maxY << '\n';
  }

  csvRgbFileQuery.close();
  csvFileMap.close();
  csvThermalFileQuery.close();
}