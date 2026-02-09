#ifndef SSH_DROP_CONNECTION_HANDLER_H_
#define SSH_DROP_CONNECTION_HANDLER_H_

#include <libssh/libssh.h>

#include "authenticator.h"
#include "secret_provider.h"
#include "ssh_types.h"

namespace drop {

class ConnectionHandler {
public:
	ConnectionHandler(SshSession		 session,
			  const IAuthenticator&	 authenticator,
			  const ISecretProvider& secret_provider,
			  int			 auth_timeout);

	void run();

private:
	static int	   on_auth_pubkey(ssh_session session, const char* user,
					  ssh_key_struct* pubkey, char signature_state,
					  void* userdata);
	static int	   on_auth_password(ssh_session session,
					    const char* user,
					    const char* password,
					    void*	userdata);
	static ssh_channel on_channel_open(ssh_session session, void* userdata);
	static int on_shell_request(ssh_session session, ssh_channel channel,
				    void* userdata);
	static int on_pty_request(ssh_session session, ssh_channel channel,
				  const char* term, int cols, int rows,
				  int py, int px, void* userdata);

	SshSession	       session_;
	const IAuthenticator&  authenticator_;
	const ISecretProvider& secret_provider_;

	int auth_timeout_;

	ssh_channel raw_channel_   = nullptr;
	bool	    authenticated_ = false;
	bool	    got_shell_	   = false;

	bool pubkey_passed_ = false;
	bool requires_both_ = false;
};

} // namespace drop

#endif // SSH_DROP_CONNECTION_HANDLER_H_
