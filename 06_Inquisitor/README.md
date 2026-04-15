# Inquisitor

An ARP poisoning MITM tool written in C++17 that intercepts FTP traffic between two hosts in real time.

---

## How it works

ARP poisoning tricks two hosts into believing the attacker's MAC address belongs to the other host. Once both ARP caches are poisoned, all traffic between them flows through the attacker's machine instead of directly peer-to-peer. With IP forwarding enabled, the attacker relays the traffic transparently while inspecting it with libpcap.

```
Before poisoning:
  FTP client ──────────────────► FTP server

After poisoning:
  FTP client ──► inquisitor ──► FTP server
                 (reads all FTP commands in real time)
```

Inquisitor performs the attack in **full duplex**: it sends forged ARP replies to both the source and the target every 2 seconds. When stopped with `CTRL+C`, the original ARP entries are restored by sending corrective replies to both hosts.

---

## Requirements

- Docker (with Compose v2)
- Linux kernel capabilities `NET_ADMIN` and `NET_RAW` (handled automatically by `docker-compose.yaml`)

---

## Project structure

```
06_Inquisitor/
├── Dockerfile              # Builds the inquisitor binary inside Debian bookworm-slim
├── docker-compose.yaml     # Defines three containers: ftp-server, ftp-client, inquisitor
├── Makefile                # make → starts environment; make build → compile only
├── includes/
│   └── inquisitor.hpp      # All headers and includes
├── src/
│   └── main.cpp            # ARP poisoning, pcap sniffing, signal handling
└── test/
    ├── ftp_server.py       # pyftpdlib FTP server (pre-populated with test files)
    └── test.sh             # Automated end-to-end test script
```

---

## Network layout

| Container     | IP            | MAC (assigned at runtime)      | Role          |
|---------------|---------------|--------------------------------|---------------|
| `ftp-server`  | 172.20.0.10   | dynamic                        | FTP server    |
| `ftp-client`  | 172.20.0.20   | dynamic                        | FTP client    |
| `inquisitor`  | 172.20.0.30   | dynamic                        | MITM attacker |

All three containers share the `inquisitor_net` bridge network (`172.20.0.0/24`).

---

## Usage

### 1. Start the environment

```bash
make
# equivalent to:
docker compose up --build
```

This builds the inquisitor image, starts the FTP server (pyftpdlib), and brings up all three containers. Use `-d` to detach:

```bash
docker compose up --build -d
```

### 2. Run the automated test

```bash
# Normal mode — shows intercepted filenames only
bash test/test.sh

# Verbose mode — shows all FTP control traffic (bonus)
bash test/test.sh -v
```

The script:
1. Checks all containers are running
2. Waits for the FTP server to accept connections
3. Populates ARP caches via ping
4. Retrieves the MAC addresses of `ftp-server` and `ftp-client`
5. Starts `inquisitor` in the background
6. Waits until ARP poisoning is active
7. Runs an FTP session from `ftp-client` (upload, list, two downloads)
8. Stops `inquisitor` and prints the captured traffic

### 3. Run inquisitor manually

Get the MAC addresses:

```bash
docker inspect ftp-client --format '{{range .NetworkSettings.Networks}}{{.MacAddress}}{{end}}'
docker inspect ftp-server --format '{{range .NetworkSettings.Networks}}{{.MacAddress}}{{end}}'
```

Then launch the attack (replace MACs with the values above):

```bash
docker exec -it inquisitor ./build/inquisitor \
    172.20.0.20 <CLIENT_MAC> \
    172.20.0.10 <SERVER_MAC>
```

With verbose mode:

```bash
docker exec -it inquisitor ./build/inquisitor -v \
    172.20.0.20 <CLIENT_MAC> \
    172.20.0.10 <SERVER_MAC>
```

Press `CTRL+C` to stop. Inquisitor will restore the ARP tables before exiting.

---

## Program arguments

```
inquisitor [-v] <IP-src> <MAC-src> <IP-target> <MAC-target>
```

