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

	std::vector<std::unique_ptr<ActiveConnection>> connections;

	while (running.load(std::memory_order_relaxed)) {
		std::erase_if(connections, [](const auto& c) {
			return c->done.load(std::memory_order_relaxed);
		});

		SshSession session;

		if (!bind.accept(session, 1000))
			continue;

		log::info("Connection accepted");

		auto  conn	= std::make_unique<ActiveConnection>();
		auto* done_flag = &conn->done;
		int   timeout	= config_.auth_timeout;

		conn->thread = std::jthread([this, s = std::move(session),
					     done_flag, timeout]() mutable {
			try {
				ConnectionHandler handler{
						std::move(s), *authenticator_,
						*secret_provider_, timeout};
				handler.run();
			} catch (const std::exception& e) {
				log::error(e.what());
			}
			done_flag->store(true, std::memory_order_relaxed);
		});

		connections.push_back(std::move(conn));
	}

	log::info("Server shutting down");
}

} // namespace drop
