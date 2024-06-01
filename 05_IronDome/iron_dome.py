import os
import sys
import time
import logging
import psutil
import pyinotify
from scipy.stats import entropy
from collections import Counter

# Ensure the log directory exists
log_directory = '/var/log/irondome'
if not os.path.exists(log_directory):
    os.makedirs(log_directory)

# Setup logging
logging.basicConfig(
    filename='/var/log/irondome/irondome.log',
    level=logging.DEBUG,
    format='%(asctime)s %(levelname)s: %(message)s'
)

logging.info('Iron Dome daemon started')

def monitor_filesystem(paths):
    wm = pyinotify.WatchManager()
    mask = pyinotify.IN_ACCESS | pyinotify.IN_MODIFY | pyinotify.IN_CREATE | pyinotify.IN_DELETE

    class EventHandler(pyinotify.ProcessEvent):
        def process_IN_ACCESS(self, event):
            logging.info(f"File accessed: {event.pathname}")
        def process_IN_MODIFY(self, event):
            logging.info(f"File modified: {event.pathname}")
        def process_IN_CREATE(self, event):
            logging.info(f"File created: {event.pathname}")
        def process_IN_DELETE(self, event):
            logging.info(f"File deleted: {event.pathname}")

    handler = EventHandler()
    notifier = pyinotify.Notifier(wm, handler)
    for path in paths:
        logging.debug(f'Adding watch on: {path}')
        wm.add_watch(path, mask, rec=True)
    notifier.loop()

def monitor_crypto_activity():
    logging.debug('Monitoring cryptographic activity...')
    for proc in psutil.process_iter(attrs=['pid', 'name', 'cpu_percent']):
        if 'openssl' in proc.info['name'].lower():
            logging.debug(f'Checking process: {proc.info}')
            if proc.info['cpu_percent'] > 80:
                logging.warning(f"High CPU usage by cryptographic process: {proc.info['name']} (PID: {proc.info['pid']})")

def calculate_entropy(data):
    counts = Counter(data)
    total_count = sum(counts.values())
    probabilities = [count / total_count for count in counts.values()]
    return entropy(probabilities)

def monitor_entropy(path):
    logging.debug(f'Calculating entropy for {path}...')
    if os.path.isdir(path):
        logging.debug(f'Skipping directory {path}')
        return
    try:
        with open(path, 'rb') as file:
            data = file.read()
            current_entropy = calculate_entropy(data)
            logging.info(f"Entropy of {path}: {current_entropy}")
    except Exception as e:
        logging.error(f"Error calculating entropy for {path}: {e}")

def main():
    paths_to_monitor = sys.argv[1:] if len(sys.argv) > 1 else ['/default/path']
    logging.debug(f'Paths to monitor: {paths_to_monitor}')
    
    while True:
        try:
            logging.debug('Starting filesystem monitoring...')
            monitor_filesystem(paths_to_monitor)
            logging.debug('Starting cryptographic activity monitoring...')
            monitor_crypto_activity()
            for path in paths_to_monitor:
                logging.debug(f'Starting entropy monitoring for: {path}')
                monitor_entropy(path)
            time.sleep(10)  # Adjust sleep time as needed
        except Exception as e:
            logging.error(f"Error in main loop: {e}")

if __name__ == "__main__":
    logging.info('Starting Iron Dome daemon')
    main()