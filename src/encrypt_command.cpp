#include "encrypt_command.hpp"

#include <fstream>
#include <iostream>
#include <string>

#include "crypto.hpp"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace drop {

namespace {

std::string read_hidden(const char* prompt)
{
	std::cerr << prompt;

#ifdef _WIN32
	std::string result;
	for (;;) {
		int ch = _getch();
		if (ch == '\r' || ch == '\n')
			break;
		if (ch == '\b' || ch == 127) {
			if (!result.empty())
				result.pop_back();
			continue;
		}
		result += static_cast<char>(ch);
	}
	std::cerr << '\n';
	return result;
#else
	struct termios old_term {};
	struct termios new_term {};
	tcgetattr(STDIN_FILENO, &old_term);
	new_term = old_term;
	new_term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

	std::string result;
	std::getline(std::cin, result);

	tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
	std::cerr << '\n';
	return result;
#endif
}

} // namespace

int run_encrypt(const char* output_path)
{
	std::string pass1 = read_hidden("Passphrase: ");
	std::string pass2 = read_hidden("Confirm passphrase: ");

	if (pass1 != pass2) {
		std::cerr << "Passphrases do not match\n";
		return 1;
	}
	if (pass1.empty()) {
		std::cerr << "Passphrase must not be empty\n";
		return 1;
	}

	std::cerr << "Secret (single line): ";
	std::string secret;
	std::getline(std::cin, secret);

	if (secret.empty()) {
		std::cerr << "Secret must not be empty\n";
		return 1;
	}

	auto encrypted = crypto::encrypt(secret, pass1);

	std::ofstream out(output_path);
	if (!out.is_open()) {
		std::cerr << "Could not open output file: " << output_path
			  << '\n';
		return 1;
	}

	out << encrypted;

	std::cerr << "Encrypted secret written to " << output_path << '\n';
	return 0;
}

int run_decrypt(const char* input_path)
{
	std::ifstream in(input_path);
	if (!in.is_open()) {
		std::cerr << "Could not open input file: " << input_path
			  << '\n';
		return 1;
	}

	std::string data_b64(
		(std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());

	if (data_b64.empty()) {
		std::cerr << "Input file is empty\n";
		return 1;
	}

	std::string passphrase = read_hidden("Passphrase: ");
	if (passphrase.empty()) {
		std::cerr << "Passphrase must not be empty\n";
		return 1;
	}

	auto result = crypto::decrypt(data_b64, passphrase);
	if (!result) {
		std::cout << "Wrong passphrase\n";
		return 1;
	}
	std::cout << *result;

	return 0;
}

} // namespace drop
