#include "connection_handler.hpp"

#include <string>
#include <utility>

#include "log.hpp"

namespace drop {

ConnectionHandler::ConnectionHandler(SshSession		    session,
				     const IAuthenticator&  authenticator,
				     const ISecretProvider& secret_provider)
    : session_{std::move(session)},
      authenticator_{authenticator},
      secret_provider_{secret_provider}
{
}

void ConnectionHandler::run()
{
	ssh_server_callbacks_struct server_cb		= {};
	server_cb.userdata				= this;
	server_cb.auth_pubkey_function			= on_auth_pubkey;
	server_cb.channel_open_request_session_function = on_channel_open;
	ssh_callbacks_init(&server_cb);

	session_.set_server_callbacks(&server_cb);
	session_.set_auth_methods(authenticator_.supported_methods());

	session_.handle_key_exchange();

	SshEvent event;
	event.add_session(session_);

	while (!authenticated_ || raw_channel_ == nullptr) {
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
	ssh_callbacks_init(&channel_cb);

	channel.set_callbacks(&channel_cb);

	while (!got_shell_) {
		if (event.poll(100) == SSH_ERROR)
			throw SshError::from(
					session_.get(),
					"Event poll failed waiting for shell");
	}

	std::string secret = secret_provider_.get_secret();
	channel.write(secret);
	channel.send_eof();

	log::info("Secret delivered");
}

int ConnectionHandler::on_auth_pubkey(ssh_session /*session*/,
				      const char* /*user*/,
				      ssh_key_struct* pubkey,
				      char signature_state, void* userdata)
{
	auto* self = static_cast<ConnectionHandler*>(userdata);

	if (signature_state == SSH_PUBLICKEY_STATE_NONE) {
		if (self->authenticator_.is_authorized(pubkey))
			return SSH_AUTH_SUCCESS;
		log::debug("Public key not authorized (probe)");
		return SSH_AUTH_DENIED;
	}

	if (signature_state == SSH_PUBLICKEY_STATE_VALID) {
		if (self->authenticator_.is_authorized(pubkey)) {
			self->authenticated_ = true;
			return SSH_AUTH_SUCCESS;
		}
	}

	log::warn("Authentication denied");
	return SSH_AUTH_DENIED;
}

ssh_channel ConnectionHandler::on_channel_open(ssh_session session,
					       void*	   userdata)
{
	auto* self	   = static_cast<ConnectionHandler*>(userdata);
	self->raw_channel_ = ssh_channel_new(session);
	return self->raw_channel_;
}

int ConnectionHandler::on_shell_request(ssh_session /*session*/,
					ssh_channel /*channel*/, void* userdata)
{
	auto* self	 = static_cast<ConnectionHandler*>(userdata);
	self->got_shell_ = true;
	return 0;
}

} // namespace drop
