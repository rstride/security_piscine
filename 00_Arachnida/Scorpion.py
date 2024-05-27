import argparse
from PIL import Image
import piexif

def display_metadata(file_path):
    try:
        image = Image.open(file_path)
        if 'exif' in image.info:
            exif_data = piexif.load(image.info['exif'])
            print(f"Metadata for {file_path}:")
            for ifd in exif_data:
                if exif_data[ifd] is None:
                    continue
                print(f"{ifd}:")
                for tag in exif_data[ifd]:
                    tag_name = piexif.TAGS[ifd][tag]["name"]
                    print(f"    {tag_name}: {exif_data[ifd][tag]}")
        else:
            print(f"No EXIF metadata found for {file_path}")
    except Exception as e:
        print(f"Error reading {file_path}: {e}")

def main():
    parser = argparse.ArgumentParser(description='Scorpion program to display image metadata.')
    parser.add_argument('files', metavar='FILE', type=str, nargs='+', help='Image files to display metadata')
    args = parser.parse_args()

    for file_path in args.files:
        display_metadata(file_path)

if __name__ == '__main__':
    main()