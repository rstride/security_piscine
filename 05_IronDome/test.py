import os
import time
import subprocess
import threading

# Paths
test_dir = os.path.expanduser('~/irondome_test')
test_file = os.path.join(test_dir, 'test_file.txt')

# Create test directory and file if they don't exist
os.makedirs(test_dir, exist_ok=True)
with open(test_file, 'w') as f:
    f.write("Initial data\n")

# Function to simulate read abuse
def simulate_read_abuse():
    while True:
        with open(test_file, 'r') as f:
            _ = f.read()
        time.sleep(0.01)

# Function to simulate cryptographic activity
def simulate_crypto_activity():
    for _ in range(10):
        subprocess.run(['openssl', 'enc', '-aes-256-cbc', '-pass', 'pass:mysecretpassword', '-in', test_file, '-out', test_file + '.enc'])

# Function to simulate entropy change
def simulate_entropy_change():
    with open(test_file, 'a') as f:
        f.write("More random data\n")

# Run tests
if __name__ == "__main__":
    # Start read abuse in a separate thread
    read_abuse_thread = threading.Thread(target=simulate_read_abuse)
    read_abuse_thread.start()

    # Simulate cryptographic activity
    simulate_crypto_activity()

    # Simulate entropy change
    simulate_entropy_change()

    # Give some time for daemon to log
    time.sleep(10)

    # Stop the read abuse thread
    read_abuse_thread.do_run = False
    read_abuse_thread.join()