#pragma once

#include <pcap.h>
#include <csignal>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

void handleSignal(int sig);
void startArpPoisoning(const char *ipSrc, const char *macSrc, const char *ipTarget, const char *macTarget);
void stopArpPoisoning();
void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet);