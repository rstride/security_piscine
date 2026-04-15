#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# Inquisitor — automated test script
#
# Prerequisites:
#   docker compose up --build -d   (from the project root)
#
# Usage:
#   bash test/test.sh              normal mode  (filenames only)
#   bash test/test.sh -v           verbose mode (all FTP traffic)
# ─────────────────────────────────────────────────────────────────────────────

set -e

SERVER_IP="172.20.0.10"
CLIENT_IP="172.20.0.20"

VERBOSE_FLAG=""
[[ "$1" == "-v" ]] && VERBOSE_FLAG="-v"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${BLUE}[*]${NC} $*"; }
success() { echo -e "${GREEN}[+]${NC} $*"; }
warn()    { echo -e "${YELLOW}[!]${NC} $*"; }
error()   { echo -e "${RED}[-]${NC} $*" >&2; }
header()  { echo -e "\n${BOLD}$*${NC}"; }

# ─── Check containers are running ─────────────────────────────────────────────
header "=== Inquisitor Test Suite ==="

for c in ftp-server ftp-client inquisitor; do
    if ! docker ps --format '{{.Names}}' | grep -q "^${c}$"; then
        error "Container '${c}' is not running."
        error "Start the environment first:  docker compose up --build -d"
        exit 1
    fi
done
success "All containers running"

# ─── Wait for FTP server to be ready ─────────────────────────────────────────
info "Waiting for FTP server (port 21)..."
for i in $(seq 1 20); do
    if docker exec ftp-server python3 -c \
        "import socket; s=socket.socket(); s.settimeout(1); s.connect(('127.0.0.1',21)); s.close()" \
        2>/dev/null; then
        success "FTP server is up"
        break
    fi
    sleep 1
    if [[ "$i" -eq 20 ]]; then
        warn "FTP server not responding — continuing anyway"
    fi
done

# ─── Populate ARP caches ──────────────────────────────────────────────────────
info "Populating ARP caches..."
docker exec inquisitor ping -c 2 -W 1 "$SERVER_IP" > /dev/null 2>&1 || true
docker exec inquisitor ping -c 2 -W 1 "$CLIENT_IP" > /dev/null 2>&1 || true
docker exec ftp-client  ping -c 2 -W 1 "$SERVER_IP" > /dev/null 2>&1 || true

# ─── Retrieve MAC addresses ───────────────────────────────────────────────────
info "Retrieving MAC addresses..."

SERVER_MAC=$(docker inspect ftp-server \
    --format '{{range .NetworkSettings.Networks}}{{.MacAddress}}{{end}}')
CLIENT_MAC=$(docker inspect ftp-client \
    --format '{{range .NetworkSettings.Networks}}{{.MacAddress}}{{end}}')

if [[ -z "$SERVER_MAC" || -z "$CLIENT_MAC" ]]; then
    error "Could not retrieve MAC addresses. Are containers on inquisitor_net?"
    exit 1
fi

success "FTP server : $SERVER_IP  ($SERVER_MAC)"
success "FTP client : $CLIENT_IP  ($CLIENT_MAC)"

# ─── Show manual command for evaluators ───────────────────────────────────────
header "=== Manual inquisitor command ==="
echo ""
echo "  docker exec -it inquisitor ./build/inquisitor ${VERBOSE_FLAG} \\"
echo "      $CLIENT_IP $CLIENT_MAC \\"
echo "      $SERVER_IP $SERVER_MAC"
echo ""

# ─── Start inquisitor in background, capturing output ─────────────────────────
header "=== Starting attack ==="
LOGFILE="/tmp/inquisitor_test.log"

docker exec inquisitor sh -c \
    "./build/inquisitor ${VERBOSE_FLAG} \
        $CLIENT_IP $CLIENT_MAC \
        $SERVER_IP $SERVER_MAC \
     > $LOGFILE 2>&1 &"

info "Inquisitor started (log: $LOGFILE inside container)"
info "Waiting for ARP poisoning to start..."
for i in $(seq 1 10); do
    if docker exec inquisitor grep -q 'Sniffing' "$LOGFILE" 2>/dev/null; then
        success "Inquisitor is sniffing — ARP poisoning active"
        break
    fi
    sleep 1
    if [[ "$i" -eq 10 ]]; then
        warn "Inquisitor did not start sniffing — check log"
    fi
done

# ─── Run FTP test session ─────────────────────────────────────────────────────
header "=== Running FTP session from ftp-client ==="

# Create a local file to upload
docker exec ftp-client sh -c "echo 'intercepted data from client' > /tmp/upload.txt"

# FTP session: login → upload → list → download two files → quit
docker exec ftp-client sh -c "
ftp -nv $SERVER_IP <<'FTP_EOF'
user user pass
put /tmp/upload.txt intercepted_upload.txt
ls
get welcome.txt /tmp/welcome_dl.txt
get secret.txt  /tmp/secret_dl.txt
bye
FTP_EOF
" 2>&1 | sed 's/^/  [ftp] /' || true

sleep 2

# ─── Stop inquisitor and show captured traffic ────────────────────────────────
header "=== Stopping inquisitor ==="
docker exec inquisitor sh -c "pkill -INT inquisitor 2>/dev/null || true"
sleep 3

header "=== Intercepted FTP traffic ==="
docker exec inquisitor cat "$LOGFILE" 2>/dev/null || warn "No log found"

success "Test complete."
echo ""
