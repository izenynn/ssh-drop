#include <iostream>
#include <memory>

#include "authenticator.hpp"
#include "drop_server.hpp"
#include "secret_provider.hpp"
#include "server_config.hpp"
#include "ssh_error.hpp"
#include "ssh_lib_guard.hpp"

int main()
{
	try {
		drop::SshLibGuard lib;
		drop::ServerConfig config;

		auto auth = std::make_unique<drop::AuthorizedKeysAuthenticator>(
			config.authorized_keys_path);
		auto secret = std::make_unique<drop::StaticSecretProvider>("pacojones");

		drop::DropServer server(std::move(config), std::move(auth), std::move(secret));
		server.run();
	} catch (const drop::SshError& e) {
		std::cerr << "SSH error: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
