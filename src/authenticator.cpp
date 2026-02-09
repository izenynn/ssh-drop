#include "authenticator.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "server_config.hpp"
#include "ssh_types.hpp"

namespace drop {

// --- AuthorizedKeysAuthenticator ---

AuthorizedKeysAuthenticator::AuthorizedKeysAuthenticator(
		std::filesystem::path keys_file)
    : keys_file_{std::move(keys_file)}
{
}

bool AuthorizedKeysAuthenticator::check_pubkey(ssh_key pubkey) const
{
	std::ifstream file(keys_file_);
	if (!file.is_open())
		return false;

	std::string line;
	while (std::getline(file, line)) {
		// Skip blank lines and comments
		size_t pos = 0;
		while (pos < line.size()
		       && std::isspace(static_cast<unsigned char>(line[pos])))
			pos++;
		if (pos >= line.size() || line[pos] == '#')
			continue;

		// Parse: <key-type> <base64-key> [comment]
		std::istringstream iss(line.substr(pos));
		std::string	   key_type_name, b64;
		if (!(iss >> key_type_name >> b64))
			continue;

		enum ssh_keytypes_e key_type =
				ssh_key_type_from_name(key_type_name.c_str());
		if (key_type == SSH_KEYTYPE_UNKNOWN)
			continue;

		ssh_key raw = nullptr;
		int	rc = ssh_pki_import_pubkey_base64(b64.c_str(), key_type,
							  &raw);
		if (rc != SSH_OK || raw == nullptr)
			continue;

		SshKeyPtr candidate{raw};
		if (ssh_key_cmp(candidate.get(), pubkey, SSH_KEY_CMP_PUBLIC)
		    == 0)
			return true;
	}

	return false;
}

// --- Authenticator (composite) ---

Authenticator::Authenticator(int			     methods,
			     std::filesystem::path	     keys_file,
			     std::unique_ptr<ISecretProvider> password_provider,
			     std::unique_ptr<ISecretProvider> user_provider)
    : methods_{methods},
      pubkey_auth_{std::move(keys_file)},
      password_provider_{std::move(password_provider)},
      user_provider_{std::move(user_provider)}
{
}

int Authenticator::supported_methods() const noexcept
{
	return methods_;
}

bool Authenticator::check_pubkey(ssh_key pubkey) const
{
	return pubkey_auth_.check_pubkey(pubkey);
}

bool Authenticator::check_password(std::string_view pw) const
{
	if (!password_provider_)
		return false;
	return password_provider_->get_secret() == pw;
}

bool Authenticator::check_user(std::string_view user) const
{
	if (!user_provider_)
		return true;
	return user_provider_->get_secret() == user;
}

// --- Factory ---

std::unique_ptr<IAuthenticator>
make_authenticator(const ServerConfig& config)
{
	int methods = 0;
	if (config.auth_method == "publickey") {
		methods = SSH_AUTH_METHOD_PUBLICKEY;
	} else if (config.auth_method == "password") {
		methods = SSH_AUTH_METHOD_PASSWORD;
	} else if (config.auth_method == "both") {
		methods = SSH_AUTH_METHOD_PUBLICKEY
			  | SSH_AUTH_METHOD_PASSWORD;
	}

	auto pw_provider = make_value_provider(config.auth_password,
					       config.auth_password_file,
					       config.auth_password_env);

	auto user_provider = make_value_provider(config.auth_user,
						 config.auth_user_file,
						 config.auth_user_env);

	return std::make_unique<Authenticator>(
			methods, config.authorized_keys_path,
			std::move(pw_provider), std::move(user_provider));
}

} // namespace drop
