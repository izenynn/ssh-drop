#include "server_config.hpp"

#include <filesystem>
#include <stdexcept>

#include "config_parser.hpp"

namespace drop {

ServerConfig ServerConfig::load(int argc, char* argv[])
{
	std::filesystem::path path;

	if (argc >= 2) {
		path = argv[1];
	} else {
		path = "config/ssh-drop.conf";
	}

	if (!std::filesystem::exists(path)) {
		if (argc >= 2)
			throw std::runtime_error{"Config file not found: " + path.string()};
		return {};
	}

	auto map = ConfigParser::parse(path);
	return ServerConfig::from_map(map);
}

void ServerConfig::validate() const
{
	int count = (!secret_value.empty() ? 1 : 0)
	          + (!secret_file_path.empty() ? 1 : 0)
	          + (!secret_env_name.empty() ? 1 : 0);

	if (count > 1)
		throw std::runtime_error{
			"Specify exactly one of secret, secret_file, secret_env"};

	if (count == 0)
		throw std::runtime_error{
			"No secret source configured "
			"(set secret, secret_file, or secret_env)"};
}

ServerConfig ServerConfig::from_map(
	const std::unordered_map<std::string, std::string>& m)
{
	ServerConfig cfg;

	auto get = [&](const std::string& key) -> const std::string* {
		auto it = m.find(key);
		return it != m.end() ? &it->second : nullptr;
	};

	if (auto* v = get("port"))
		cfg.port = *v;
	if (auto* v = get("host_key"))
		cfg.host_key_path = *v;
	if (auto* v = get("authorized_keys"))
		cfg.authorized_keys_path = *v;
	if (auto* v = get("log_level"))
		cfg.log_level = *v;
	if (auto* v = get("log_file"))
		cfg.log_file = *v;
	if (auto* v = get("secret"))
		cfg.secret_value = *v;
	if (auto* v = get("secret_file"))
		cfg.secret_file_path = *v;
	if (auto* v = get("secret_env"))
		cfg.secret_env_name = *v;

	return cfg;
}

} // namespace drop
