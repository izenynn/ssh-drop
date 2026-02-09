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
	bind.accept(session);
	session.handle_key_exchange();

	authenticate(session);
	SshChannel channel = open_channel(session);
	wait_for_shell(session, channel);
	deliver_secret(channel);
}

void DropServer::authenticate(SshSession& session)
{
	bool authenticated = false;

	session.message_loop([&](ssh_message msg) -> MessageAction {
		if (ssh_message_type(msg) != SSH_REQUEST_AUTH)
			return MessageAction::Reject;

		if (ssh_message_subtype(msg) == SSH_AUTH_METHOD_PUBLICKEY) {
			int state = ssh_message_auth_publickey_state(msg);
			ssh_key pubkey = ssh_message_auth_pubkey(msg);

			if (state == SSH_PUBLICKEY_STATE_NONE) {
				if (authenticator_->is_authorized(pubkey)) {
					ssh_message_auth_reply_pk_ok_simple(msg);
					return MessageAction::Continue;
				}
			} else if (state == SSH_PUBLICKEY_STATE_VALID) {
				if (authenticator_->is_authorized(pubkey)) {
					ssh_message_auth_reply_success(msg, 0);
					authenticated = true;
					return MessageAction::Done;
				}
			}
		}

		ssh_message_auth_set_methods(msg, authenticator_->supported_methods());
		return MessageAction::Reject;
	});

	if (!authenticated)
		throw SshError{"Authentication failed"};
}

SshChannel DropServer::open_channel(SshSession& session)
{
	ssh_channel raw = nullptr;

	session.message_loop([&](ssh_message msg) -> MessageAction {
		if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN &&
		    ssh_message_subtype(msg) == SSH_CHANNEL_SESSION) {
			raw = ssh_message_channel_request_open_reply_accept(msg);
			return MessageAction::Done;
		}
		return MessageAction::Reject;
	});

	return SshChannel{raw};
}

void DropServer::wait_for_shell(SshSession& session, SshChannel& channel)
{
	bool shell = false;

	session.message_loop([&](ssh_message msg) -> MessageAction {
		if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL &&
		    ssh_message_subtype(msg) == SSH_CHANNEL_REQUEST_SHELL) {
			ssh_message_channel_request_reply_success(msg);
			shell = true;
			return MessageAction::Done;
		}
		return MessageAction::Reject;
	});

	if (!shell)
		throw SshError{"Failed to get shell request"};
}

void DropServer::deliver_secret(SshChannel& channel)
{
	std::string secret = secret_provider_->get_secret();
	channel.write(secret);
	channel.send_eof();
	channel.close();
}

} // namespace drop
