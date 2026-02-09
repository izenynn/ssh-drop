#include "authenticator.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

#include "ssh_types.hpp"

namespace drop {

AuthorizedKeysAuthenticator::AuthorizedKeysAuthenticator(std::filesystem::path keys_file)
	: keys_file_{std::move(keys_file)}
{
}

int AuthorizedKeysAuthenticator::supported_methods() const noexcept
{
	return SSH_AUTH_METHOD_PUBLICKEY;
}

bool AuthorizedKeysAuthenticator::is_authorized(ssh_key pubkey) const
{
	std::ifstream file(keys_file_);
	if (!file.is_open())
		return false;

	std::string line;
	while (std::getline(file, line)) {
		// Skip blank lines and comments
		size_t pos = 0;
		while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
			pos++;
		if (pos >= line.size() || line[pos] == '#')
			continue;

		// Parse: <key-type> <base64-key> [comment]
		std::istringstream iss(line.substr(pos));
		std::string key_type_name, b64;
		if (!(iss >> key_type_name >> b64))
			continue;

		enum ssh_keytypes_e key_type = ssh_key_type_from_name(key_type_name.c_str());
		if (key_type == SSH_KEYTYPE_UNKNOWN)
			continue;

		ssh_key raw = nullptr;
		int rc = ssh_pki_import_pubkey_base64(b64.c_str(), key_type, &raw);
		if (rc != SSH_OK || raw == nullptr)
			continue;

		SshKeyPtr candidate{raw};
		if (ssh_key_cmp(candidate.get(), pubkey, SSH_KEY_CMP_PUBLIC) == 0)
			return true;
	}

	return false;
}

} // namespace drop
