#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>

#include <libssh/server.h>
#include <libssh/libsshpp.hpp>

constexpr char SECRET[] = "pacojones";
constexpr char AUTHORIZED_KEYS_PATH[] = "key/authorized_keys";

static bool key_is_authorized(ssh_key pubkey)
{
	std::ifstream file(AUTHORIZED_KEYS_PATH);
	if (!file.is_open()) {
		std::cerr << "Could not open " << AUTHORIZED_KEYS_PATH << std::endl;
		return false;
	}

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

		ssh_key candidate = nullptr;
		int rc = ssh_pki_import_pubkey_base64(b64.c_str(), key_type, &candidate);
		if (rc != SSH_OK || candidate == nullptr)
			continue;

		int cmp = ssh_key_cmp(candidate, pubkey, SSH_KEY_CMP_PUBLIC);
		ssh_key_free(candidate);

		if (cmp == 0)
			return true;
	}

	return false;
}

int main()
{
	int rc;
	rc = ssh_init();
	if (rc != SSH_OK) {
		std::cerr << "ssh_init failed with code: " << rc << std::endl;
		return 1;
	}
	ssh_bind sshbind = ssh_bind_new();
	ssh_session session = ssh_new();

	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, "9393");
	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, "key/id_ed25519");

	if (ssh_bind_listen(sshbind) < 0) {
		std::cerr << "Error listening: " << ssh_get_error(sshbind) << std::endl;
		return 1;
	}

	rc = ssh_bind_accept(sshbind, session);
	if (rc != SSH_OK) {
		std::cerr << "Error accepting: " << ssh_get_error(sshbind) << std::endl;
		return 1;
	}

	if (ssh_handle_key_exchange(session)) {
		std::cerr << "Key exchange failed: " << ssh_get_error(session) << std::endl;
		return 1;
	}

	ssh_message message;
	bool authenticated = false;
	while ((message = ssh_message_get(session)) != nullptr) {
		if (ssh_message_type(message) == SSH_REQUEST_AUTH) {
			if (ssh_message_subtype(message) == SSH_AUTH_METHOD_PUBLICKEY) {
				int state = ssh_message_auth_publickey_state(message);
				ssh_key pubkey = ssh_message_auth_pubkey(message);

				if (state == SSH_PUBLICKEY_STATE_NONE) {
					// Key probe: client asks if this key is acceptable
					if (key_is_authorized(pubkey)) {
						ssh_message_auth_reply_pk_ok_simple(message);
						ssh_message_free(message);
						continue;
					}
				} else if (state == SSH_PUBLICKEY_STATE_VALID) {
					// Signature verified by libssh, grant access if key is authorized
					if (key_is_authorized(pubkey)) {
						ssh_message_auth_reply_success(message, 0);
						authenticated = true;
						ssh_message_free(message);
						break;
					}
				}
			}
			// Reject: advertise only pubkey auth
			ssh_message_auth_set_methods(message, SSH_AUTH_METHOD_PUBLICKEY);
		}
		ssh_message_reply_default(message);
		ssh_message_free(message);
	}

	if (!authenticated) {
		std::cerr << "Authentication failed." << std::endl;
		ssh_disconnect(session);
		ssh_bind_free(sshbind);
		ssh_finalize();
		return 1;
	}

	ssh_channel chan = nullptr;
	while ((message = ssh_message_get(session)) != nullptr) {
		if (ssh_message_type(message) == SSH_REQUEST_CHANNEL_OPEN &&
			ssh_message_subtype(message) == SSH_CHANNEL_SESSION) {
			chan = ssh_message_channel_request_open_reply_accept(message);
			ssh_message_free(message);
			break;
		}
		ssh_message_reply_default(message);
		ssh_message_free(message);
	}

	if (chan == nullptr) {
		std::cerr << "Failed to open channel." << std::endl;
		ssh_disconnect(session);
		ssh_free(session);
		ssh_bind_free(sshbind);
		ssh_finalize();
		return 1;
	}

	// Wait for shell request
	bool shell = false;
	while ((message = ssh_message_get(session)) != nullptr) {
		if (ssh_message_type(message) == SSH_REQUEST_CHANNEL &&
			ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SHELL) {
			shell = true;
			ssh_message_channel_request_reply_success(message);
			ssh_message_free(message);
			break;
		}
		ssh_message_reply_default(message);
		ssh_message_free(message);
	}

	if (!shell) {
		std::cerr << "Failed to get shell request." << std::endl;
		ssh_channel_free(chan);
		ssh_disconnect(session);
		ssh_free(session);
		ssh_bind_free(sshbind);
		ssh_finalize();
		return 1;
	}

	ssh_channel_write(chan, SECRET, strlen(SECRET));
	ssh_channel_send_eof(chan);
	ssh_channel_close(chan);
	ssh_channel_free(chan);

	ssh_disconnect(session);
	ssh_free(session);
	ssh_bind_free(sshbind);
	ssh_finalize();
	return 0;
}
