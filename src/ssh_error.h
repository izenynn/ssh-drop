#ifndef SSH_DROP_SSH_ERROR_H_
#define SSH_DROP_SSH_ERROR_H_

#include <stdexcept>
#include <string>

#include <libssh/libssh.h>

namespace drop {

class SshError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	template<typename T>
	static SshError from(T handle, const std::string& context)
	{
		return SshError{context + ": " + ssh_get_error(handle)};
	}
};

} // namespace drop

#endif // SSH_DROP_SSH_ERROR_H_
