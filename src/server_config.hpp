#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace drop {

struct ServerConfig {
	std::string port;
	std::string host_key_path;
	std::string authorized_keys_path;

	int auth_timeout = 30;

	std::string log_level = "info";
	std::string log_file;

	std::optional<std::string> secret;
	std::optional<std::string> secret_file;
	std::optional<std::string> secret_env;
	bool			   secret_encrypted = false;

	std::string auth_method;

	std::optional<std::string> auth_user;
	std::optional<std::string> auth_user_file;
	std::optional<std::string> auth_user_env;

	std::optional<std::string> auth_password;
	std::optional<std::string> auth_password_file;
	std::optional<std::string> auth_password_env;

	static ServerConfig load(int argc, char* argv[]);
	void		    validate() const;

	static ServerConfig
	from_map(const std::unordered_map<std::string, std::string>& m);
};

} // namespace drop
