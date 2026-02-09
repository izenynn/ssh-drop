#ifndef SSH_DROP_CONFIG_PARSER_H_
#define SSH_DROP_CONFIG_PARSER_H_

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

#endif // SSH_DROP_CONFIG_PARSER_H_
