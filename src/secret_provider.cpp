#include "secret_provider.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

} // namespace drop
