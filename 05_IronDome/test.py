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
def simulate_read_abuse(stop_event):
    while not stop_event.is_set():
        with open(test_file, 'r') as f:
            _ = f.read()
        time.sleep(0.01)
    print("Read abuse simulation stopped")

# Function to simulate cryptographic activity
def simulate_crypto_activity():
    print("Starting cryptographic activity simulation...")
    for i in range(10):
        subprocess.run(['openssl', 'enc', '-aes-256-cbc', '-pass', 'pass:mysecretpassword', '-in', test_file, '-out', test_file + '.enc'])
        print(f"Cryptographic activity iteration {i+1} complete")
    print("Cryptographic activity simulation finished")

# Function to simulate entropy change
def simulate_entropy_change():
    print("Starting entropy change simulation...")
    with open(test_file, 'a') as f:
        f.write("More random data\n")
    print("Entropy change simulation finished")

# Run tests
if __name__ == "__main__":
    stop_event = threading.Event()
    read_abuse_thread = threading.Thread(target=simulate_read_abuse, args=(stop_event,))
    
    try:
        # Start read abuse in a separate thread
        read_abuse_thread.start()
        
        # Simulate cryptographic activity
        simulate_crypto_activity()
        
        # Simulate entropy change
        simulate_entropy_change()
        
        # Give some time for daemon to log
        time.sleep(10)
    
    finally:
        # Signal the read abuse thread to stop
        stop_event.set()
        read_abuse_thread.join()
        print("Test script completed")