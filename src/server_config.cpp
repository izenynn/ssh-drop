#include "server_config.hpp"

#include <cstdlib>
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
			throw std::runtime_error{"Config file not found: "
						 + path.string()};
		return {};
	}

	auto map = ConfigParser::parse(path);
	return ServerConfig::from_map(map);
}

void ServerConfig::validate() const
{
	const int port_num = std::stoi(port);
	if (port_num < 1 || port_num > 65535)
		throw std::runtime_error{"Port out of range: " + port};

	if (!std::filesystem::exists(host_key_path))
		throw std::runtime_error{"Host key not found: "
					 + host_key_path};

	if (!std::filesystem::exists(authorized_keys_path))
		throw std::runtime_error{"Authorized keys not found: "
					 + authorized_keys_path};

	if (auth_timeout < 1)
		throw std::runtime_error{"auth_timeout must be >= 1"};

	const int count = (!secret_value.empty() ? 1 : 0)
			  + (!secret_file_path.empty() ? 1 : 0)
			  + (!secret_env_name.empty() ? 1 : 0);
	if (count > 1)
		throw std::runtime_error{
				"Specify exactly one of secret, secret_file, secret_env"};
	if (count == 0)
		throw std::runtime_error{
				"No secret source configured "
				"(set secret, secret_file, or secret_env)"};

	if (!secret_file_path.empty()
	    && !std::filesystem::exists(secret_file_path))
		throw std::runtime_error{"Secret file not found: "
					 + secret_file_path};
	if (!secret_env_name.empty()
	    && std::getenv(secret_env_name.c_str()) == nullptr)
		throw std::runtime_error{"Secret env var not set: "
					 + secret_env_name};
}

ServerConfig
ServerConfig::from_map(const std::unordered_map<std::string, std::string>& m)
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
	if (auto* v = get("auth_timeout"))
		cfg.auth_timeout = std::stoi(*v);
	if (auto* v = get("log_level"))
		cfg.log_level = *v;
	if (auto* v = get("log_file"))
		cfg.log_file = *v;
	if (m.contains("secret") || m.contains("secret_file")
	    || m.contains("secret_env")) {
		cfg.secret_value     = {};
		cfg.secret_file_path = {};
		cfg.secret_env_name  = {};
	}
	if (auto* v = get("secret"))
		cfg.secret_value = *v;
	if (auto* v = get("secret_file"))
		cfg.secret_file_path = *v;
	if (auto* v = get("secret_env"))
		cfg.secret_env_name = *v;

	return cfg;
}

} // namespace drop
