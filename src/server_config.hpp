#pragma once

#include <string>
#include <unordered_map>

namespace drop {

struct ServerConfig {
	std::string port		 = "7022";
	std::string host_key_path	 = "key/id_ed25519";
	std::string authorized_keys_path = "key/authorized_keys";

	std::string log_level = "info";
	std::string log_file;

	std::string secret_value;
	std::string secret_file_path;
	std::string secret_env_name;

	static ServerConfig load(int argc, char* argv[]);
	void		    validate() const;

	static ServerConfig
	from_map(const std::unordered_map<std::string, std::string>& m);
};

} // namespace drop
