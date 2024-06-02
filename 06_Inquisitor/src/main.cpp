#include "inquisitor.hpp"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <IP-src> <MAC-src> <IP-target> <MAC-target>" << std::endl;
        return 1;
    }

    // Signal handling
    std::signal(SIGINT, handleSignal);

    startArpPoisoning(argv[1], argv[2], argv[3], argv[4]);

    // Capture packets and handle them
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);
    if (handle == nullptr) {
        std::cerr << "Could not open device: " << errbuf << std::endl;
        return 2;
    }

    pcap_loop(handle, 0, packetHandler, nullptr);
    pcap_close(handle);

    return 0;
}

void handleSignal(int sig) {
    if (sig == SIGINT) {
        std::cout << "Caught SIGINT, cleaning up..." << std::endl;
        stopArpPoisoning();
        std::exit(0);
    }
}

void startArpPoisoning(const char *ipSrc, const char *macSrc, const char *ipTarget, const char *macTarget) {
    // Implementation of ARP poisoning
    std::cout << "Starting ARP poisoning..." << std::endl;
}

void stopArpPoisoning() {
    // Implementation to restore ARP tables
    std::cout << "Stopping ARP poisoning and restoring ARP tables..." << std::endl;
}

void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    // Implementation of packet handling to extract FTP filenames
    std::cout << "Packet captured" << std::endl;
}