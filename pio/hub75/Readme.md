HUB75E Pinout:

```
    /-----\
R0  | o o | G0
B0  | o o | GND
R1  | o o | G1
B1  \ o o | E
A   / o o | B
C   | o o | D
CLK | o o | STB
OEn | o o | GND
    \-----/
```

Wiring:

```
Must be contiguous, in order:
R0 - GPIO0
G0 - GPIO1
B0 - GPIO2
R1 - GPIO3
G1 - GPIO4
B1 - GPIO5

Must be contiguous, somewhat ok to change order:
A - GPIO6
B - GPIO7
C - GPIO8
D - GPIO9
E - GPIO10

Can be anywhere:
CLK - GPIO11

Must be contiguous, in order:
STB - GPIO12
OEn - GPIO13
```

This is a 1/32nd scan panel. The inputs A, B, C, D, E select one of 32 rows, starting at the top and working down (assuming the first pixel to be shifted is the one on the left of the screen, even though this is the "far end" of the shift register). R0, B0, G0 contain pixel data for the upper half of the screen. R1, G1, B1 contain pixel data for the lower half of the screen, which is scanned simultaneously with the upper half.

Image credit for mountains_128x64.png: Paul Gilmore, found on [this wikimedia page](https://commons.wikimedia.org/wiki/File:Mountain_lake_dam.jpg)

