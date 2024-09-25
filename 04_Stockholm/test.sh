#!/bin/bash

# Enable strict error handling
set -euo pipefail

# Variables
INFECT_DIR="$HOME/infection"
STOCKHOLM_BINARY="./stockholm"
encryption_key=""

# Function to set up the test environment
setup_test_env() {
    echo "Setting up test environment..."
    rm -rf "$INFECT_DIR"
    mkdir -p "$INFECT_DIR"
    echo "Test file 1" > "$INFECT_DIR/test1.txt"
    echo "Test file 2" > "$INFECT_DIR/test2.doc"
    echo "Test file 3" > "$INFECT_DIR/test3.jpg"
    echo "Test files created in $INFECT_DIR:"
    ls -l "$INFECT_DIR"
    echo ""
}

# Function to run the encryption test
run_encryption_test() {
    echo "Running encryption test..."
    # Run the Stockholm binary and capture both stdout and stderr
    encryption_output=$("$STOCKHOLM_BINARY" 2>&1)
    encryption_exit_code=$?

    if [[ $encryption_exit_code -ne 0 ]]; then
        echo "Error: Encryption failed to run."
        echo "Stockholm output:"
        echo "$encryption_output"
        exit 1
    fi

    # Extract the encryption key from the output
    encryption_key=$(echo "$encryption_output" | grep -Eo 'Encryption Key: [A-Fa-f0-9]+' | awk '{print $3}')
    if [[ -z "$encryption_key" ]]; then
        echo "Error: Failed to retrieve encryption key."
        echo "Stockholm output:"
        echo "$encryption_output"
        exit 1
    fi
    echo "Encryption key: $encryption_key"
    echo ""

    # Check if encryption was successful
    encrypted_files=("$INFECT_DIR"/*.ft)
    if [[ -e "${encrypted_files[0]}" ]]; then
        echo "Encryption successful. Encrypted files:"
        ls -l "${encrypted_files[@]}"
    else
        echo "Error: No encrypted files found."
        exit 1
    fi
    echo ""
}

# Function to run the decryption test
run_decryption_test() {
    echo "Running decryption test..."
    # Run the Stockholm binary to decrypt using the captured key
    echo "Decrypting with key: $encryption_key"
    decryption_output=$("$STOCKHOLM_BINARY" --reverse "$encryption_key" 2>&1)
    echo "Decryption output:"
    decryption_exit_code=$?
    echo "Decryption output:"

    if [[ $decryption_exit_code -ne 0 ]]; then
        echo "Error: Decryption failed to run."
        echo "Stockholm output:"
        echo "$decryption_output"
        exit 1
    fi

    # Check if decryption was successful
    decrypted_files=("$INFECT_DIR"/*)
    if [[ -e "${decrypted_files[0]}" && ! -e "${INFECT_DIR}/test1.txt.ft" ]]; then
        echo "Decryption successful. Decrypted files:"
        ls -l "${decrypted_files[@]}"
    else
        echo "Error: Decryption failed or encrypted files still exist."
        exit 1
    fi
    echo ""
}

# Function to clean up the test environment
clean_test_env() {
    echo "Cleaning up test environment..."
    rm -rf "$INFECT_DIR"
    echo "Cleanup complete."
}

# Main execution flow
main() {
    # Ensure the Stockholm binary exists and is executable
    if [[ ! -x "$STOCKHOLM_BINARY" ]]; then
        echo "Error: Stockholm binary not found or not executable at $STOCKHOLM_BINARY"
        exit 1
    fi

    setup_test_env
    run_encryption_test
    run_decryption_test
    clean_test_env
}

# Run the main function
main