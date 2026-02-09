#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <libssh/callbacks.h>
#include <libssh/server.h>

#include "ssh_error.hpp"

namespace drop {

// ─── SshKeyPtr ──────────────────────────────────────────────────────────────

using SshKeyPtr = std::unique_ptr<
	ssh_key_struct,
	decltype([](ssh_key k) { ssh_key_free(k); })>;

// ─── SshSession ─────────────────────────────────────────────────────────────

class SshSession {
public:
	SshSession();
	~SshSession();

	SshSession(SshSession&& other) noexcept;
	SshSession& operator=(SshSession&& other) noexcept;
	SshSession(const SshSession&) = delete;
	SshSession& operator=(const SshSession&) = delete;

	void set_server_callbacks(ssh_server_callbacks cb);
	void set_auth_methods(int methods);
	void handle_key_exchange();

	ssh_session get() const noexcept { return session_; }

private:
	ssh_session session_ = nullptr;
};

// ─── SshBind ────────────────────────────────────────────────────────────────

class SshBind {
public:
	SshBind();
	~SshBind();

	SshBind(SshBind&& other) noexcept;
	SshBind& operator=(SshBind&& other) noexcept;
	SshBind(const SshBind&) = delete;
	SshBind& operator=(const SshBind&) = delete;

	void set_port(const std::string& port);
	void set_host_key(const std::string& path);
	void listen();
	void accept(SshSession& session);

	ssh_bind get() const noexcept { return bind_; }

private:
	ssh_bind bind_ = nullptr;
};

// ─── SshChannel ─────────────────────────────────────────────────────────────

class SshChannel {
public:
	explicit SshChannel(ssh_channel raw);
	~SshChannel();

	SshChannel(SshChannel&& other) noexcept;
	SshChannel& operator=(SshChannel&& other) noexcept;
	SshChannel(const SshChannel&) = delete;
	SshChannel& operator=(const SshChannel&) = delete;

	void set_callbacks(ssh_channel_callbacks cb);
	void write(std::string_view data);
	void send_eof();
	void close();

	ssh_channel get() const noexcept { return channel_; }

private:
	ssh_channel channel_ = nullptr;
};

// ─── SshEvent ───────────────────────────────────────────────────────────────

class SshEvent {
public:
	SshEvent();
	~SshEvent();

	SshEvent(SshEvent&& other) noexcept;
	SshEvent& operator=(SshEvent&& other) noexcept;
	SshEvent(const SshEvent&) = delete;
	SshEvent& operator=(const SshEvent&) = delete;

	void add_session(SshSession& session);
	int poll(int timeout_ms);

private:
	ssh_event event_ = nullptr;
};

} // namespace drop
