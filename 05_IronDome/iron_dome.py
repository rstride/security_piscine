import os
import sys
import time
import logging
import psutil
import pyinotify
from scipy.stats import entropy
from daemon import DaemonContext

log_directory = '/var/log/irondome'
if not os.path.exists(log_directory):
    os.makedirs(log_directory)

# Setup logging
logging.basicConfig(filename='/var/log/irondome/irondome.log', level=logging.INFO)

def monitor_filesystem(paths):
    wm = pyinotify.WatchManager()
    mask = pyinotify.IN_ACCESS | pyinotify.IN_MODIFY | pyinotify.IN_CREATE | pyinotify.IN_DELETE

    class EventHandler(pyinotify.ProcessEvent):
        def process_IN_ACCESS(self, event):
            logging.info(f"File accessed: {event.pathname}")
            # Add logic to detect read abuse
        def process_IN_MODIFY(self, event):
            logging.info(f"File modified: {event.pathname}")
        def process_IN_CREATE(self, event):
            logging.info(f"File created: {event.pathname}")
        def process_IN_DELETE(self, event):
            logging.info(f"File deleted: {event.pathname}")

    handler = EventHandler()
    notifier = pyinotify.Notifier(wm, handler)
    for path in paths:
        wm.add_watch(path, mask, rec=True)
    notifier.loop()

def monitor_crypto_activity():
    for proc in psutil.process_iter(attrs=['pid', 'name', 'cpu_percent']):
        if 'crypto' in proc.info['name'].lower():
            if proc.info['cpu_percent'] > 80:
                logging.warning(f"High CPU usage by cryptographic process: {proc.info['name']} (PID: {proc.info['pid']})")

def calculate_entropy(data):
    p_data = pd.value_counts(data) / len(data) 
    entropy_value = entropy(p_data)
    return entropy_value

def monitor_entropy(path):
    try:
        with open(path, 'rb') as file:
            data = file.read()
            current_entropy = calculate_entropy(data)
            logging.info(f"Entropy of {path}: {current_entropy}")
    except Exception as e:
        logging.error(f"Error calculating entropy for {path}: {e}")

def main():
    paths_to_monitor = sys.argv[1:] if len(sys.argv) > 1 else ['/default/path']
    
    while True:
        monitor_filesystem(paths_to_monitor)
        monitor_crypto_activity()
        for path in paths_to_monitor:
            monitor_entropy(path)
        time.sleep(10)  # Adjust sleep time as needed

if __name__ == "__main__":
    with DaemonContext():
        main()