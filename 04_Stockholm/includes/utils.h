#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#define HOME getenv("HOME")
#define INFECTION_DIR "/infection"

void setup_infection_directory();
void rename_file(const char *filename);

#endif // UTILS_H
