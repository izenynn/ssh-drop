#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include <libssh/libssh.h>

#include "secret_provider.hpp"

namespace drop {

struct ServerConfig;

class IAuthenticator {
public:
	virtual ~IAuthenticator() = default;

	[[nodiscard]] virtual int  supported_methods() const noexcept	      = 0;
	[[nodiscard]] virtual bool check_pubkey(ssh_key pubkey) const	      = 0;
	[[nodiscard]] virtual bool check_password(std::string_view pw) const  = 0;
	[[nodiscard]] virtual bool check_user(std::string_view user) const    = 0;
};

class AuthorizedKeysAuthenticator {
public:
	explicit AuthorizedKeysAuthenticator(
			std::filesystem::path keys_file);

	[[nodiscard]] bool check_pubkey(ssh_key pubkey) const;

private:
	std::filesystem::path keys_file_;
};

class Authenticator : public IAuthenticator {
public:
	Authenticator(int			     methods,
		      std::filesystem::path	     keys_file,
		      std::unique_ptr<ISecretProvider> password_provider,
		      std::unique_ptr<ISecretProvider> user_provider);

	[[nodiscard]] int  supported_methods() const noexcept override;
	[[nodiscard]] bool check_pubkey(ssh_key pubkey) const override;
	[[nodiscard]] bool check_password(std::string_view pw) const override;
	[[nodiscard]] bool check_user(std::string_view user) const override;

private:
	int				   methods_;
	AuthorizedKeysAuthenticator	   pubkey_auth_;
	std::unique_ptr<ISecretProvider> password_provider_;
	std::unique_ptr<ISecretProvider> user_provider_;
};

[[nodiscard]] std::unique_ptr<IAuthenticator>
make_authenticator(const ServerConfig& config);

} // namespace drop
