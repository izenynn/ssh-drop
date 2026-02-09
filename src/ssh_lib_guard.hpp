#pragma once

#include <libssh/libssh.h>

#include "ssh_error.hpp"

namespace drop {

class SshLibGuard {
public:
	SshLibGuard()
	{
		if (ssh_init() != SSH_OK)
			throw SshError{"ssh_init failed"};
	}

	~SshLibGuard() { ssh_finalize(); }

	SshLibGuard(const SshLibGuard&) = delete;
	SshLibGuard& operator=(const SshLibGuard&) = delete;
	SshLibGuard(SshLibGuard&&) = delete;
	SshLibGuard& operator=(SshLibGuard&&) = delete;
};

} // namespace drop