| Argument       | Description                                 |
|----------------|---------------------------------------------|
| `IP-src`       | IPv4 address of the source host (client)    |
| `MAC-src`      | MAC address of the source host              |
| `IP-target`    | IPv4 address of the target host (server)    |
| `MAC-target`   | MAC address of the target host              |
| `-v`           | Verbose mode (optional, position-independent) |

MACs are accepted in both `aa:bb:cc:dd:ee:ff` and `aa-bb-cc-dd-ee-ff` format.

All input errors (wrong IPv4, malformed MAC, wrong number of arguments) cause an immediate exit with a descriptive message and return code 1.

---

## Output

### Normal mode

Only FTP commands that transfer or name files are printed:

```
[*] Interface : eth0  (e2:1a:8a:20:f4:59)
[*] Source    : 172.20.0.20  da:6a:e5:1b:53:84
[*] Target    : 172.20.0.10  ee:a1:ee:56:27:56
[*] ARP poisoning running — CTRL+C to stop
[*] Sniffing FTP traffic between 172.20.0.20 and 172.20.0.10...
[FTP] 172.20.0.20 -> 172.20.0.10  |  STOR intercepted_upload.txt
[FTP] 172.20.0.20 -> 172.20.0.10  |  LIST
[FTP] 172.20.0.20 -> 172.20.0.10  |  RETR welcome.txt
[FTP] 172.20.0.20 -> 172.20.0.10  |  RETR secret.txt
```

Intercepted commands: `STOR`, `RETR`, `LIST`, `NLST`, `RNFR`, `RNTO`.

### Verbose mode (`-v`)

Every FTP control message in both directions is shown, including the login sequence:

```
[FTP] 172.20.0.10:21 -> 172.20.0.20:PORT  |  220 inquisitor-test-ftpd ready
[FTP] 172.20.0.20:PORT -> 172.20.0.10:21  |  USER user
[FTP] 172.20.0.10:21 -> 172.20.0.20:PORT  |  331 Username ok, send password.
[FTP] 172.20.0.20:PORT -> 172.20.0.10:21  |  PASS pass
[FTP] 172.20.0.10:21 -> 172.20.0.20:PORT  |  230 Login successful.
...
[FTP] 172.20.0.20:PORT -> 172.20.0.10:21  |  STOR intercepted_upload.txt
[FTP] 172.20.0.20:PORT -> 172.20.0.10:21  |  RETR secret.txt
...
[FTP] 172.20.0.20:PORT -> 172.20.0.10:21  |  QUIT
[FTP] 172.20.0.10:21 -> 172.20.0.20:PORT  |  221 Goodbye.
```

### On CTRL+C

```
[!] Caught SIGINT, stopping...
[*] Restoring ARP tables...
[*] ARP tables restored.
```

---

## Makefile targets

| Target        | Description                                          |
|---------------|------------------------------------------------------|
| `make`        | Build images and start the full Docker environment   |
| `make build`  | Compile the binary only (runs inside the container)  |
| `make stop`   | Stop containers without removing build artifacts     |
| `make clean`  | Stop containers and delete the `build/` directory    |

---

## Implementation notes

- **ARP poisoning** is done with raw `AF_PACKET` sockets. The program sends forged ARP reply frames (opcode 2) every 2 seconds in both directions, making each host cache the attacker's MAC for the other's IP.
- **IP forwarding** is enabled at startup by writing to `/proc/sys/net/ipv4/ip_forward`, so relayed traffic reaches its real destination.
- **Packet capture** uses libpcap with a BPF filter restricting capture to TCP port 21 traffic between the two hosts. This keeps CPU usage low and output clean.
- **Deduplication**: in a MITM setup, each packet is seen twice on the attacker's interface (once inbound, once after IP forwarding). Inquisitor deduplicates by tracking TCP sequence numbers per flow.
- **Signal safety**: the `SIGINT` handler uses only `write()` (async-signal-safe) and `pcap_breakloop()` to exit the capture loop cleanly, then the main thread restores ARP before exiting.
