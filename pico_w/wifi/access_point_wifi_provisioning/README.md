## Access point Wi-Fi provisioning

This example shows how to provision Wi-Fi credentials over an access point, so a
headless Pico W can be put onto a network without compiling the SSID and password
into the firmware. Credentials entered through a web page are saved to flash and
reused on subsequent boots, so once a network has been provisioned the device
connects to it automatically.

### How it works

On startup the Pico W reads any credentials saved in flash and tries each set in
turn as a Wi-Fi station (STA). If one succeeds, provisioning is complete and the
example goes on to run an iperf server as a placeholder for whatever your
application would do once connected.

If no saved credentials work (or none are stored yet), the Pico W starts an
access point named `picow_test` and runs a web server on it. Connect a phone or
laptop to that access point and you will be directed to a provisioning page where
you can enter the SSID and password of the network you want to join. A bundled
DNS server resolves every name to the Pico W itself, so any address you open in a
browser lands on the provisioning page (a captive portal). A DHCP server hands
the connecting device an address on the access point's subnet.

When you submit credentials the Pico W attempts to join that network. The access
point stays up during the attempt, so the page remains reachable: if the join
fails you can simply try again without reconnecting. On success the access point
and its DNS/DHCP servers are shut down and the new credentials are saved to flash
for next time.

You can also pick from previously saved networks on the same page, and clear all
saved credentials with the "Clear saved credentials" button.

### Using it

1. Build and flash the example, then open a serial terminal to watch its progress.
2. On first run there are no saved credentials, so the Pico W starts the
   `picow_test` access point (password `password`).
3. Connect a phone or laptop to `picow_test`. Most devices will pop up the
   captive-portal page automatically; if not, open any web address in a browser.
4. Enter the SSID and password of your network and press **Connect**.
5. On success the Pico W joins your network, shuts down the access point, and
   saves the credentials. The serial output shows the assigned IP address.
6. On later boots the Pico W connects to a saved network automatically without
   starting the access point.

At startup the example also offers two serial options: press `w` within the first
few seconds to wipe all stored credentials, or `s` to skip the automatic connect
and go straight to the access point.

### Notes

Credentials are stored in a single flash sector as a list of SSID/password pairs.
Saving a network whose SSID already exists updates that entry in place rather than
adding a duplicate. The number of stored networks and the maximum SSID/password
lengths are set by `MAX_CREDENTIALS`, `MAX_SSID_LEN` and `MAX_PASSWORD_LEN` in the
source; the list must fit within one 4096-byte sector.