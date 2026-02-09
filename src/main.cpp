#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "authenticator.hpp"
#include "drop_server.hpp"
#include "encrypt_command.hpp"
#include "log.hpp"
#include "secret_provider.hpp"
#include "server_config.hpp"
#include "signal_guard.hpp"
#include "ssh_error.hpp"
#include "ssh_lib_guard.hpp"

int main(int argc, char* argv[])
{
	if (argc >= 3 && std::strcmp(argv[1], "--encrypt") == 0)
		return drop::run_encrypt(argv[2]);
	if (argc >= 3 && std::strcmp(argv[1], "--decrypt") == 0)
		return drop::run_decrypt(argv[2]);

	try {
		auto config = drop::ServerConfig::load(argc, argv);
		config.validate();

		drop::log::init(drop::log::level_from_string(config.log_level),
				config.log_file);
		drop::log::info("Starting");

		drop::SshLibGuard lib;
		std::atomic<bool> running{true};
		drop::SignalGuard signals{running};

		auto auth   = drop::make_authenticator(config);
		auto secret = drop::make_secret_provider(config);

		drop::DropServer server{std::move(config), std::move(auth),
					std::move(secret)};
		server.run(running);
	} catch (const drop::SshError& e) {
		drop::log::error(e.what());
		return 1;
	} catch (const std::exception& e) {
		drop::log::error(e.what());
		return 1;
	}

	return 0;
}
