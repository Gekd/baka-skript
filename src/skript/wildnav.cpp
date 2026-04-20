#include <fstream>
#include <curl/curl.h>
#include "wildnav.hpp"


/*
wildnav format(RGB only)(https://github.com/TIERS/wildnav/tree/main/assets):
assets/
  query/
    photo_metadata.csv(Filename, Latitude, Longitude, Altitude, Gimball_Roll,	Gimball_Yaw, Gimball_Pitch, Flight_Roll, Flight_Yaw, Flight_Pitch)
    rgb_1.jpg
    rgb_2.jpg
    ...
  map/
    map.csv(Filename, Top_left_lat, Top_left_lon, Bottom_right_lat, Bottom_right_long)
    sat_map_00.png





TODO:
- Testi kas wildnav töötab
- Command line parameters
*/

void minMaxLatLong(double centerLat, double centerLong, double &minX, double &minY, double &maxX, double &maxY) {
  
  double x{};
  double y{};
  convertLatLong(centerLat, centerLong, x, y);

  double halfSize = DEFAULT_MAX_TILE_SIZE * DEFAULT_RESOLUTION_M / 2;

  minX = x - halfSize;
  maxX = x + halfSize;
  minY = y - halfSize;
  maxY = y + halfSize;
}


// EPSG:4326 to EPSG:3301 conversion
// https://stackoverflow.com/questions/75300513/using-proj-c-api-for-crs-to-crs-transformations
void convertLatLong(double &lat, double &lon, double &x, double &y) {
  
  PJ_CONTEXT *ctx = proj_context_create();

  PJ *PCreate = proj_create_crs_to_crs(ctx, "EPSG:4326", "EPSG:3301", NULL);
  if (!PCreate) {
    std::cout << "Failed to create projection\n";
    return;
  }

  PJ *P = proj_normalize_for_visualization(ctx, PCreate);

  PJ_COORD input = proj_coord(lon, lat, 0, 0);
  PJ_COORD output = proj_trans(P, PJ_FWD, input);

  x = output.xy.x;
  y = output.xy.y;

  proj_destroy(PCreate);
  proj_destroy(P);
  proj_context_destroy(ctx);
}


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


// https://stackoverflow.com/questions/51317221/how-to-use-libcurl-in-c-to-send-a-post-request-and-receive-it
cv::Mat downloadOrthophoto(const double &minX, const double &minY, const double &maxX, const double &maxY) {
    
  std::string bbox = std::to_string(minY) + "," + std::to_string(minX) + "," + std::to_string(maxY) + "," + std::to_string(maxX);

  std::cout << std::fixed << std::setprecision(12) << "Downloading orthophoto for area: " << minY << "," << minX << " - " << maxY << "," << maxX << '\n';

  CURL *curl = curl_easy_init();
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
    curl_easy_cleanup(curl);
  }

  std::vector<uchar> data(readBuffer.begin(), readBuffer.end());
  cv::Mat img = cv::imdecode(data, cv::IMREAD_COLOR);

  if (img.empty()) {
      std::cerr << "WMS returned invalid image (likely CRS/BBOX error)\n";
      std::cerr << readBuffer.substr(0, 300) << "\n";
  }
  return img;
}



void wildnavOutput(FileGroup fileGroup, const std::vector<FrameDataPairCSV> &data, fs::path outputPath) {

  outputPath /= fileGroup.mp4VPath.stem();

  cv::VideoCapture vCap = cv::VideoCapture(fileGroup.mp4VPath);

  if(!vCap.isOpened()) {
    std::cout << "Can't open file: " << fileGroup.mp4VPath << '\n';
  }

  if (!fs::exists(outputPath)) {
    fs::create_directories(outputPath);
  }

  if (!fs::exists(outputPath / WILDNAV_QUERY_DIR_NAME)) {
    fs::create_directory(outputPath / WILDNAV_QUERY_DIR_NAME);
  }

  if (!fs::exists(outputPath / WILDNAV_MAP_DIR_NAME)) {
    fs::create_directory(outputPath / WILDNAV_MAP_DIR_NAME);
  }

  std::ofstream csvFileQuery;
  csvFileQuery.open(outputPath / "query" / WILDNAV_QUERY_CSV_NAME);
  csvFileQuery << std::fixed << std::setprecision(12);
  csvFileQuery << "Filename,Latitude,Longitude,Altitude,Gimball_Roll,Gimball_Yaw,Gimball_Pitch,Flight_Roll,Flight_Yaw,Flight_Pitch\n";


  std::ofstream csvFileMap;
  csvFileMap.open(outputPath / "map" / WILDNAV_MAP_CSV_NAME);
  csvFileMap << std::fixed << std::setprecision(12);
  csvFileMap << "Filename,Top_left_lat,Top_left_lon,Bottom_right_lat,Bottom_right_long\n";

  cv::Mat frame;

  for (long unsigned int i = 1; i <= 10; i++) {  //data.size()
    // If gimball is not pointing down
    if (data.at(i).second.gimbalPitch > -87.5) {
      continue;
    }


    vCap.set(cv::CAP_PROP_POS_FRAMES, data.at(i).first.first.frame);
    vCap >> frame;
    std::string filenameQuery = WILDNAV_IMAGE_PREFIX + std::to_string(i) + ".jpg";
    std::string filenameMap = "sat_map_" + std::to_string(i) + ".jpg";

    double minX{};
    double minY{};
    double maxX{};
    double maxY{};
    
    minMaxLatLong(data.at(i).first.first.lat, data.at(i).first.first.lon, minX, minY, maxX, maxY);
    std::cout << "RAW GPS: "
          << data.at(i).first.first.lat << ", "
          << data.at(i).first.first.lon << "\n";
    cv::imwrite(outputPath.string() + "/" + WILDNAV_QUERY_DIR_NAME + "/" + filenameQuery, frame);
    cv::imwrite(outputPath.string() + "/" + WILDNAV_MAP_DIR_NAME + "/" + filenameMap, downloadOrthophoto(minX, minY, maxX, maxY));
    csvFileQuery << filenameQuery << "," << data.at(i).second.latitude << "," << data.at(i).second.longitude << "," << data.at(i).first.first.rel_alt << "," << data.at(i).second.gimbalRoll << "," << data.at(i).second.gimbalYaw << "," << data.at(i).second.gimbalPitch << "," << data.at(i).second.roll << "," << data.at(i).second.yaw360 << "," << data.at(i).second.pitch << '\n';
    csvFileMap << filenameMap << "," << minX << "," << minY << "," << maxX << "," << maxY << '\n';
  }

  csvFileQuery.close();
  csvFileMap.close();
}