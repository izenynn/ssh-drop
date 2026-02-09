#include "connection_handler.h"

#include <chrono>
#include <string>
#include <utility>

#include "log.h"

namespace drop {

ConnectionHandler::ConnectionHandler(SshSession		    session,
				     const IAuthenticator&  authenticator,
				     const ISecretProvider& secret_provider,
				     int		    auth_timeout)
    : session_{std::move(session)},
      authenticator_{authenticator},
      secret_provider_{secret_provider},
      auth_timeout_{auth_timeout}
{
}

void ConnectionHandler::run()
{
	const int supported = authenticator_.supported_methods();
	requires_both_	    = supported
			 == (SSH_AUTH_METHOD_PUBLICKEY
			     | SSH_AUTH_METHOD_PASSWORD);

	ssh_server_callbacks_struct server_cb		= {};
	server_cb.userdata				= this;
	server_cb.channel_open_request_session_function = on_channel_open;

	if (supported & SSH_AUTH_METHOD_PUBLICKEY)
		server_cb.auth_pubkey_function = on_auth_pubkey;
	if (supported & SSH_AUTH_METHOD_PASSWORD)
		server_cb.auth_password_function = on_auth_password;

	ssh_callbacks_init(&server_cb);

	session_.set_server_callbacks(&server_cb);

	// Only reveal pubkey initially if both required for security
	const int initial =
			requires_both_ ? SSH_AUTH_METHOD_PUBLICKEY : supported;
	session_.set_auth_methods(initial);

	session_.handle_key_exchange();

	SshEvent event;
	event.add_session(session_);

	const auto deadline = std::chrono::steady_clock::now()
			      + std::chrono::seconds(auth_timeout_);

	while (!authenticated_ || raw_channel_ == nullptr) {
		if (std::chrono::steady_clock::now() >= deadline)
			throw SshError{"Authentication timed out"};
		if (event.poll(100) == SSH_ERROR) {
			if (raw_channel_) {
				ssh_channel_free(raw_channel_);
				raw_channel_ = nullptr;
			}
			throw SshError::from(session_.get(),
					     "Event poll failed during auth");
		}
	}

	log::info("Client authenticated");

	SshChannel channel{raw_channel_};
	raw_channel_ = nullptr;

	ssh_channel_callbacks_struct channel_cb	  = {};
	channel_cb.userdata			  = this;
	channel_cb.channel_shell_request_function = on_shell_request;
	channel_cb.channel_pty_request_function	  = on_pty_request;
	ssh_callbacks_init(&channel_cb);

	channel.set_callbacks(&channel_cb);

	while (!got_shell_) {
		if (std::chrono::steady_clock::now() >= deadline)
			throw SshError{"Authentication timed out"};
		if (event.poll(100) == SSH_ERROR)
			throw SshError::from(
					session_.get(),
					"Event poll failed waiting for shell");
	}

	std::string passphrase;
	if (secret_provider_.needs_passphrase()) {
		passphrase = channel.read(auth_timeout_ * 1000);
		if (passphrase.empty()) {
			log::warn("No passphrase received");
			return;
		}
	}

	std::string secret = secret_provider_.get_secret(passphrase);
	channel.write(secret);
	channel.send_eof();

	log::info("Secret delivered");
}

int ConnectionHandler::on_auth_pubkey(ssh_session session, const char* user,
				      ssh_key_struct* pubkey,
				      char signature_state, void* userdata)
{
	(void)session;

	auto* self = static_cast<ConnectionHandler*>(userdata);

	if (signature_state == SSH_PUBLICKEY_STATE_NONE) {
		if (self->authenticator_.check_pubkey(pubkey))
			return SSH_AUTH_SUCCESS;
		log::debug("Public key not authorized (probe)");
		return SSH_AUTH_DENIED;
	}

	if (signature_state == SSH_PUBLICKEY_STATE_VALID) {
		if (!self->authenticator_.check_pubkey(pubkey)) {
			log::warn("Authentication denied");
			return SSH_AUTH_DENIED;
		}

		if (self->requires_both_) {
			self->pubkey_passed_ = true;
			self->session_.set_auth_methods(
					SSH_AUTH_METHOD_PASSWORD);
			return SSH_AUTH_PARTIAL;
		}

		if (!self->authenticator_.check_user(user)) {
			log::warn("Authentication denied");
			return SSH_AUTH_DENIED;
		}

		self->authenticated_ = true;
		return SSH_AUTH_SUCCESS;
	}

	log::warn("Authentication denied");
	return SSH_AUTH_DENIED;
}

int ConnectionHandler::on_auth_password(ssh_session session, const char* user,
					const char* password, void* userdata)
{
	(void)session;

	auto* self = static_cast<ConnectionHandler*>(userdata);

	if (self->requires_both_ && !self->pubkey_passed_) {
		log::warn("Authentication denied");
		return SSH_AUTH_DENIED;
	}

	if (!self->authenticator_.check_password(password)) {
		log::warn("Authentication denied");
		return SSH_AUTH_DENIED;
	}

	if (!self->authenticator_.check_user(user)) {
		log::warn("Authentication denied");
		return SSH_AUTH_DENIED;
	}

	self->authenticated_ = true;
	return SSH_AUTH_SUCCESS;
}

ssh_channel ConnectionHandler::on_channel_open(ssh_session session,
					       void*	   userdata)
{
	auto* self	   = static_cast<ConnectionHandler*>(userdata);
	self->raw_channel_ = ssh_channel_new(session);
	return self->raw_channel_;
}

int ConnectionHandler::on_shell_request(ssh_session session,
					ssh_channel channel, void* userdata)
{
	(void)session;
	(void)channel;

	auto* self	 = static_cast<ConnectionHandler*>(userdata);
	self->got_shell_ = true;
	return 0;
}

int ConnectionHandler::on_pty_request(ssh_session session, ssh_channel channel,
				      const char* term, int cols, int rows,
				      int py, int px, void* userdata)
{
	(void)session;
	(void)channel;
	(void)term;
	(void)cols;
	(void)rows;
	(void)py;
	(void)px;
	(void)userdata;

	// Accept — puts client in raw mode (no local echo),
	// which hides passphrase input.
	return 0;
}

} // namespace drop
