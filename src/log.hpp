#pragma once

#include <string>
#include <string_view>

namespace drop::log {

enum class Level {
	debug,
	info,
	warn,
	error
};

void  init(Level min_level, const std::string& file_path = "");
void  debug(std::string_view msg);
void  info(std::string_view msg);
void  warn(std::string_view msg);
void  error(std::string_view msg);
Level level_from_string(std::string_view str);

} // namespace drop::log
