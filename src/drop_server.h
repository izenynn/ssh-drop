#ifndef SSH_DROP_DROP_SERVER_H_
#define SSH_DROP_DROP_SERVER_H_

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "authenticator.h"
#include "secret_provider.h"
#include "server_config.h"

namespace drop {

class DropServer {
public:
	DropServer(ServerConfig			    config,
		   std::unique_ptr<IAuthenticator>  authenticator,
		   std::unique_ptr<ISecretProvider> secret_provider);

	void run(std::atomic<bool>& running);

private:
	struct ActiveConnection {
		std::jthread	  thread;
		std::atomic<bool> done{false};
	};

	ServerConfig			 config_;
	std::unique_ptr<IAuthenticator>	 authenticator_;
	std::unique_ptr<ISecretProvider> secret_provider_;
};

} // namespace drop

#endif // SSH_DROP_DROP_SERVER_H_
