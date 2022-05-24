#!/usr/bin/env python3

# Converts a grayscale image into a format able to be
# displayed by the SSD1306 driver in horizontal addressing mode

# usage: python3 img_to_array.py <logo.bmp>

# depends on the Pillow library
# `python3 -m pip install --upgrade Pillow`

from PIL import Image
import sys
from pathlib import Path

OLED_HEIGHT = 32
OLED_WIDTH = 128
OLED_PAGE_HEIGHT = 8

if len(sys.argv) < 2:
    print("No image path provided.")
    sys.exit()

img_path = sys.argv[1]

try:
    im = Image.open(img_path)
except OSError:
    raise Exception("Oops! The image could not be opened.")

img_width = im.size[0]
img_height = im.size[1]

if img_width > OLED_WIDTH or img_height > OLED_HEIGHT:
    print(f'Your image is f{img_width} pixels wide and {img_height} pixels high, but...')
    raise Exception(f"OLED display only {OLED_WIDTH} pixels wide and {OLED_HEIGHT} pixels high!")

if not (im.mode == "1" or im.mode == "L"):
    raise Exception("Image must be grayscale only")

# black or white
out = im.convert("1")

img_name = Path(im.filename).stem

# `pixels` is a flattened array with the top left pixel at index 0
# and bottom right pixel at the width*height-1
pixels = list(out.getdata())

# swap white for black and swap (255, 0) for (1, 0)
pixels = [0 if x == 255 else 1 for x in pixels]

# our goal is to divide the image into 8-pixel high pages
# and turn a pixel column into one byte, eg for one page:
# 0 1 0 ....
# 1 0 0
# 1 1 1
# 0 0 1
# 1 1 0
# 0 1 0
# 1 1 1
# 0 0 1 ....

# we get 0x6A, 0xAE, 0x33 ... and so on
# as `pixels` is flattened, each bit in a column is IMG_WIDTH apart from the next

buffer = []
for i in range(img_height // OLED_PAGE_HEIGHT):
    start_index = i*img_width*OLED_PAGE_HEIGHT
    for j in range(img_width):
        out_byte = 0
        for k in range(OLED_PAGE_HEIGHT):
            out_byte |= pixels[k*img_width + start_index + j] << k
        buffer.append(f'{out_byte:#04x}')

buffer = ", ".join(buffer)
buffer_hex = f'static uint8_t {img_name}[] = {{{buffer}}}\n'

with open(f'{img_name}.h', 'wt') as file:
    file.write(f'#define IMG_WIDTH {img_width}\n')
    file.write(f'#define IMG_HEIGHT {img_height}\n\n')
    file.write(buffer_hex)
