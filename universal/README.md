# Universal Examples

These examples show ways to load the same code onto different chips, and package
it in such a way that the bootrom only executes the code compatible with that chip.

## Universal Binary vs Universal UF2

There is a difference between a **Universal Binary** and a **Universal UF2**,
for the purposes of these examples:
- A **Universal Binary** is a `.bin` file that can be loaded into flash (or sram) and executed,
allowing RP2040 and RP2350 (Arm & RISC-V) to run from identical flash contents.
- A **Universal UF2** is multiple individual `.uf2` files with different family IDs
concatenated together to create a single `.uf2` file. When dragged & dropped onto a device, 
only the portion of the file with a family ID corresponding to that device will be processed, and the
rest of the file will be ignored.

A **Universal Binary** can be packaged into a UF2 file for loading onto a device. However,
as there isn't a common family ID between RP2040 and RP2350, you would have to package it into a **Universal UF2** with two copies (using `rp2040` and `absolute` family IDs), thus creating a **Universal UF2** of a **Universal Binary**.

## How Universal Binaries work

Universal binaries must be recognised by both the RP2040 and RP2350 bootroms. Therefore, they need the following structure for flash binaries:
- RP2040 boot2
  - Required by the RP2040 bootrom
- RP2040 binary containing an embedded block
  - The embedded block contains an `IGNORED` item due to RP2350-E13, but you can use an RP2040
  `IMAGE_DEF` item instead if not using RP2350-A2 chips
- RP2350 Arm binary containing an embedded block
  - In addition to the RP2350 `IMAGE_DEF` item, this embedded block contains a
  `ROLLING_WINDOW_DELTA` item to translate this binary to the start of flash for execution
- RP2350 RISC-V binary containing an embedded block
  - Ditto

All of the embedded blocks are linked into one big block loop.

These are then booted by the respective bootroms:
- **RP2040** - sees the boot2 at the start and uses that to execute the RP2040 binary, as
RP2040 has no support for embedded blocks.
- **RP2350** - sees the block loop and parses it to find the correct embedded block to boot
from (Arm vs RISC-V). It then translates the flash address according to the
`ROLLING_WINDOW_DELTA` so that the binary containing that embedded block appears at the start of the
flash address space, and executes from there.

For no_flash binaries the RP2040 boot2 is omitted as the RP2040 bootrom just executes from the start
of SRAM, and instead of `ROLLING_WINDOW_DELTA` items the RP2350 binaries use `LOAD_MAP` items,
to copy the code in SRAM to the correct location for execution rather than using address
translation.

## How you should use them

For most use cases, **Universal UF2s** are the best option to use. They will only load the
code that runs on that device into flash. The [blink_universal](blink_universal) example uses a
Universal UF2 for that reason, as the Wi-Fi firmware is quite large. **Universal Binaries**
are only currently useful when the commonality of having a single `.bin` file for programming
outweighs the disadvantage of the extra flash usage.
