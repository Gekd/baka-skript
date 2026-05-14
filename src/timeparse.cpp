#include "timeparse.hpp"
#include <iostream>

std::chrono::milliseconds csvTimeToMS(std::string time, std::string date) {
  std::string msStr{};
  std::string ampm{};
  char dot{};

  std::tm tm = {};
  std::stringstream ss{date + " " + time};

  ss >> std::get_time(&tm, "%m/%d/%Y %I:%M:%S");
  ss >> dot >> msStr >> ampm;

  if (ampm == "PM" && tm.tm_hour != 12) {
    tm.tm_hour += 12;
  }
  if (ampm == "AM" && tm.tm_hour == 12) {
    tm.tm_hour = 0;
  }

  int milliseconds = stoi(msStr);
  if (msStr.size() == 2) {
    milliseconds *= 10;
  } else if (msStr.size() == 1) {
    milliseconds *= 100;
  } 

  time_t epoch = timegm(&tm);

  return std::chrono::milliseconds{epoch * 1000LL + milliseconds};
}

std::chrono::milliseconds toMs(std::string dateTime, int timezone) {
  if (dateTime.size() < 23) {
    return std::chrono::milliseconds(0);
  }
  
  std::tm tm = {};
  std::stringstream ss{dateTime};

  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  char dot{};
  std::string msStr{};
  ss >> dot >> msStr;


  int milliseconds = stoi(msStr);
  if (msStr.size() == 2) {
    milliseconds *= 10;
  } else if (msStr.size() == 1) {
    milliseconds *= 100;
  } 

  time_t epoch = timegm(&tm) - timezone * 3600;

  return std::chrono::milliseconds{epoch * 1000LL + milliseconds};  
}
