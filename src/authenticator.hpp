#pragma once

#include <filesystem>

#include <libssh/libssh.h>

namespace drop {

class IAuthenticator {
public:
	virtual ~IAuthenticator() = default;

	[[nodiscard]] virtual int  supported_methods() const noexcept  = 0;
	[[nodiscard]] virtual bool is_authorized(ssh_key pubkey) const = 0;
};

class AuthorizedKeysAuthenticator : public IAuthenticator {
public:
	explicit AuthorizedKeysAuthenticator(std::filesystem::path keys_file);

	[[nodiscard]] int  supported_methods() const noexcept override;
	[[nodiscard]] bool is_authorized(ssh_key pubkey) const override;

private:
	std::filesystem::path keys_file_;
};

} // namespace drop
