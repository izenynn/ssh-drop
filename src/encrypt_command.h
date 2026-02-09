#ifndef SSH_DROP_ENCRYPT_COMMAND_H_
#define SSH_DROP_ENCRYPT_COMMAND_H_

namespace drop {

int run_encrypt(const char* output_path);
int run_decrypt(const char* input_path);

} // namespace drop

#endif // SSH_DROP_ENCRYPT_COMMAND_H_
