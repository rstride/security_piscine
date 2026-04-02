#include "encryption.h"

int stockholm_decrypt(crypto_params *params) {
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

void decrypt_files(const char *key_hex, int silent_mode) {
    crypto_params params;
    params.key = (unsigned char *)malloc(KEY_SIZE);
    unsigned char iv[IV_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    memcpy(params.iv, iv, IV_SIZE);
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
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
            
            // Only decrypt .ft files
            if (strstr(entry->d_name, ".ft")) {
                FILE *file = fopen(filepath, "rb");
                if (file) {
                    fseek(file, 0, SEEK_END);
                    long file_len = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    if (file_len > IV_SIZE) {
                        // Read IV first
                        fread(params.iv, 1, IV_SIZE, file);

                        long ciphertext_len = file_len - IV_SIZE;
                        unsigned char *ciphertext = malloc(ciphertext_len);
                        fread(ciphertext, 1, ciphertext_len, file);
                        fclose(file);

                        // Decrypt the file
                        params.input = ciphertext;
                        params.input_len = ciphertext_len;
                        if (stockholm_decrypt(&params) != -1) {
                            // Strip .ft from filepath
                            char original_filepath[512];
                            strncpy(original_filepath, filepath, sizeof(original_filepath) - 1);
                            original_filepath[sizeof(original_filepath) - 1] = '\0';
                            
                            char *ft_ext = strstr(original_filepath, ".ft");
                            if (ft_ext) {
                                *ft_ext = '\0';
                            }
                            
                            FILE *new_file = fopen(original_filepath, "wb");
                            if (new_file) {
                                fwrite(params.output, 1, params.output_len, new_file);
                                fclose(new_file);
                                
                                // Delete the encrypted file
                                remove(filepath);
                                
                                if (!silent_mode) {
                                    printf("%s\n", original_filepath);
                                }
                            }
                        }
                        free(ciphertext);
                    } else {
                        fclose(file);
                    }
                }
            }
        }
    }
    closedir(dir);

    free(params.key);
    free(params.iv);
    free(params.input);
    free(params.output);
}