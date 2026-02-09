#pragma once

#include <stdexcept>
#include <string>

#include <libssh/libssh.h>

namespace drop {

class SshError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	template <typename T>
	static SshError from(T handle, const std::string& context)
	{
		return SshError{context + ": " + ssh_get_error(handle)};
	}
};

} // namespace drop
