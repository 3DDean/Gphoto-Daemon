#pragma once
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

inline std::string get_time(std::string_view formatStr = "%Y-%m-%d %X")
{
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	// convert to string
	std::stringstream ss;
	ss << std::put_time(std::localtime(&t), formatStr.data());

	return ss.str();
}