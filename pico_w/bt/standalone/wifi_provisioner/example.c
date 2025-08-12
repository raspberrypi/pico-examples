#include <stdio.h>
#include <wifi_prov_lib.h>


int main(void) {
    // if unable to connect with saved ssid and password, waits 120 seconds
    // for new credentials to be provisioned over BLE
    start_ble_wifi_provisioning(120000);
    printf("finished provisioning\n");
}