#include "log.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace drop::log {

namespace {

Level	      g_min_level = Level::info;
std::ofstream g_log_file;
std::mutex    g_mutex;

const char* level_tag(Level lv)
{
	switch (lv) {
	case Level::debug:
		return "DEBUG";
	case Level::info:
		return "INFO";
	case Level::warn:
		return "WARN";
	case Level::error:
		return "ERROR";
	}
	return "?";
}

void write_to(std::ostream& os, Level lv, std::string_view msg)
{
	auto	now  = std::chrono::system_clock::now();
	auto	time = std::chrono::system_clock::to_time_t(now);
	std::tm tm_buf{};
#ifdef _WIN32
	localtime_s(&tm_buf, &time);
#else
	localtime_r(&time, &tm_buf);
#endif
	os << '[' << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] ["
	   << level_tag(lv) << "] " << msg << '\n';
}

void log_impl(Level lv, std::string_view msg)
{
	if (lv < g_min_level)
		return;

	std::lock_guard lock{g_mutex};

	std::ostream& console = (lv == Level::error) ? std::cerr : std::cout;
	write_to(console, lv, msg);

	if (g_log_file.is_open()) {
		write_to(g_log_file, lv, msg);
		g_log_file.flush();
	}
}

} // namespace

void init(Level min_level, const std::string& file_path)
{
	g_min_level = min_level;

	if (!file_path.empty()) {
		g_log_file.open(file_path, std::ios::app);
		if (!g_log_file.is_open())
			throw std::runtime_error{"Could not open log file: "
						 + file_path};
	}
}

void debug(std::string_view msg)
{
	log_impl(Level::debug, msg);
}
void info(std::string_view msg)
{
	log_impl(Level::info, msg);
}
void warn(std::string_view msg)
{
	log_impl(Level::warn, msg);
}
void error(std::string_view msg)
{
	log_impl(Level::error, msg);
}

Level level_from_string(std::string_view str)
{
	if (str == "debug")
		return Level::debug;
	if (str == "info")
		return Level::info;
	if (str == "warn")
		return Level::warn;
	if (str == "error")
		return Level::error;
	throw std::runtime_error{"Unknown log level: " + std::string{str}
				 + " (expected: debug, info, warn, error)"};
}

} // namespace drop::log
