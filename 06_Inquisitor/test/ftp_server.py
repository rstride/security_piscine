#!/usr/bin/env python3
"""Simple FTP server for inquisitor testing (uses pyftpdlib)."""

import os
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer
from pyftpdlib.authorizers import DummyAuthorizer

FTP_ROOT = "/tmp/ftp"
FTP_USER = "user"
FTP_PASS = "pass"
FTP_HOST = "0.0.0.0"
FTP_PORT = 21
PASV_PORTS = range(60000, 60010)


def main():
    os.makedirs(FTP_ROOT, exist_ok=True)

    # Pre-populate with files the client can download
    with open(os.path.join(FTP_ROOT, "welcome.txt"), "w") as f:
        f.write("Welcome to the inquisitor test FTP server!\n")
    with open(os.path.join(FTP_ROOT, "secret.txt"), "w") as f:
        f.write("This file transfer will be intercepted.\n")

    auth = DummyAuthorizer()
    auth.add_user(FTP_USER, FTP_PASS, FTP_ROOT, perm="elradfmwMT")
    auth.add_anonymous(FTP_ROOT, perm="elr")

    handler = FTPHandler
    handler.authorizer = auth
    handler.passive_ports = PASV_PORTS
    handler.banner = "inquisitor-test-ftpd ready"

    server = FTPServer((FTP_HOST, FTP_PORT), handler)
    print(f"[ftp-server] listening on {FTP_HOST}:{FTP_PORT}")
    print(f"[ftp-server] root: {FTP_ROOT}  user: {FTP_USER}  pass: {FTP_PASS}")
    server.serve_forever()


if __name__ == "__main__":
    main()
