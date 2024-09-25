#include "encryption.h"

int decrypt(crypto_params *params) {
    EVP_CIPHER_CTX *ctx;
    int len;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;

    // Initialize decryption operation (AES-256-CBC)
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, params->key, params->iv) != 1) return -1;

    // Decrypt the ciphertext
    if (EVP_DecryptUpdate(ctx, params->output, &len, params->input, params->input_len) != 1) return -1;
    params->output_len = len;

    // Finalize decryption
    if (EVP_DecryptFinal_ex(ctx, params->output + len, &len) != 1) return -1;
    params->output_len += len;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return params->output_len;
}

void decrypt_files(const char *key_hex) {
    crypto_params params;
    params.key = (unsigned char *)malloc(KEY_SIZE);
    params.iv = (unsigned char *)malloc(IV_SIZE);
    params.input = (unsigned char *)malloc(1024);
    params.output = (unsigned char *)malloc(1024);
    params.input_len = 0;
    params.output_len = 0;

    // Convert hex key to binary
    hex_to_bin(key_hex, params.key, KEY_SIZE);

    // Get the infection directory
    char path[256];
    snprintf(path, sizeof(path), "%s%s", HOME, INFECTION_DIR);
    DIR *dir = opendir(path);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
            if (strstr(filepath, ".ft")) {
                FILE *file = fopen(filepath, "rb");
                if (file) {
                    fseek(file, 0, SEEK_END);
                    long file_len = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    unsigned char *ciphertext = malloc(file_len);
                    fread(ciphertext, 1, file_len, file);

                    fclose(file);

                    // Decrypt the file
                    memcpy(params.input, ciphertext, file_len);
                    params.input_len = file_len;
                    if (decrypt(&params) != -1) {
                        // Write the decrypted file
                        char *filename = strrchr(filepath, '/');
                        if (filename) {
                            filename++;
                        } else {
                            filename = filepath;
                        }
                        char new_filepath[512];
                        snprintf(new_filepath, sizeof(new_filepath), "%s/%s", path, filename);
                        FILE *new_file = fopen(new_filepath, "wb");
                        if (new_file) {
                            fwrite(params.output, 1, params.output_len, new_file);
                            fclose(new_file);
                        }
                    }
                }
            }
        }
    }
    closedir(dir);
}