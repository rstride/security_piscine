#pragma once

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <vector>

const std::string LOG_FILE = "/var/log/irondome.log";