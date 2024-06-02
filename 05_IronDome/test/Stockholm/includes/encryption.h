#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <dirent.h>
#include "utils.h"

#define KEY_SIZE 16
#define IV_SIZE 16

void handle_errors();
void encrypt_file(const char *filename, const unsigned char *key, const unsigned char *iv);
void decrypt_file(const char *filename, const unsigned char *key, const unsigned char *iv);

void encrypt_files();
void decrypt_files(const char *key_hex);

#endif // ENCRYPTION_H
