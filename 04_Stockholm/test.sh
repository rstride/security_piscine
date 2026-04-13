#!/bin/bash

# Enable strict error handling
set -euo pipefail

# ANSI Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Variables
INFECT_DIR="$HOME/infection"
STOCKHOLM_BINARY="./stockholm"
encryption_key=""

# Function to set up the test environment
setup_test_env() {
    echo -e "${BLUE}${BOLD}⏳ Setting up test environment...${NC}"
    rm -rf "$INFECT_DIR"
    mkdir -p "$INFECT_DIR"
    echo "Test file 1" > "$INFECT_DIR/test1.txt"
    echo "Test file 2" > "$INFECT_DIR/test2.doc"
    echo "Test file 3" > "$INFECT_DIR/test3.jpg"
    echo -e "${CYAN}📁 Test files created in ${INFECT_DIR}:${NC}"
    ls -l "$INFECT_DIR" | awk '{print "   " $0}'
    echo ""
}

# Function to run the encryption test
run_encryption_test() {
    echo -e "${PURPLE}${BOLD}🔒 Running encryption test...${NC}"
    # Run the Stockholm binary and capture both stdout and stderr
    encryption_output=$("$STOCKHOLM_BINARY" 2>&1) || encryption_exit_code=$?
    encryption_exit_code=${encryption_exit_code:-0}

    if [[ $encryption_exit_code -ne 0 ]]; then
        echo -e "${RED}❌ Error: Encryption failed to run.${NC}"
        echo -e "${YELLOW}Stockholm output:${NC}"
        echo "$encryption_output" | awk '{print "   " $0}'
        exit 1
    fi

    # Extract the encryption key from the output
    encryption_key=$(echo "$encryption_output" | grep -Eo 'Encryption Key: [A-Fa-f0-9]+' | awk '{print $3}') || true
    if [[ -z "$encryption_key" ]]; then
        echo -e "${RED}❌ Error: Failed to retrieve encryption key.${NC}"
        echo -e "${YELLOW}Stockholm output:${NC}"
        echo "$encryption_output" | awk '{print "   " $0}'
        exit 1
    fi
    echo -e "${GREEN}🔑 Encryption key captured: ${BOLD}$encryption_key${NC}"
    echo ""

    # Check if encryption was successful
    shopt -s nullglob
    encrypted_files=("$INFECT_DIR"/*.ft)
    shopt -u nullglob
    
    if [[ ${#encrypted_files[@]} -gt 0 && -e "${encrypted_files[0]}" ]]; then
        echo -e "${GREEN}✅ Encryption successful. Encrypted files:${NC}"
        ls -l "${encrypted_files[@]}" | awk '{print "   " $0}'
    else
        echo -e "${RED}❌ Error: No encrypted files found.${NC}"
        exit 1
    fi
    echo ""
}

# Function to run the decryption test
run_decryption_test() {
    echo -e "${CYAN}${BOLD}🔓 Running decryption test...${NC}"
    # Run the Stockholm binary to decrypt using the captured key
    echo -e "${BLUE}💎 Decrypting with key: ${BOLD}$encryption_key${NC}"
    decryption_output=$("$STOCKHOLM_BINARY" --reverse "$encryption_key" 2>&1) || decryption_exit_code=$?
    decryption_exit_code=${decryption_exit_code:-0}

    if [[ $decryption_exit_code -ne 0 ]]; then
        echo -e "${RED}❌ Error: Decryption failed to run.${NC}"
        echo -e "${YELLOW}Stockholm output:${NC}"
        echo "$decryption_output" | awk '{print "   " $0}'
        exit 1
    fi

    # Check if decryption was successful
    shopt -s nullglob
    decrypted_files=("$INFECT_DIR"/*)
    shopt -u nullglob
    
    if [[ ${#decrypted_files[@]} -gt 0 && -e "${decrypted_files[0]}" && ! -e "${INFECT_DIR}/test1.txt.ft" ]]; then
        echo -e "${GREEN}✅ Decryption successful. Decrypted files:${NC}"
        ls -l "${decrypted_files[@]}" | awk '{print "   " $0}'
    else
        echo -e "${RED}❌ Error: Decryption failed or encrypted files still exist.${NC}"
        exit 1
    fi
    echo ""
}

# Function to clean up the test environment
clean_test_env() {
    echo -e "${YELLOW}${BOLD}🧹 Cleaning up test environment...${NC}"
    rm -rf "$INFECT_DIR"
    echo -e "${GREEN}✅ Cleanup complete. All tests passed! 🎉${NC}"
}

# Main execution flow
main() {
    # Ensure the Stockholm binary exists and is executable
    if [[ ! -x "$STOCKHOLM_BINARY" ]]; then
        echo -e "${RED}❌ Error: Stockholm binary not found or not executable at $STOCKHOLM_BINARY${NC}"
        exit 1
    fi

    echo -e "${BOLD}================================================${NC}"
    echo -e "${BOLD}${CYAN}        STOCKHOLM RANSOMWARE TEST SUITE         ${NC}"
    echo -e "${BOLD}================================================${NC}\n"

    setup_test_env
    run_encryption_test
    run_decryption_test
    clean_test_env
}

# Run the main function
main