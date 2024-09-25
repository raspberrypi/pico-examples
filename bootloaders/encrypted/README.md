Replace private.pem and privateaes.bin with your own keys - your signing key must be for the _secp256k1_ curve, in PEM format. You can create a .PEM file with:

```bash
openssl ecparam -name secp256k1 -genkey -out private.pem
```

The AES key is just a 32 byte binary file - you can create one with

```bash
dd if=/dev/urandom of=privateaes.bin bs=1 count=32
```

You will need to program your OTP using the generated `otp.json` file in the build folder. Note that this will enable secure boot on your device, so only signed binaries can run, and will also lock down the page the AES key is stored in. If you wish to test without enabling secure boot then you can load the `otp.json` file in the source folder, which will just program the AES key and lock down that page.
```bash
picotool otp load otp.json
```

Then either drag & drop the UF2 files to the device in order (enc_bootloader first, then hello_serial_enc) waiting for a reboot in-between, or run
```bash
picotool load enc_bootloader.uf2
picotool reboot -u
picotool load -x hello_serial_enc.uf2
```
