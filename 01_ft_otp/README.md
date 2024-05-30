# <p align="center">Arachnida</p>

> *An introductory project to web scraping and metadata manipulation.*

## Project Description
Arachnida is a cybersecurity project aimed at developing skills in web scraping and metadata analysis. The project involves two main tasks:
1. **Spider**: A program to automatically extract and download images from a specified website.
2. **Scorpion**: A program to analyze the metadata of the downloaded images.

### Features
- **Spider**:
  - Recursively download images from a given URL.
  - Specify recursion depth and download path.
  - Supports .jpg, .jpeg, .png, .gif, and .bmp image formats.
- **Scorpion**:
  - Parse and display EXIF metadata from image files.
  - Supports the same image formats as the Spider program.

## Install
```bash
pip install -r requirements.txt
```

## Usage
```bash
./main.py -g <64 hex bytes>
# exemple: ./main.py -g $(for _ in {1..64}; do echo -n f; done)
./main.py -ik
# or ./main.py -qik
```

