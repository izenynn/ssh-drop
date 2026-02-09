#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace drop {

struct ServerConfig;

class ISecretProvider {
public:
	virtual ~ISecretProvider() = default;

	[[nodiscard]] virtual std::string get_secret() const = 0;
};

class StaticSecretProvider : public ISecretProvider {
public:
	explicit StaticSecretProvider(std::string secret);

	[[nodiscard]] std::string get_secret() const override;

private:
	std::string secret_;
};

class EnvSecretProvider : public ISecretProvider {
public:
	explicit EnvSecretProvider(std::string var_name);

	[[nodiscard]] std::string get_secret() const override;

private:
	std::string var_name_;
};

class FileSecretProvider : public ISecretProvider {
public:
	explicit FileSecretProvider(std::filesystem::path path);

	[[nodiscard]] std::string get_secret() const override;

private:
	std::filesystem::path path_;
};

[[nodiscard]] std::unique_ptr<ISecretProvider>
make_value_provider(const std::optional<std::string>& value,
		    const std::optional<std::string>& file_path,
		    const std::optional<std::string>& env_name);

[[nodiscard]] std::unique_ptr<ISecretProvider>
make_secret_provider(const ServerConfig& config);

} // namespace drop
