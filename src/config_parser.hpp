#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace drop {

class ConfigParser {
public:
	static std::unordered_map<std::string, std::string>
	parse(const std::filesystem::path& path);
};

} // namespace drop
