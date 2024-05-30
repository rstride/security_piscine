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
./spider [-rlp] URL
```
•Option -r : recursively downloads the images in a URL received as a parameter.
•Option -r -l [N] : indicates the maximum depth level of the recursive download.
If not indicated, it will be 5.
•Option -p [PATH] : indicates the path where the downloaded files will be saved.
If not specified, ./data/ will be used.

```
./scorpion FILE1 [FILE2 ...]
```
