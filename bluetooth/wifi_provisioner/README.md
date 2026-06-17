### BLE wifi provisioning

This example demonstrates provisioning wifi credentials using bluetooth low energy.
The pico saves the most recent set of succesful credentials in flash for future use.
Upon powering, the pico attemps to connect using the saved credentials.
If this fails, the pico sets up a GATT server which you can connect to using a mobile BLE scanner app or the attached python script.
The GATT server has 2 custom characteritics, one for ssid and one for password.
To write to these characteristics you can run 'python3 set_credentials.py ssid password address'.
To run set_credentials.py you have to install the "bleak" python library, e.g...

```
python3 -m venv venv
. venv/bin/activate
pip install bleak
```

From the on you just need to activate the python virtual environment

```
. venv/bin/activate
```

It takes 3 parameters, the ssid name, the password and the Bluetooth address of the device running this example, e.g.

```
python set_credentials.py "my ssid" "my password" 2C:CF:67:BE:08:05
submitted ssid:  my ssid
submitted password:  my password
submitted address:  2C:CF:67:BE:08:05
Connected: True
Writing SSID...
Writing password...
```

The example waits 3s for you to press `W` when it starts to make it wipe any stored ssid and password to help testing.

On the pico you should something like this...

```
Waiting to receive ssid and password via BLE
Identity resolving failed
Connection complete
Pairing started
Just Works requested
Pairing complete, success
Setting SSID
Current saved SSID: "my ssid"
Current saved password length: 0
Setting password
Current saved SSID: "my ssid"
Current saved password length: 7
connect status: joining
connect status: no ip
connect status: link up
Succesfully provisioned credentials using wifi_prov_lib!
finished provisioning result=0

Ready, running iperf server at 10.3.194.230
```

When connected to the internet the example runs iperf.
Press "D" to disconnect and the pico will reboot.
The next time it connects it should retrieve the ssid and password details from flash.

```
Read credentials
Current saved SSID: "my ssid"
BTstack up and running on 2C:CF:67:BE:08:05.
Current saved password length: 7

connect status: joining
connect status: no ip
connect status: link up
Connected.
finished provisioning result=0

Ready, running iperf server at 10.3.194.230
```
