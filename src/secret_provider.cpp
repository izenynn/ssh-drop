#include "secret_provider.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "server_config.hpp"

namespace drop {

// ─── StaticSecretProvider ───────────────────────────────────────────────────

StaticSecretProvider::StaticSecretProvider(std::string secret)
	: secret_{std::move(secret)}
{
}

std::string StaticSecretProvider::get_secret() const
{
	return secret_;
}

// ─── EnvSecretProvider ──────────────────────────────────────────────────────

EnvSecretProvider::EnvSecretProvider(std::string var_name)
	: var_name_{std::move(var_name)}
{
}

std::string EnvSecretProvider::get_secret() const
{
	const char* val = std::getenv(var_name_.c_str());
	if (!val)
		throw std::runtime_error{"Environment variable not set: " + var_name_};
	return val;
}

// ─── FileSecretProvider ─────────────────────────────────────────────────────

FileSecretProvider::FileSecretProvider(std::filesystem::path path)
	: path_{std::move(path)}
{
}

std::string FileSecretProvider::get_secret() const
{
	std::ifstream file(path_);
	if (!file.is_open())
		throw std::runtime_error{"Could not open secret file: " + path_.string()};

	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

// ─── Factory ────────────────────────────────────────────────────────────────

std::unique_ptr<ISecretProvider> make_secret_provider(const ServerConfig& config)
{
	if (!config.secret_value.empty())
		return std::make_unique<StaticSecretProvider>(config.secret_value);
	if (!config.secret_file_path.empty())
		return std::make_unique<FileSecretProvider>(config.secret_file_path);
	if (!config.secret_env_name.empty())
		return std::make_unique<EnvSecretProvider>(config.secret_env_name);

	throw std::runtime_error{
		"No secret source configured "
		"(set secret, secret_file, or secret_env)"};
}

} // namespace drop
