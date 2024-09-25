#include "encryption.h"

void show_help() {
    printf("Usage: stockholm [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -v, --version      Show version\n");
    printf("  -r, --reverse KEY  Reverse encryption using the provided key\n");
    printf("  -s, --silent       Silent mode, no output\n");
}

void show_version() {
    printf("Stockholm version 1.0\n");
}

int main(int argc, char *argv[]) {
    int silent_mode = 0;
    char *key = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version();
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reverse") == 0) {
            if (i + 1 < argc) {
                key = argv[++i];  // Capture the key argument
            } else {
                fprintf(stderr, "Error: --reverse option requires a key\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            silent_mode = 1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    // If key is provided, proceed to decryption
    if (key) {
        // Decrypt files using the provided key
        if (!silent_mode) {
            printf("Decrypting with key: %s\n", key);
        }
        decrypt_files(key);
    } else {
        // If no key, proceed to encrypt
        if (!silent_mode) {
            printf("Encrypting files...\n");
        }
        encrypt_files();
    }

    return 0;
}