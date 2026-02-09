#include "ssh_types.hpp"

#include <utility>

namespace drop {

// ─── SshSession ─────────────────────────────────────────────────────────────

SshSession::SshSession()
	: session_{ssh_new()}
{
	if (!session_)
		throw SshError{"ssh_new failed"};
}

SshSession::~SshSession()
{
	if (session_) {
		ssh_disconnect(session_);
		ssh_free(session_);
	}
}

SshSession::SshSession(SshSession&& other) noexcept
	: session_{std::exchange(other.session_, nullptr)}
{
}

SshSession& SshSession::operator=(SshSession&& other) noexcept
{
	if (this != &other) {
		if (session_) {
			ssh_disconnect(session_);
			ssh_free(session_);
		}
		session_ = std::exchange(other.session_, nullptr);
	}
	return *this;
}

void SshSession::handle_key_exchange()
{
	if (ssh_handle_key_exchange(session_) != SSH_OK)
		throw SshError::from(session_, "Key exchange failed");
}

void SshSession::message_loop(std::function<MessageAction(ssh_message)> handler)
{
	ssh_message msg;
	while ((msg = ssh_message_get(session_)) != nullptr) {
		MessageAction action = handler(msg);
		switch (action) {
		case MessageAction::Done:
			ssh_message_free(msg);
			return;
		case MessageAction::Reject:
			ssh_message_reply_default(msg);
			ssh_message_free(msg);
			break;
		case MessageAction::Continue:
			ssh_message_free(msg);
			break;
		}
	}
}

// ─── SshBind ────────────────────────────────────────────────────────────────

SshBind::SshBind()
	: bind_{ssh_bind_new()}
{
	if (!bind_)
		throw SshError{"ssh_bind_new failed"};
}

SshBind::~SshBind()
{
	if (bind_)
		ssh_bind_free(bind_);
}

SshBind::SshBind(SshBind&& other) noexcept
	: bind_{std::exchange(other.bind_, nullptr)}
{
}

SshBind& SshBind::operator=(SshBind&& other) noexcept
{
	if (this != &other) {
		if (bind_)
			ssh_bind_free(bind_);
		bind_ = std::exchange(other.bind_, nullptr);
	}
	return *this;
}

void SshBind::set_port(const std::string& port)
{
	ssh_bind_options_set(bind_, SSH_BIND_OPTIONS_BINDPORT_STR, port.c_str());
}

void SshBind::set_host_key(const std::string& path)
{
	ssh_bind_options_set(bind_, SSH_BIND_OPTIONS_HOSTKEY, path.c_str());
}

void SshBind::listen()
{
	if (ssh_bind_listen(bind_) < 0)
		throw SshError::from(bind_, "Error listening");
}

void SshBind::accept(SshSession& session)
{
	if (ssh_bind_accept(bind_, session.get()) != SSH_OK)
		throw SshError::from(bind_, "Error accepting");
}

// ─── SshChannel ─────────────────────────────────────────────────────────────

SshChannel::SshChannel(ssh_channel raw)
	: channel_{raw}
{
	if (!channel_)
		throw SshError{"Failed to open channel"};
}

SshChannel::~SshChannel()
{
	if (channel_) {
		ssh_channel_close(channel_);
		ssh_channel_free(channel_);
	}
}

SshChannel::SshChannel(SshChannel&& other) noexcept
	: channel_{std::exchange(other.channel_, nullptr)}
{
}

SshChannel& SshChannel::operator=(SshChannel&& other) noexcept
{
	if (this != &other) {
		if (channel_) {
			ssh_channel_close(channel_);
			ssh_channel_free(channel_);
		}
		channel_ = std::exchange(other.channel_, nullptr);
	}
	return *this;
}

void SshChannel::write(std::string_view data)
{
	ssh_channel_write(channel_, data.data(), static_cast<uint32_t>(data.size()));
}

void SshChannel::send_eof()
{
	ssh_channel_send_eof(channel_);
}

void SshChannel::close()
{
	ssh_channel_close(channel_);
}

} // namespace drop
