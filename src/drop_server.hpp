#pragma once

#include <memory>

#include "authenticator.hpp"
#include "secret_provider.hpp"
#include "server_config.hpp"
#include "ssh_types.hpp"

namespace drop {

class DropServer {
public:
	DropServer(ServerConfig config,
	           std::unique_ptr<IAuthenticator> authenticator,
	           std::unique_ptr<ISecretProvider> secret_provider);

	void run();

private:
	void authenticate(SshSession& session);
	SshChannel open_channel(SshSession& session);
	void wait_for_shell(SshSession& session, SshChannel& channel);
	void deliver_secret(SshChannel& channel);

	ServerConfig config_;
	std::unique_ptr<IAuthenticator> authenticator_;
	std::unique_ptr<ISecretProvider> secret_provider_;
};

} // namespace drop
