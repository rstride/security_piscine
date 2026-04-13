#pragma once

// C++ standard library
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// POSIX / system
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Linux raw socket / ARP
#include <linux/if_packet.h>
#include <net/ethernet.h>

// libpcap
#include <pcap.h>
