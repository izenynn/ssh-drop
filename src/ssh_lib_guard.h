#ifndef SSH_DROP_SSH_LIB_GUARD_H_
#define SSH_DROP_SSH_LIB_GUARD_H_

#include <libssh/libssh.h>

#include "ssh_error.h"

namespace drop {

class SshLibGuard {
public:
	SshLibGuard()
	{
		if (ssh_init() != SSH_OK)
			throw SshError{"ssh_init failed"};
	}

	~SshLibGuard()
	{
		ssh_finalize();
	}

	SshLibGuard(const SshLibGuard&)		   = delete;
	SshLibGuard& operator=(const SshLibGuard&) = delete;
	SshLibGuard(SshLibGuard&&)		   = delete;
	SshLibGuard& operator=(SshLibGuard&&)	   = delete;
};

} // namespace drop

#endif // SSH_DROP_SSH_LIB_GUARD_H_
