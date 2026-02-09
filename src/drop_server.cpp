#include "drop_server.hpp"

#include <utility>

namespace drop {

DropServer::DropServer(ServerConfig config,
                       std::unique_ptr<IAuthenticator> authenticator,
                       std::unique_ptr<ISecretProvider> secret_provider)
	: config_{std::move(config)}
	, authenticator_{std::move(authenticator)}
	, secret_provider_{std::move(secret_provider)}
{
}

void DropServer::run()
{
	SshBind bind;
	bind.set_port(config_.port);
	bind.set_host_key(config_.host_key_path);
	bind.listen();

	SshSession session;

	// Set up server callbacks before key exchange
	ssh_server_callbacks_struct server_cb = {};
	server_cb.userdata = this;
	server_cb.auth_pubkey_function = on_auth_pubkey;
	server_cb.channel_open_request_session_function = on_channel_open;
	ssh_callbacks_init(&server_cb);

	session.set_server_callbacks(&server_cb);
	session.set_auth_methods(authenticator_->supported_methods());

	bind.accept(session);
	session.handle_key_exchange();

	// Poll until authenticated and channel opened
	SshEvent event;
	event.add_session(session);

	while (!authenticated_ || raw_channel_ == nullptr) {
		if (event.poll(100) == SSH_ERROR)
			throw SshError::from(session.get(), "Event poll failed during auth");
	}

	// Set channel callbacks for shell request
	ssh_channel_callbacks_struct channel_cb = {};
	channel_cb.userdata = this;
	channel_cb.channel_shell_request_function = on_shell_request;
	ssh_callbacks_init(&channel_cb);

	SshChannel channel{raw_channel_};
	raw_channel_ = nullptr;
	channel.set_callbacks(&channel_cb);

	while (!got_shell_) {
		if (event.poll(100) == SSH_ERROR)
			throw SshError::from(session.get(), "Event poll failed waiting for shell");
	}

	// Deliver secret
	std::string secret = secret_provider_->get_secret();
	channel.write(secret);
	channel.send_eof();
}

int DropServer::on_auth_pubkey(ssh_session /*session*/, const char* /*user*/,
                               ssh_key_struct* pubkey, char signature_state,
                               void* userdata)
{
	auto* self = static_cast<DropServer*>(userdata);

	if (signature_state == SSH_PUBLICKEY_STATE_NONE) {
		if (self->authenticator_->is_authorized(pubkey))
			return SSH_AUTH_SUCCESS;
		return SSH_AUTH_DENIED;
	}

	if (signature_state == SSH_PUBLICKEY_STATE_VALID) {
		if (self->authenticator_->is_authorized(pubkey)) {
			self->authenticated_ = true;
			return SSH_AUTH_SUCCESS;
		}
	}

	return SSH_AUTH_DENIED;
}

ssh_channel DropServer::on_channel_open(ssh_session session, void* userdata)
{
	auto* self = static_cast<DropServer*>(userdata);
	self->raw_channel_ = ssh_channel_new(session);
	return self->raw_channel_;
}

int DropServer::on_shell_request(ssh_session /*session*/,
                                 ssh_channel /*channel*/, void* userdata)
{
	auto* self = static_cast<DropServer*>(userdata);
	self->got_shell_ = true;
	return 0;
}

} // namespace drop
