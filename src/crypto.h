#ifndef SSH_DROP_CRYPTO_H_
#define SSH_DROP_CRYPTO_H_

#include <optional>
#include <string>
#include <string_view>

namespace drop::crypto {

constexpr int kSaltLen	   = 16;
constexpr int kNonceLen	   = 12;
constexpr int kTagLen	   = 16;
constexpr int kKeyLen	   = 32;
constexpr int kPbkdf2Iters = 210000;
constexpr int kHeaderLen   = kSaltLen + kNonceLen + kTagLen;

[[nodiscard]] std::string encrypt(std::string_view plaintext,
				  std::string_view passphrase);

[[nodiscard]] std::optional<std::string> decrypt(std::string_view data_b64,
						  std::string_view passphrase);

} // namespace drop::crypto

#endif // SSH_DROP_CRYPTO_H_
