#pragma once

#include <string>

namespace drop {

struct ServerConfig {
	std::string port = "9393";
	std::string host_key_path = "key/id_ed25519";
	std::string authorized_keys_path = "key/authorized_keys";
};

} // namespace drop
