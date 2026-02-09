#include "crypto.hpp"

#include <stdexcept>
#include <vector>

#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/gcm.h>
#include <mbedtls/pkcs5.h>

namespace drop::crypto {

namespace {

struct Rng {
	mbedtls_entropy_context	 entropy;
	mbedtls_ctr_drbg_context drbg;

	Rng()
	{
		mbedtls_entropy_init(&entropy);
		mbedtls_ctr_drbg_init(&drbg);
		if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
					  nullptr, 0)
		    != 0)
			throw std::runtime_error{"CSPRNG seed failed"};
	}

	~Rng()
	{
		mbedtls_ctr_drbg_free(&drbg);
		mbedtls_entropy_free(&entropy);
	}

	void fill(unsigned char* buf, std::size_t len)
	{
		if (mbedtls_ctr_drbg_random(&drbg, buf, len) != 0)
			throw std::runtime_error{"CSPRNG generation failed"};
	}
};

void derive_key(const unsigned char* salt, std::string_view passphrase,
		unsigned char* key_out)
{
	mbedtls_md_context_t md;
	mbedtls_md_init(&md);

	const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!info)
		throw std::runtime_error{"SHA-256 not available"};

	if (mbedtls_md_setup(&md, info, 1) != 0) {
		mbedtls_md_free(&md);
		throw std::runtime_error{"md_setup failed"};
	}

	int rc = mbedtls_pkcs5_pbkdf2_hmac(
			&md,
			reinterpret_cast<const unsigned char*>(
					passphrase.data()),
			passphrase.size(), salt, kSaltLen, kPbkdf2Iters,
			kKeyLen, key_out);

	mbedtls_md_free(&md);

	if (rc != 0)
		throw std::runtime_error{"PBKDF2 derivation failed"};
}

std::string base64_encode(const unsigned char* data, std::size_t len)
{
	std::size_t out_len = 0;
	mbedtls_base64_encode(nullptr, 0, &out_len, data, len);

	std::string result(out_len, '\0');
	if (mbedtls_base64_encode(
			    reinterpret_cast<unsigned char*>(result.data()),
			    result.size(), &out_len, data, len)
	    != 0)
		throw std::runtime_error{"Base64 encode failed"};

	// mbedtls adds a null terminator in the count; trim it
	if (!result.empty() && result.back() == '\0')
		result.pop_back();

	return result;
}

std::vector<unsigned char> base64_decode(std::string_view b64)
{
	std::size_t out_len = 0;
	mbedtls_base64_decode(nullptr, 0, &out_len,
			      reinterpret_cast<const unsigned char*>(b64.data()),
			      b64.size());

	std::vector<unsigned char> result(out_len);
	if (mbedtls_base64_decode(
			    result.data(), result.size(), &out_len,
			    reinterpret_cast<const unsigned char*>(b64.data()),
			    b64.size())
	    != 0)
		throw std::runtime_error{
				"Base64 decode failed (corrupt data)"};

	result.resize(out_len);
	return result;
}

} // namespace

std::string encrypt(std::string_view plaintext, std::string_view passphrase)
{
	Rng rng;

	unsigned char salt[kSaltLen];
	unsigned char nonce[kNonceLen];
	rng.fill(salt, kSaltLen);
	rng.fill(nonce, kNonceLen);

	unsigned char key[kKeyLen];
	derive_key(salt, passphrase, key);

	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);

	if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
		mbedtls_gcm_free(&gcm);
		throw std::runtime_error{"GCM setkey failed"};
	}

	std::vector<unsigned char> out(kHeaderLen + plaintext.size());
	unsigned char		   tag[kTagLen];

	int rc = mbedtls_gcm_crypt_and_tag(
			&gcm, MBEDTLS_GCM_ENCRYPT, plaintext.size(), nonce,
			kNonceLen, nullptr, 0,
			reinterpret_cast<const unsigned char*>(
					plaintext.data()),
			out.data() + kHeaderLen, kTagLen, tag);

	mbedtls_gcm_free(&gcm);

	if (rc != 0)
		throw std::runtime_error{"GCM encryption failed"};

	// Layout: salt || nonce || tag || ciphertext
	std::copy(salt, salt + kSaltLen, out.data());
	std::copy(nonce, nonce + kNonceLen, out.data() + kSaltLen);
	std::copy(tag, tag + kTagLen, out.data() + kSaltLen + kNonceLen);

	return base64_encode(out.data(), out.size());
}

std::optional<std::string> decrypt(std::string_view data_b64, std::string_view passphrase)
{
	auto data = base64_decode(data_b64);

	if (data.size() < static_cast<std::size_t>(kHeaderLen))
		throw std::runtime_error{
				"Encrypted data too short (corrupt or not encrypted)"};

	const unsigned char* salt	= data.data();
	const unsigned char* nonce	= data.data() + kSaltLen;
	const unsigned char* tag	= data.data() + kSaltLen + kNonceLen;
	const unsigned char* ciphertext = data.data() + kHeaderLen;
	const std::size_t    ct_len	= data.size() - kHeaderLen;

	unsigned char key[kKeyLen];
	derive_key(salt, passphrase, key);

	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);

	if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
		mbedtls_gcm_free(&gcm);
		throw std::runtime_error{"GCM setkey failed"};
	}

	std::string plaintext(ct_len, '\0');

	int rc = mbedtls_gcm_auth_decrypt(
			&gcm, ct_len, nonce, kNonceLen, nullptr, 0, tag,
			kTagLen, ciphertext,
			reinterpret_cast<unsigned char*>(plaintext.data()));

	mbedtls_gcm_free(&gcm);

	if (rc != 0)
		return std::nullopt;

	return plaintext;
}

} // namespace drop::crypto
