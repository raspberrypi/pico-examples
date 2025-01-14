Replace private.pem and privateaes.bin with your own keys - your signing key must be for the _secp256k1_ curve, in PEM format. You can create a .PEM file with:

```bash
openssl ecparam -name secp256k1 -genkey -out private.pem
```

The AES key is stored as a 4-way share in a 128 byte binary file - you can create one with

```bash
dd if=/dev/urandom of=privateaes.bin bs=1 count=128
```

or in Powershell 7
```powershell
[byte[]] $(Get-SecureRandom -Maximum 256 -Count 128) | Set-Content privateaes.bin -AsByteStream
```

Then either drag & drop the UF2 files to the device in order (enc_bootloader first, then hello_serial_enc) waiting for a reboot in-between, or run
```bash
picotool load enc_bootloader.uf2
picotool reboot -u
picotool load -x hello_serial_enc.uf2
```
