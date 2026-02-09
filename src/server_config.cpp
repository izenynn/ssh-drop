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

	if (!std::filesystem::exists(path))
		throw std::runtime_error{"Config file not found: "
					 + path.string()};

	auto map = ConfigParser::parse(path);
	return ServerConfig::from_map(map);
}

void ServerConfig::validate() const
{
	if (port.empty())
		throw std::runtime_error{"port is required"};
	const int port_num = std::stoi(port);
	if (port_num < 1 || port_num > 65535)
		throw std::runtime_error{"Port out of range: " + port};

	if (host_key_path.empty())
		throw std::runtime_error{"host_key is required"};
	if (!std::filesystem::exists(host_key_path))
		throw std::runtime_error{"Host key not found: "
					 + host_key_path};

	if (auth_method.empty())
		throw std::runtime_error{"auth_method is required"};
	if (auth_method != "publickey" && auth_method != "password"
	    && auth_method != "both")
		throw std::runtime_error{
				"auth_method must be publickey, password, or both"};

	if (auth_method == "publickey" || auth_method == "both") {
		if (authorized_keys_path.empty())
			throw std::runtime_error{
					"authorized_keys is required when "
					"auth_method is publickey or both"};
		if (!std::filesystem::exists(authorized_keys_path))
			throw std::runtime_error{
					"Authorized keys not found: "
					+ authorized_keys_path};
	}

	if (auth_timeout < 1)
		throw std::runtime_error{"auth_timeout must be >= 1"};

	// Secret source: exactly one must be set
	const int secret_count = (secret.has_value() ? 1 : 0)
				 + (secret_file.has_value() ? 1 : 0)
				 + (secret_env.has_value() ? 1 : 0);
	if (secret_count > 1)
		throw std::runtime_error{
				"Specify exactly one of secret, secret_file, secret_env"};
	if (secret_count == 0)
		throw std::runtime_error{
				"No secret source configured "
				"(set secret, secret_file, or secret_env)"};

	if (secret_file.has_value()
	    && !std::filesystem::exists(*secret_file))
		throw std::runtime_error{"Secret file not found: "
					 + *secret_file};
	if (secret_env.has_value()
	    && std::getenv(secret_env->c_str()) == nullptr)
		throw std::runtime_error{"Secret env var not set: "
					 + *secret_env};

	// Password source: required for password/both modes
	const int pw_count = (auth_password.has_value() ? 1 : 0)
			     + (auth_password_file.has_value() ? 1 : 0)
			     + (auth_password_env.has_value() ? 1 : 0);
	if (auth_method == "password" || auth_method == "both") {
		if (pw_count != 1)
			throw std::runtime_error{
					"Specify exactly one of auth_password, "
					"auth_password_file, auth_password_env"};
	} else if (pw_count > 0) {
		throw std::runtime_error{
				"auth_password* keys are only valid when "
				"auth_method is password or both"};
	}

	if (auth_password_file.has_value()
	    && !std::filesystem::exists(*auth_password_file))
		throw std::runtime_error{"Password file not found: "
					 + *auth_password_file};
	if (auth_password_env.has_value()
	    && std::getenv(auth_password_env->c_str()) == nullptr)
		throw std::runtime_error{"Password env var not set: "
					 + *auth_password_env};

	// User source: at most one
	const int user_count = (auth_user.has_value() ? 1 : 0)
			       + (auth_user_file.has_value() ? 1 : 0)
			       + (auth_user_env.has_value() ? 1 : 0);
	if (user_count > 1)
		throw std::runtime_error{
				"Specify at most one of auth_user, "
				"auth_user_file, auth_user_env"};

	if (auth_user_file.has_value()
	    && !std::filesystem::exists(*auth_user_file))
		throw std::runtime_error{"User file not found: "
					 + *auth_user_file};
	if (auth_user_env.has_value()
	    && std::getenv(auth_user_env->c_str()) == nullptr)
		throw std::runtime_error{"User env var not set: "
					 + *auth_user_env};
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

	if (auto* v = get("secret"))
		cfg.secret = *v;
	if (auto* v = get("secret_file"))
		cfg.secret_file = *v;
	if (auto* v = get("secret_env"))
		cfg.secret_env = *v;

	if (auto* v = get("auth_method"))
		cfg.auth_method = *v;

	if (auto* v = get("auth_user"))
		cfg.auth_user = *v;
	if (auto* v = get("auth_user_file"))
		cfg.auth_user_file = *v;
	if (auto* v = get("auth_user_env"))
		cfg.auth_user_env = *v;

	if (auto* v = get("auth_password"))
		cfg.auth_password = *v;
	if (auto* v = get("auth_password_file"))
		cfg.auth_password_file = *v;
	if (auto* v = get("auth_password_env"))
		cfg.auth_password_env = *v;

	return cfg;
}

} // namespace drop
