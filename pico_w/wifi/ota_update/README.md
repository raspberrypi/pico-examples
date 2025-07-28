# Overview

This example demonstrates how to implement an Over The Air (OTA) software update mechanism using facilities provided by the RP2350 bootrom.

 A python script runs on a host machine to _push_ a new operating software image to a Pico 2 W running the `ota_update` example image. The incoming image is received via the LwIP IP stack and programmed into Pico 2 W flash memory.

On successful completion of the flash programming, the Pico 2 W will be rebooted and the updated operating image will be selected and executed by the RP2350 bootrom.  This process can be repeated as required.

## More detail

The Pico 2 W listens on TCP port 4242.  The incoming octet stream, pushed by the host, is expected to be a sequence of UF2 blocks which are programmed into Pico 2 W flash using the APIs provided by the RP2350 bootrom.  In addition to programming, the received data is 'hashed' using the RP2350 SHA256 hardware, and the hash sent back to host to allow integrity checking.

The flash must be appropriately  partitioned for this example to work. Two ` IMAGE_DEF` partitions are required, one for the currently running software and the other which is updated by the `ota_update` example.

Note: This example _also_ demonstrates how the CYW43 Wi-fi firmware can be stored in a separate flash partition(s). This means the Pico 2 W application can be updated separately which reduces the size of the update download.

For more information flash partitioning and boot image selection please see section 5 of [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

## How to

### Flash partitioning

Before the example can be run, the Flash on the Pico 2 W must be appropriately partitioned.

Flash partitioning only needs to be done once for this example but, as written below, will completely remove any previous flash contents.

The required partition data can be loaded by creating a UF2 from the partition table JSON in this folder:

```
picotool partition create main.json pt.uf2
```
then dragging & dropping this UF2 onto the device, or loading it using `picotool` and rebooting:
```
picotool load pt.uf2
picotool reboot -u
```

> [!NOTE]
> `reboot -u` reboots back to bootsel mode so we can send more commands to the device

Once the partition table is loaded, you then need to load the Wi-Fi firmware UF2 (`picow_ota_update_firmware.uf2`) followed by loading and executing the main program (`picow_ota_update.uf2`) - either by dragging and dropping them in order, or using `picotool`:
```
picotool load picow_ota_update_firmware.uf2
picotool load -x picow_ota_update.uf2
```

> [!NOTE]
> `load -x` attempts to execute the uf2 after the load

### Operation

This example will send debug output text on the default UART. On startup it displays the current boot partition (from where the firmware is running) and IP address of the Pico 2 W

```
Boot partition was 0
Starting server at 192.168.0.103 on port 4242
```

Once running, you can use [python_ota_update.py](python_ota_update.py) to upload new UF2s from the host to the Pico 2 W using it's IP address.  For example:
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

The update is downloaded into the free boot partition that's not currently in use, before rebooting the Pico 2 W to take the new software into use. After the restart the boot partition should have changed.
