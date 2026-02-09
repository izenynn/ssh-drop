#include "drop_server.hpp"

#include <string>
#include <utility>

#include "connection_handler.hpp"
#include "log.hpp"
#include "ssh_types.hpp"

namespace drop {

DropServer::DropServer(ServerConfig			config,
		       std::unique_ptr<IAuthenticator>	authenticator,
		       std::unique_ptr<ISecretProvider> secret_provider)
    : config_{std::move(config)},
      authenticator_{std::move(authenticator)},
      secret_provider_{std::move(secret_provider)}
{
}

void DropServer::run(std::atomic<bool>& running)
{
	SshBind bind;
	bind.set_port(config_.port);
	bind.set_host_key(config_.host_key_path);
	bind.listen();

	log::info("Listening on port " + config_.port);

	while (running.load(std::memory_order_relaxed)) {
		SshSession session;

		if (!bind.accept(session, 1000))
			continue;

		log::info("Connection accepted");

		try {
			ConnectionHandler handler{std::move(session),
						  *authenticator_,
						  *secret_provider_};
			handler.run();
		} catch (const std::exception& e) {
			log::error(e.what());
		}
	}

	log::info("Server shutting down");
}

} // namespace drop
