#include "ssh_types.hpp"

#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

namespace drop {

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

void SshSession::set_server_callbacks(ssh_server_callbacks cb)
{
	if (ssh_set_server_callbacks(session_, cb) != SSH_OK)
		throw SshError{"Failed to set server callbacks"};
}

void SshSession::set_auth_methods(int methods)
{
	ssh_set_auth_methods(session_, methods);
}

void SshSession::handle_key_exchange()
{
	if (ssh_handle_key_exchange(session_) != SSH_OK)
		throw SshError::from(session_, "Key exchange failed");
}

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
	if (ssh_bind_options_set(bind_, SSH_BIND_OPTIONS_BINDPORT_STR,
				 port.c_str())
	    != SSH_OK)
		throw SshError::from(bind_, "Failed to set bind port");
}

void SshBind::set_host_key(const std::string& path)
{
	if (ssh_bind_options_set(bind_, SSH_BIND_OPTIONS_HOSTKEY, path.c_str())
	    != SSH_OK)
		throw SshError::from(bind_, "Failed to set host key");
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

bool SshBind::accept(SshSession& session, int timeout_ms)
{
	socket_t fd = ssh_bind_get_fd(bind_);
	if (fd == SSH_INVALID_SOCKET)
		throw SshError{"ssh_bind has no valid fd"};

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	struct timeval tv;
	tv.tv_sec  = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	int rc = select(static_cast<int>(fd) + 1, &read_fds, nullptr, nullptr,
			&tv);
	if (rc < 0)
		throw SshError{"select() failed on bind fd"};
	if (rc == 0)
		return false;

	if (ssh_bind_accept(bind_, session.get()) != SSH_OK)
		throw SshError::from(bind_, "Error accepting");

	return true;
}

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

void SshChannel::set_callbacks(ssh_channel_callbacks cb)
{
	if (ssh_set_channel_callbacks(channel_, cb) != SSH_OK)
		throw SshError{"Failed to set channel callbacks"};
}

std::string SshChannel::read(int timeout_ms)
{
	std::string result;
	char	    buf[256];

	for (;;) {
		const int n = ssh_channel_read_timeout(
				channel_, buf, sizeof(buf), 0, timeout_ms);
		if (n < 0)
			throw SshError{"Channel read failed"};
		if (n == 0)
			break;

		result.append(buf, static_cast<std::size_t>(n));

		// Stop at first line terminator — we only need one line
		// PTY mode sends \r for Enter, non-PTY sends \n
		if (result.find('\n') != std::string::npos
		    || result.find('\r') != std::string::npos)
			break;
	}

	// Trim trailing newline/carriage-return
	while (!result.empty()
	       && (result.back() == '\n' || result.back() == '\r'))
		result.pop_back();

	return result;
}

void SshChannel::write(std::string_view data)
{
	ssh_channel_write(channel_, data.data(),
			  static_cast<uint32_t>(data.size()));
}

void SshChannel::send_eof()
{
	ssh_channel_send_eof(channel_);
}

void SshChannel::close()
{
	ssh_channel_close(channel_);
}

SshEvent::SshEvent()
    : event_{ssh_event_new()}
{
	if (!event_)
		throw SshError{"ssh_event_new failed"};
}

SshEvent::~SshEvent()
{
	if (event_)
		ssh_event_free(event_);
}

SshEvent::SshEvent(SshEvent&& other) noexcept
    : event_{std::exchange(other.event_, nullptr)}
{
}

SshEvent& SshEvent::operator=(SshEvent&& other) noexcept
{
	if (this != &other) {
		if (event_)
			ssh_event_free(event_);
		event_ = std::exchange(other.event_, nullptr);
	}
	return *this;
}

void SshEvent::add_session(SshSession& session)
{
	if (ssh_event_add_session(event_, session.get()) != SSH_OK)
		throw SshError{"Failed to add session to event loop"};
}

int SshEvent::poll(int timeout_ms)
{
	return ssh_event_dopoll(event_, timeout_ms);
}

} // namespace drop
