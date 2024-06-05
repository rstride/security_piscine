#include "irondome.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <directory>" << std::endl;
        return EXIT_FAILURE;
    }

    if (getuid() != 0) {
        std::cerr << "You must be root to run this program." << std::endl;
        return EXIT_FAILURE;
    }

    daemonize();

    log_message("IronDome daemon started.");

    while (true) {
        monitor_directory(argv[1]);
        sleep(10);
    }

    return 0;
}