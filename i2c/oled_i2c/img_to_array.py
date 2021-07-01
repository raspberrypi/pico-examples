#!/usr/bin/env python3

# Converts a greyscale image into a format able to be
# displayed by the SSD1306 driver in horizontal addressing mode

# usage: ./img_to_array.py <logo.bmp> on a Raspberry Pi
# or python3 img_to_array.py <logo.bmp>

# depends on the Pillow library
# `python3 -m pip install --upgrade Pillow`

from PIL import Image
import sys

OLED_HEIGHT = 32
OLED_WIDTH = 128
OLED_PAGE_HEIGHT = 8
OLED_NUM_PAGES = OLED_HEIGHT / OLED_PAGE_HEIGHT

if len(sys.argv) < 2:
    print("No image path provided.")
    sys.exit()

img_path = sys.argv[1]

try:
    im = Image.open(img_path)
except OSError:
    print("Oops! The image could not be opened.")
    sys.exit()

if im.size[0] > OLED_WIDTH or im.size[1] > OLED_HEIGHT:
    print("OLED display only 128 pixels wide and 32 pixels high!")
    sys.exit()

if not (im.mode == "1" or im.mode == "L"):
    print("Images must be grayscale only")
    sys.exit()

# black or white
out = im.convert("1")

IMG_WIDTH = out.size[0]
IMG_HEIGHT = out.size[1]

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
for i in range(IMG_HEIGHT // OLED_PAGE_HEIGHT):
    start_index = i*IMG_WIDTH*OLED_PAGE_HEIGHT
    for j in range(IMG_WIDTH):
        out_byte = 0
        for k in range(8):
            out_byte |= pixels[k*IMG_WIDTH + start_index + j] << k
        buffer.append(f'{out_byte:#04x}')

buffer = ", ".join(buffer)
buffer_hex = f"static uint8_t[] = {{{buffer}}}"

print(buffer_hex)
