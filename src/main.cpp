#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "authenticator.h"
#include "drop_server.h"
#include "encrypt_command.h"
#include "log.h"
#include "secret_provider.h"
#include "server_config.h"
#include "signal_guard.h"
#include "ssh_error.h"
#include "ssh_lib_guard.h"

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
