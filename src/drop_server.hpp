#pragma once

#include <atomic>
#include <memory>

#include "authenticator.hpp"
#include "secret_provider.hpp"
#include "server_config.hpp"

namespace drop {

class DropServer {
public:
	DropServer(ServerConfig config,
	           std::unique_ptr<IAuthenticator> authenticator,
	           std::unique_ptr<ISecretProvider> secret_provider);

	void run(std::atomic<bool>& running);

private:
	ServerConfig config_;
	std::unique_ptr<IAuthenticator> authenticator_;
	std::unique_ptr<ISecretProvider> secret_provider_;
};

} // namespace drop
