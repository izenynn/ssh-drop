#pragma once

#include <memory>

#include <libssh/libssh.h>

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
	static int on_auth_pubkey(ssh_session session, const char* user,
	                          ssh_key_struct* pubkey, char signature_state,
	                          void* userdata);
	static ssh_channel on_channel_open(ssh_session session, void* userdata);
	static int on_shell_request(ssh_session session, ssh_channel channel,
	                            void* userdata);

	ServerConfig config_;
	std::unique_ptr<IAuthenticator> authenticator_;
	std::unique_ptr<ISecretProvider> secret_provider_;

	// Transient state for a single run()
	ssh_channel raw_channel_ = nullptr;
	bool authenticated_ = false;
	bool got_shell_ = false;
};

} // namespace drop
