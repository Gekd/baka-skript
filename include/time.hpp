#pragma once

#include <chrono>

std::chrono::milliseconds toMs(std::string time);

std::chrono::milliseconds csvTimeToMS(std::string time, int timezone);
