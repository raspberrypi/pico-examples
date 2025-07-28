# Overview

This example demonstrates how to implement an Over-The-Air (OTA) software update mechanism using facilities provided by the RP2350 bootrom.

A python script runs on a host machine to _push_ a new (UF2 format) application image to a Pico 2 W running the `ota_update` example application. The incoming application image is received via the LwIP IP stack and programmed into Pico 2 W flash memory.

Two flash partitions ("A"/"B") are used for application images so that the application can be running from one, while the new application image is being written to the other. The RP2350 bootrom provides support for determining the location in flash memory to write the new version of the application.

On successful completion of the flash programming, the Pico 2 W will be rebooted and the updated operating image will be selected and executed by the RP2350 bootrom.  This process can be repeated as required.

## More detail

The Pico 2 W listens on TCP port 4242.  The host [python_ota_update.py](python_ota_update.py) script transmits the update image in a series of fixed sized chunks.  Each chunk contains an integer number of UF2 image blocks.  If the update image UF2 block count is not an exact sub-multiple of the number of chunks the script will pad the last chunk as required.

On receipt of pushed chunks, the ota_update image will program the UF2 block stream into flash using the APIs provided by the RP2350 bootrom. In addition to programming, each chunk is 'hashed', using SHA256, and the calculated hash transmitted to the host as an acknowledgement of the received chunk. The host script compares the local and remotely computed hashes and if data corruption has occurred the update will halt.

The flash must be appropriately partitioned for this example to work. Two partitions are required, one for the currently running software and the other to be updated with incoming data.

Note: This example _also_ demonstrates how the CYW43 Wi-fi firmware can be stored in a separate flash partition(s). This means the Pico 2 W application can be updated separately which reduces the size of the update download.

For more information flash partitioning and boot image selection please see section 5 of [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

# How to

## Flash partitioning

Before the example can be run, the Flash on the Pico 2 W must be appropriately partitioned.

The required partition data can be loaded by creating a UF2 from the supplied partition table JSON in this folder:

```
picotool partition create main.json pt.uf2
```
then dragging & dropping this UF2 onto the device, or loading it using `picotool` and rebooting:
```
picotool load pt.uf2
picotool reboot -u
```

> **NOTE**
> `reboot -u` reboots back to bootsel mode so we can send more commands to the device, and additionally the device needs to have rebooted in order for the partition table to take effect.

WIth the partition table loaded, you then need to load the Wi-Fi firmware UF2 (`picow_ota_update_wifi_firmware.uf2`) followed by loading and executing the main program (`picow_ota_update.uf2`) - either by dragging and dropping them in order, or using `picotool`:
```
picotool load picow_ota_update_wifi_firmware.uf2
picotool load -x picow_ota_update.uf2
```

> **NOTE**
> The first `load` writes the Wifi Firmware into one of the partitions set aside for that. The second `load -x` loads the main application code into one of the main partitions, then resets the chip to start execution (`-x`)

## Operation

This example will send debug output text on the default UART. On startup it displays the current boot partition (from where the firmware is running) and IP address of the Pico 2 W

```
Boot partition was 0
Starting server at 192.168.0.103 on port 4242
```

Once running, you can use [python_ota_update.py](python_ota_update.py) to upload new UF2s from the host to the Pico 2 W using its IP address.  For example:
```
python ./python_ota_update.py 192.168.0.103 picow_ota_update.uf2
```
This will update the Pico 2 W at `192.168.0.103` with the specified image.

```
Code Target partition is fc6d21b6 fc061003
Start 1b6000, End 36a000, Size 1b4000
Done - rebooting for a flash update boot 0
Chosen CYW43 firmware in partition 2
Boot partition was 1
Someone updated into me
Flash update base was 101b6000
Update info 1
Update info now 4
Starting server at 192.168.0.103 on port 4242
```

The update is downloaded into the main partition that's not currently in use, before rebooting the Pico 2 W to run the new version of the software. This choice of software is made by the RP2350 bootrom based on the version number of the software
