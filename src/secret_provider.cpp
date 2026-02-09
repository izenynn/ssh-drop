#include "secret_provider.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "crypto.hpp"
#include "server_config.hpp"

namespace drop {

StaticSecretProvider::StaticSecretProvider(std::string secret)
    : secret_{std::move(secret)}
{
}

std::string StaticSecretProvider::get_secret(std::string_view passphrase) const
{
	(void)passphrase;

	return secret_;
}

EnvSecretProvider::EnvSecretProvider(std::string var_name)
    : var_name_{std::move(var_name)}
{
}

std::string EnvSecretProvider::get_secret(std::string_view passphrase) const
{
	(void)passphrase;

	const char* val = std::getenv(var_name_.c_str());
	if (!val)
		throw std::runtime_error{"Environment variable not set: "
					 + var_name_};
	return val;
}

FileSecretProvider::FileSecretProvider(std::filesystem::path path)
    : path_{std::move(path)}
{
}

std::string FileSecretProvider::get_secret(std::string_view passphrase) const
{
	(void)passphrase;

	std::ifstream file(path_);
	if (!file.is_open())
		throw std::runtime_error{"Could not open secret file: "
					 + path_.string()};

	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

EncryptedSecretProvider::EncryptedSecretProvider(
		std::unique_ptr<ISecretProvider> inner)
    : inner_{std::move(inner)}
{
}

std::string
EncryptedSecretProvider::get_secret(std::string_view passphrase) const
{
	std::string data_b64 = inner_->get_secret();
	auto	    result   = crypto::decrypt(data_b64, passphrase);
	if (!result)
		throw std::runtime_error{
				"Decryption failed (wrong passphrase)"};
	return std::move(*result);
}

std::unique_ptr<ISecretProvider>
make_value_provider(const std::optional<std::string>& value,
		    const std::optional<std::string>& file_path,
		    const std::optional<std::string>& env_name)
{
	if (value.has_value())
		return std::make_unique<StaticSecretProvider>(*value);
	if (file_path.has_value())
		return std::make_unique<FileSecretProvider>(*file_path);
	if (env_name.has_value())
		return std::make_unique<EnvSecretProvider>(*env_name);
	return nullptr;
}

std::unique_ptr<ISecretProvider>
make_secret_provider(const ServerConfig& config)
{
	auto p = make_value_provider(config.secret, config.secret_file,
				     config.secret_env);
	if (!p)
		throw std::runtime_error{
				"No secret source configured (set secret, "
				"secret_file, or secret_env)"};

	if (config.secret_encrypted)
		p = std::make_unique<EncryptedSecretProvider>(std::move(p));

	return p;
}

} // namespace drop
