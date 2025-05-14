#include <iostream>
#include <cstring>

#include <libssh/server.h>
#include <libssh/libsshpp.hpp>

constexpr char PASSWORD[] = "pacojones";

int main()
{
	int rc;
	ssh_bind sshbind = ssh_bind_new();
	ssh_session session = ssh_new();

	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, "9393");
	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, "key");

	if (ssh_bind_listen(sshbind) < 0) {
		std::cerr << "Error listening: " << ssh_get_error(sshbind) << std::endl;
		return 1;
	}

	rc = ssh_bind_accept(sshbind, session);
	if (rc != SSH_OK) {
		std::cerr << "Error accepting: " << ssh_get_error(sshbind) << std::endl;
		return 1;
	}

	if (ssh_handle_key_exchange(session)) {
		std::cerr << "Key exchange failed: " << ssh_get_error(session) << std::endl;
		return 1;
	}

	ssh_message message;
	bool authenticated = false;
	while ((message = ssh_message_get(session)) != nullptr) {
		if (ssh_message_type(message) == SSH_REQUEST_AUTH && ssh_message_subtype(message) == SSH_AUTH_METHOD_PASSWORD) {
			const char* user = ssh_message_auth_user(message);
			const char* pass = ssh_message_auth_password(message);
			if (strcmp(user, "paco") == 0 && strcmp(pass, "jones") == 0) {
				ssh_message_auth_reply_success(message, 0);
				authenticated = true;
				ssh_message_free(message);
				break;
			}
		}
		ssh_message_auth_set_methods(message, SSH_AUTH_METHOD_PASSWORD);
		ssh_message_reply_default(message);
		ssh_message_free(message);
	}

	if (!authenticated) {
		std::cerr << "Authentication failed." << std::endl;
		ssh_disconnect(session);
		ssh_bind_free(sshbind);
		ssh_finalize();
		return 1;
	}

	ssh_channel chan = ssh_channel_new(session);
	if (ssh_channel_open_session(chan) != SSH_OK) {
		std::cerr << "Failed to open channel." << std::endl;
		return 1;
	}

	ssh_channel_write(chan, PASSWORD, strlen(PASSWORD));
	ssh_channel_send_eof(chan);
	ssh_channel_close(chan);
	ssh_channel_free(chan);

	ssh_disconnect(session);
	ssh_free(session);
	ssh_bind_free(sshbind);
	ssh_finalize();
	return 0;
}
