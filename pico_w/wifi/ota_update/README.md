This example requires a partition table in flash. This can be loaded by creating a UF2 from the partition table JSON in this folder:
```
picotool partition create main.json pt.uf2
```
then dragging & dropping this UF2 onto the device, or loading it using `picotool` and rebooting:
```
picotool load pt.uf2
picotool reboot -u
```

Once the partition table is loaded, you first need to load the Wi-Fi firmware UF2 (`picow_ota_update_firmware.uf2`) followed by loading & executing the main program (`picow_ota_update.uf2`) - either by dragging and dropping them in order, or using `picotool`:
```
picotool load picow_ota_update_firmware.uf2
picotool load -x picow_ota_update.uf2
```

Once running, you can use [python_ota_update.py](python_ota_update.py) to upload new UF2s to it using it's IP address:
```
python python_ota_update.py 192.168.0.103 picow_ota_update.uf2
```
