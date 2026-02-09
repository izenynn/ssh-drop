#include "config_parser.hpp"

#include <fstream>
#include <stdexcept>

namespace drop {

namespace {

std::string trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return {};
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

} // namespace

std::unordered_map<std::string, std::string>
ConfigParser::parse(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error{"Could not open config file: "
					 + path.string()};

	std::unordered_map<std::string, std::string> result;
	std::string				     line;
	int					     line_num = 0;

	while (std::getline(file, line)) {
		++line_num;
		std::string trimmed = trim(line);

		if (trimmed.empty() || trimmed[0] == '#')
			continue;

		auto eq = trimmed.find('=');
		if (eq == std::string::npos)
			throw std::runtime_error{path.string() + ":"
						 + std::to_string(line_num)
						 + ": expected 'key = value'"};

		std::string key	  = trim(trimmed.substr(0, eq));
		std::string value = trim(trimmed.substr(eq + 1));

		if (key.empty())
			throw std::runtime_error{path.string() + ":"
						 + std::to_string(line_num)
						 + ": empty key"};

		result[key] = value;
	}

	return result;
}

} // namespace drop
