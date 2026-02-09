#include "secret_provider.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "server_config.hpp"

namespace drop {

StaticSecretProvider::StaticSecretProvider(std::string secret)
    : secret_{std::move(secret)}
{
}

std::string StaticSecretProvider::get_secret() const
{
	return secret_;
}

EnvSecretProvider::EnvSecretProvider(std::string var_name)
    : var_name_{std::move(var_name)}
{
}

std::string EnvSecretProvider::get_secret() const
{
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

std::string FileSecretProvider::get_secret() const
{
	std::ifstream file(path_);
	if (!file.is_open())
		throw std::runtime_error{"Could not open secret file: "
					 + path_.string()};

	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
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
				"No secret source configured "
				"(set secret, secret_file, or secret_env)"};
	return p;
}

} // namespace drop
