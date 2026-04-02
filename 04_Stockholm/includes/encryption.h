#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <sys/stat.h>
#include <dirent.h>

#define HOME getenv("HOME")
#define INFECTION_DIR "/infection"
#define KEY_SIZE 32
#define IV_SIZE 16

extern const char *wannacry_extensions[];
extern const int num_wannacry_extensions;

typedef struct {
    unsigned char *key;         // Encryption key
    unsigned char iv[16];          // Initialization vector
    unsigned char *input;       // Plaintext or Ciphertext
    unsigned char *output;      // Ciphertext or Plaintext (depending on operation)
    int input_len;              // Length of input data
    int output_len;             // Length of output data
} crypto_params;

// Encrypt
int     stockholm_encrypt(crypto_params *params);
void    encrypt_files(int silent_mode);

// Decrypt
int     stockholm_decrypt(crypto_params *params);
void    decrypt_files(const char *key_hex, int silent_mode);

// Utils
void    setup_infection_directory();
void    rename_file(const char *filename);
void    handle_errors();
void    hex_to_bin(const char *hex, unsigned char *bin, size_t len);

#endif // ENCRYPTION_H
