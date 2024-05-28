# <p align="center">Arachnida</p>
The spider program allow you to extract all the images from a website, recursively, by
providing a url as a parameter.

The scorpion program receive image files as parameters and must be able to
parse them for EXIF and other metadata, displaying them on the screen.

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
