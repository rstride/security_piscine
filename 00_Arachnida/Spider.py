import requests
from bs4 import BeautifulSoup
import os
import sys
import argparse
from urllib.parse import urljoin, urlparse

def download_image(url, path):
    try:
        response = requests.get(url, stream=True)
        if response.status_code == 200:
            filename = os.path.join(path, os.path.basename(urlparse(url).path))
            with open(filename, 'wb') as f:
                for chunk in response.iter_content(1024):
                    f.write(chunk)
            print(f"Downloaded: {url}")
        else:
            print(f"Failed to download: {url}")
    except Exception as e:
        print(f"Error downloading {url}: {e}")

def find_images(url, depth, path, visited):
    if depth == 0 or url in visited:
        return
    visited.add(url)
    try:
        response = requests.get(url)
        soup = BeautifulSoup(response.text, 'html.parser')
        for img_tag in soup.find_all('img'):
            img_url = img_tag.get('src')
            if img_url:
                img_url = urljoin(url, img_url)
                if any(img_url.endswith(ext) for ext in ['.jpg', '.jpeg', '.png', '.gif', '.bmp']):
                    download_image(img_url, path)
        if depth > 1:
            for link_tag in soup.find_all('a'):
                link_url = link_tag.get('href')
                if link_url:
                    link_url = urljoin(url, link_url)
                    find_images(link_url, depth - 1, path, visited)
    except Exception as e:
        print(f"Error fetching {url}: {e}")

def main():
    parser = argparse.ArgumentParser(description='Spider program to download images from a website.')
    parser.add_argument('-r', action='store_true', help='Recursively download images')
    parser.add_argument('-l', type=int, default=5, help='Maximum depth level of recursive download')
    parser.add_argument('-p', type=str, default='./data/', help='Path to save downloaded images')
    parser.add_argument('url', type=str, help='URL to download images from')
    args = parser.parse_args()

    if not os.path.exists(args.p):
        os.makedirs(args.p)

    find_images(args.url, args.l if args.r else 1, args.p, set())

if __name__ == '__main__':
    main()