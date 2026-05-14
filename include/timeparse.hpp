#pragma once

#include <chrono>

std::chrono::milliseconds toMs(std::string dateTime, int timezone);

std::chrono::milliseconds csvTimeToMS(std::string time, std::string date);