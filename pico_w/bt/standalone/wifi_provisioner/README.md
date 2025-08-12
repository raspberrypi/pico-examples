### BLE wifi provisioning

This example demonstrates provisioning wifi credentials using bluetooth low energy. The pico saves the most recent set of succesful credentials in flash for future use. Upon powering, the pico attemps to connect using the saved credentials. If this fails, the pico sets up a GATT server which you can connect to using a mobile BLE scanner app or the attached python script. The GATT server has 2 custom characteritics - one for ssid and one for password. To write to these characteristics you can run 'python3 set_credentials.py ssid password address'.
