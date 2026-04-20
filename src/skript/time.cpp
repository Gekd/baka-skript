#include "time.hpp"

std::chrono::milliseconds csvTimeToMS(std::string time, int timezone) {
  int hours{};
  int minutes{};
  int seconds{};
  std::string msStr{};
  std::string ampm{};
  char colon{};
  char dot{};

  std::stringstream ss{time};

  ss >> hours >> colon >> minutes >> colon >> seconds >> dot >> msStr >> ampm;
  
  int milliseconds = stoi(msStr);
  if (msStr.size() == 2) {
    milliseconds *= 10;
  } else if (msStr.size() == 1) {
    milliseconds *= 100;
  } 

  if (ampm == "PM" && hours != 12) {
    hours += 12;
  }
  if (ampm == "AM" && hours == 12) {
    hours = 0;
  }
  hours += timezone;
  hours = (hours % 24 + 24) % 24;

  return std::chrono::milliseconds{hours*3600000 + minutes*60000 + seconds*1000 + milliseconds};
}