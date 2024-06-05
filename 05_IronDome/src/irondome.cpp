#include "irondome.hpp"

void log_message(const std::string &message) {
    std::ofstream log(LOG_FILE, std::ios::app);
    if (!logfile) {
        std::cerr << "Error opening log file: "<< std::endl;
        exit(EXIT_FAILURE);
    }
    std::time_t now = std::time(nullptr);
    logfile << std::ctime(&now) << ": " << message << std::endl;
}

void monitor_directory(const std::string &path) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(path.c_str()))) {
        log_message("Error opening directory: " + path);
        return;
    }

    while ((entry = readdir(dir)) != nullptr) {
        struct stat st;
        std::string full_path = path + "/" + entry->d_name;
        
        if (stat(full_path.c_str(), &st) == -1) {
            log_message("Error stating file: " + full_path);
            continue;
        }
    }
    closedir(dir);
}

void daemonize() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}