#ifndef SSH_DROP_SECRET_PROVIDER_H_
#define SSH_DROP_SECRET_PROVIDER_H_

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace drop {

struct ServerConfig;

class ISecretProvider {
public:
	virtual ~ISecretProvider() = default;

	[[nodiscard]] virtual bool needs_passphrase() const
	{
		return false;
	}

	[[nodiscard]] virtual std::string
	get_secret(std::string_view passphrase = {}) const = 0;
};

class StaticSecretProvider : public ISecretProvider {
public:
	explicit StaticSecretProvider(std::string secret);

	[[nodiscard]] std::string
	get_secret(std::string_view passphrase = {}) const override;

private:
	std::string secret_;
};

class EnvSecretProvider : public ISecretProvider {
public:
	explicit EnvSecretProvider(std::string var_name);

	[[nodiscard]] std::string
	get_secret(std::string_view passphrase = {}) const override;

private:
	std::string var_name_;
};

class FileSecretProvider : public ISecretProvider {
public:
	explicit FileSecretProvider(std::filesystem::path path);

	[[nodiscard]] std::string
	get_secret(std::string_view passphrase = {}) const override;

private:
	std::filesystem::path path_;
};

class EncryptedSecretProvider : public ISecretProvider {
public:
	explicit EncryptedSecretProvider(std::unique_ptr<ISecretProvider> inner);

	[[nodiscard]] bool needs_passphrase() const override
	{
		return true;
	}

	[[nodiscard]] std::string
	get_secret(std::string_view passphrase = {}) const override;

private:
	std::unique_ptr<ISecretProvider> inner_;
};

[[nodiscard]] std::unique_ptr<ISecretProvider>
make_value_provider(const std::optional<std::string>& value,
		    const std::optional<std::string>& file_path,
		    const std::optional<std::string>& env_name);

[[nodiscard]] std::unique_ptr<ISecretProvider>
make_secret_provider(const ServerConfig& config);

} // namespace drop

#endif // SSH_DROP_SECRET_PROVIDER_H_
