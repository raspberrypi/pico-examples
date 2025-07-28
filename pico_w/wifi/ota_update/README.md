# Overview

This example uses a python script running on a host machine to _push_ new operating software image to a specified Pico 2 W running this `ota_update` example image.  The incoming image is programmed into Pico 2 W flash memory.

On successful completion of the flash programming, the Pico 2 W will be rebooted and the updated operating image will be selected and executed by the RP2350 bootrom.  This process can be repeated as required.

## More detail

The Pico 2 W operates as a server which listens on TCP port 4242.  The incoming octet stream, pushed by the host, is expected to be a sequence of UF2 blocks which are programmed into Pico 2 W flash using the APIs provided by the RP2350 bootrom.  In addition to programming, the received data is 'hashed' using the RP2350 SHA256 hardware, and the hash sent back to host to allow integrity checking.

The flash must be appropriately  partitioned for this example to work; most obviously *two* ` IMAGE_DEF` partitions are required, one for the running software and the other which is updated by the example software.

Note: This example _also_ demonstrates storage of the CYW43 Wi-fi firmware object using additional flash partition(s) although this firmware is not updated by this example.

For more information flash partitioning and boot image selection please see section 5 of [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

## How to

### Flash partitioning

Before the example can be run, the Flash on the Pico 2 W must be appropriately partitioned.

Flash partitioning only needs doing only once for this example but, as written below, will completely remove any previous flash contents.

The required partition data can be loaded by creating a UF2 from the partition table JSON in this folder:

```
picotool partition create main.json pt.uf2
```
then dragging & dropping this UF2 onto the device, or loading it using `picotool` and rebooting:
```
picotool load pt.uf2
picotool reboot -u
```

Once the partition table is loaded, you first need to load the Wi-Fi firmware UF2 (`picow_ota_update_wifi_firmware.uf2`) followed by loading and executing the main program (`picow_ota_update.uf2`) - either by dragging and dropping them in order, or using `picotool`:
```
picotool load picow_ota_update_wifi_firmware.uf2
picotool load -x picow_ota_update.uf2
```

### Operation

Once running, you can use [python_ota_update.py](python_ota_update.py) to upload new UF2s from the host to it using it's IP address.  For example:
```
python ./python_ota_update.py 192.168.0.103 picow_ota_update.uf2
```

Will update the Pico 2 W at `192.168.0.103` with the specified image.

Note: This example will send debug output text on the default UART 0, `GPIO 0`.
