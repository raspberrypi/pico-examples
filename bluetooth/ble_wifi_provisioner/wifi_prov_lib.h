/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #ifndef WIFI_PROV_LIB
#define WIFI_PROV_LIB

int start_ble_wifi_provisioning(int ble_timeout_ms);
int erase_credentials(void);

#endif