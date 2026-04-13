# Stockholm

## Description
Stockholm is an educational ransomware program for understanding how malware operates.

## Encryption Justification
This project utilizes the **OpenSSL** library for encryption and decryption. OpenSSL is a robust, widely-used, and well-tested toolkit that provides comprehensive cryptographic functions. It was chosen for its reliability, broad support across different platforms, and efficiency in handling AES encryption, which is standard for secure data encryption.

## Compilation
```bash
make
```

## Usage
```bash
./stockholm [options]
```

## Options
•	-h, --help: Show help message
•	-v, --version: Show program version
•	-r, --reverse <key>: Reverse the encryption with the provided key
•	-s, --silent: Silent mode
