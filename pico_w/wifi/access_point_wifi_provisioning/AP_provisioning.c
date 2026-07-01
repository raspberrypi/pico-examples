/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip4_addr.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/apps/httpd.h"
#include "lwip/pbuf.h"
#include "lwip/apps/lwiperf.h"

#include "dhcpserver.h"
#include "dnsserver.h"

#include "pico/flash.h"
#include "hardware/flash.h" // for saving succesful credentials

static absolute_time_t wifi_connected_time;

// Set by the CGI handlers (running in lwIP context) to ask the main loop to
// attempt a STA connection using the current ssid/password globals. The main
// loop owns all radio mode transitions; the handlers only signal intent.
static volatile bool connect_requested = false;

// Maximum credential content lengths (excluding the null terminator).
// 32 is the 802.11 SSID limit; 63 is the WPA/WPA2 passphrase limit.
#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 63

// buffers are content length + 1 for the null terminator
static char ssid[MAX_SSID_LEN + 1];
static char password[MAX_PASSWORD_LEN + 1];
static int num_credentials;

// Credentials are stored in a single 4096-byte flash sector as a stream of
// null-separated strings: count byte, then ssid0\0password0\0ssid1\0...
// MAX_CREDENTIALS must be small enough that that many (ssid + password)
// pairs fit in FLASH_SECTOR_SIZE (4096) bytes. Worst case per pair is
// MAX_SSID_LEN + 1 + MAX_PASSWORD_LEN + 1 = 97 bytes, so 4094/97 ~= 42;
// 20 leaves comfortable margin.
#define MAX_CREDENTIALS 20
static char ssid_list[MAX_CREDENTIALS][MAX_SSID_LEN + 1];
static char password_list[MAX_CREDENTIALS][MAX_PASSWORD_LEN + 1];

// Define flash offset towards end of flash
#ifndef PICO_FLASH_BANK_TOTAL_SIZE
#define PICO_FLASH_BANK_TOTAL_SIZE (FLASH_SECTOR_SIZE * 2u)
#endif

#ifndef PICO_FLASH_BANK_STORAGE_OFFSET
#if PICO_RP2350 && PICO_RP2350_A2_SUPPORTED 
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE - FLASH_SECTOR_SIZE)
#else
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - PICO_FLASH_BANK_TOTAL_SIZE - FLASH_SECTOR_SIZE)
#endif
#endif

static const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// Function prototypes
static void call_flash_range_erase(void *param);
static void call_flash_range_program(void *param);

static void save_credentials(const char *new_ssid, const char *new_password);
static void read_credentials(void);

static void attempt_wifi_connection(void);

static const char *connect_from_saved_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *clear_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
    , uint16_t current_tag_part, uint16_t *next_tag_part
#endif
);

static tCGI cgi_handlers[] = {
    { "/connect_from_saved.cgi", connect_from_saved_cgi_handler},
    {"/clear.cgi", clear_cgi_handler}
};

// Be aware of LWIP_HTTPD_MAX_TAG_NAME_LEN
static const char *ssi_tags[] = {
    "wifilist",
    "ssid",
    "password"
};

#define CYW43_STA_IS_ACTIVE(self) (((self)->itf_state >> CYW43_ITF_STA) & 1)
#define CYW43_AP_IS_ACTIVE(self) (((self)->itf_state >> CYW43_ITF_AP) & 1)

#define WIFI_CONNECT_TIME_S 20

#define AP_SSID "picow_test"
#define AP_PASSWORD "password"

// Report IP results and exit
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;

    total_iperf_megabytes += mbytes;

    printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
}

// Note: This is called from an interrupt handler
void key_pressed_func(void *param) {
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (key == 'd' || key == 'D') {
        bool *exit = (bool*)param;
        *exit = true;
    }
}

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    printf("intitialised\n");

    // To make testing easier - add an option to clear the ssid storage
    // or skip the initial automatic wifi connect
    printf("Press 'w' in the next 3s to WIPE stored ssid and passwords\n");
    printf("Press 's' to SKIP automatic WiFi connect on startup\n");
    int c = getchar_timeout_us(3000000);
    if (c == 'w' || c == 'W') {
        // for testing, erase memory first
        // might need to erase memory first time you use provisioning in case there are garbage values in flash
        int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
        hard_assert(rc == PICO_OK);
    }
    bool try_connect = true;
    if (c == 's' || c == 'S') {
        try_connect = false;
    }

    // Loop completion is driven by the return code of the blocking connect
    // calls, not by polling link status: cyw43_arch_wifi_connect_timeout_ms
    // only returns success once an IP address has been obtained (CYW43_LINK_UP),
    // so its rc is an authoritative "connected" signal.
    bool connected = false;

    read_credentials();
    if (try_connect) {
        // First, try to connect to network using one of the saved credentials
        // the idea is that once you have saved credentials wifi should "just work"
        for (int i = 0; i < num_credentials; i++) {
            if (i == 0) cyw43_arch_enable_sta_mode();
            printf("Trying to connect with STA \"%s\" at index %d\n", ssid_list[i], i);
            int rc = cyw43_arch_wifi_connect_timeout_ms(ssid_list[i], password_list[i], CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIME_S * 1000);
            if (rc) { 
                printf("failed to connect with saved credentials rc=%d\n", rc);
                cyw43_arch_disable_sta_mode();
            } else {
                printf("Connected.\n");
                connected = true;
                break;
            }
        }
    }

    // Enter a loop until wifi is connected.
    // The DHCP server and DNS server structs are declared here so they outlive
    // every loop iteration - lwIP callbacks hold pointers to them. They (and
    // httpd) are initialised once, guarded by services_up.
    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
    bool services_up = false;

    // The AP (and its DHCP/DNS/http services) is brought up once and kept up
    // for the whole provisioning session, so the user's browser stays connected
    // to the captive portal across failed connection attempts. The CYW43439 is
    // a single-radio part, so AP and STA share a channel: when STA associates,
    // the AP is dragged to the router's channel and notifies its client via a
    // CSA so the phone can follow. On a *successful* connect we tear the AP down
    // so the device settles cleanly onto the STA channel.
    while(!connected) {
        // Bring up the AP + services once, at the start (neither itf active yet).
        if (!CYW43_STA_IS_ACTIVE(&cyw43_state) && !CYW43_AP_IS_ACTIVE(&cyw43_state) && !services_up) {
            cyw43_arch_enable_ap_mode(AP_SSID, AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
            printf("\nReady, running web server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
            printf("Connect to ssid \"%s\" with password \"%s\"\n", AP_SSID, AP_PASSWORD);

            #if LWIP_IPV6
            #define IP(x) ((x).u_addr.ip4)
            #else
            #define IP(x) (x)
            #endif

            ip4_addr_t mask;
            ip4_addr_t gw;
            IP(gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
            IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

            #undef IP

            dhcp_server_init(&dhcp_server, &cyw43_state.netif[CYW43_ITF_AP], &gw, &mask);
            dns_server_init(&dns_server, &cyw43_state.netif[CYW43_ITF_AP], &gw);

            char hostname[sizeof(CYW43_HOST_NAME) + 4];
            memcpy(&hostname[0], CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
            hostname[sizeof(hostname) - 1] = '\0';
            netif_set_hostname(&cyw43_state.netif[CYW43_ITF_STA], hostname);

            // start http server
            wifi_connected_time = get_absolute_time();

            // setup http server
            cyw43_arch_lwip_begin();
            httpd_init();
            http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
            http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
            cyw43_arch_lwip_end();

            services_up = true;
        }

        if (connect_requested) {
            // The user submitted credentials (via a CGI handler). Attempt the
            // connect here on the main loop, leaving the AP up so the portal
            // stays reachable. STA is enabled alongside the running AP.
            connect_requested = false;
            cyw43_arch_enable_sta_mode();
            // With both interfaces up, lwIP needs a default route for traffic
            // that isn't on the AP subnet (i.e. the join to the router). Make
            // the STA netif the default so the connect can route out.
            netif_set_default(&cyw43_state.netif[CYW43_ITF_STA]);
            printf("Trying to connect with STA \"%s\"\n", ssid);
            int rc = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIME_S * 1000);
            if (rc) {
                printf("failed to connect with credentials rc=%d\n", rc);
                // Leave the AP up; just drop the failed STA attempt and restore
                // the AP as the default netif so the portal keeps working.
                cyw43_arch_disable_sta_mode();
                netif_set_default(&cyw43_state.netif[CYW43_ITF_AP]);
            } else {
                printf("Connected.\n");
                // Success: shut down the provisioning services. The DHCP/DNS
                // server UDP PCBs are bound to ports 67/53 independent of the AP
                // netif, so just disabling AP mode leaves them listening and
                // still answering requests from any lingering AP-side clients.
                // Deinit them explicitly (frees the PCBs / unbinds the ports),
                // then drop the AP netif. httpd has no deinit in the lwIP API,
                // but with the AP down and STA-only routing it is no longer
                // reachable on the AP subnet.
                cyw43_arch_lwip_begin();
                dns_server_deinit(&dns_server);
                dhcp_server_deinit(&dhcp_server);
                cyw43_arch_lwip_end();
                services_up = false;

                cyw43_arch_disable_ap_mode();
                connected = true;
            }
        }

        if (!connected) {
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
        }
    }
    printf("Finished provisioning credentials. status=%d\n", cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA));

    // We should be connected now - run iperf to do something useful
    cyw43_arch_lwip_begin();
    printf("\nReady, running iperf server at %s (press 'd' to disconnect)\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
    cyw43_arch_lwip_end();

    // Enter a loop
    bool exit = false;
    stdio_set_chars_available_callback(key_pressed_func, &exit);
    while(!exit && cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_DOWN) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(at_the_end_of_time);
    }
    cyw43_arch_disable_sta_mode();

    cyw43_arch_deinit();
    return 0;
}

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_SECTOR_SIZE);
}

// Functions for saving and reading credentials from flash
// Saves (new_ssid, new_password). If an entry with the same ssid already exists,
// its password is updated in place (preserving list order); otherwise the entry
// is appended. Passing "" for new_ssid is treated as the "initialise empty
// store" case. Serializes from the in-memory ssid_list/password_list, so
// read_credentials must have populated them (it runs at boot and after each save).
static void save_credentials(const char *new_ssid, const char *new_password) {
    // create empty sector-sized buffer
    uint8_t flash_data[FLASH_SECTOR_SIZE] = {0};

    // Special case: empty init write, just lay down count = 0.
    if (new_ssid[0] == '\0') {
        flash_data[1] = 0;
    } else {
        // Find an existing entry with the same ssid.
        int match = -1;
        for (int i = 0; i < num_credentials; i++) {
            if (strcmp(ssid_list[i], new_ssid) == 0) {
                match = i;
                break;
            }
        }

        // Update in place, or append, in the in-memory lists.
        if (match >= 0) {
            // overwrite password only; ssid and position unchanged
            strncpy(password_list[match], new_password, sizeof(password_list[0]) - 1);
            password_list[match][sizeof(password_list[0]) - 1] = '\0';
        } else if (num_credentials < MAX_CREDENTIALS) {
            int i = num_credentials;
            strncpy(ssid_list[i], new_ssid, sizeof(ssid_list[0]) - 1);
            ssid_list[i][sizeof(ssid_list[0]) - 1] = '\0';
            strncpy(password_list[i], new_password, sizeof(password_list[0]) - 1);
            password_list[i][sizeof(password_list[0]) - 1] = '\0';
            num_credentials++;
        }
        // else: list full, silently keep existing entries

        // Serialize the lists back to flash in their current order.
        // no character has ascii value 0, so a single 0 separates each field:
        // ssid0\0password0\0ssid1\0password1\0...
        uint count = 0;
        uint pos = 2;  // byte 0 reserved, byte 1 holds the count
        for (int i = 0; i < num_credentials; i++) {
            uint sl = strlen(ssid_list[i]);
            uint pl = strlen(password_list[i]);
            if (pos + sl + 1 + pl + 1 > sizeof(flash_data)) break;  // out of room
            memcpy(&flash_data[pos], ssid_list[i], sl);     pos += sl;
            flash_data[pos++] = 0;
            memcpy(&flash_data[pos], password_list[i], pl); pos += pl;
            flash_data[pos++] = 0;
            count++;
        }
        flash_data[1] = (uint8_t)count;
    }

    // must always erase flash before write
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);

    // write flash
    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)flash_data};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);
}

static void read_credentials(void) {
    uint credential_count;

    // first check if the flash page begins with FF - this indicates the flash has not yet been written to 
    // so must initialise with empty write
    if (flash_target_contents[0] == 255) {
        save_credentials("", "");
    }

    // second byte saves credential count
    credential_count = flash_target_contents[1];
    // clamp to capacity: a corrupt/unexpected count byte must not overflow the
    // parse arrays below (we index t_*_list[space_count / 2])
    if (credential_count > MAX_CREDENTIALS) {
        credential_count = MAX_CREDENTIALS;
    }
    num_credentials = credential_count;
    printf("read_credentials count=%d\n", num_credentials);

    // initialise temporary ssid and password lists (zeroed to ensure null termination)
    char t_ssid_list[MAX_CREDENTIALS][MAX_SSID_LEN + 1] = {0};
    char t_password_list[MAX_CREDENTIALS][MAX_PASSWORD_LEN + 1] = {0};

    uint space_count = 0;
    uint start_index = 1;

    for (uint i = 2; i < FLASH_SECTOR_SIZE; i++) {
        if (space_count >= 2*credential_count) {
            break;
        } else if (flash_target_contents[i] == 0) {
            space_count++;
            start_index = i;
        } else if (flash_target_contents[i] != 0 && space_count % 2 == 0) {
            // there is a char, and even space count. So we are reading a ssid
            t_ssid_list[(int) space_count / 2][i - start_index - 1] = flash_target_contents[i];
        } else if (flash_target_contents[i] != 0 && space_count % 2 == 1) {
            // there is a char and odd space count, so reading password
            t_password_list[(int) space_count / 2][i - start_index - 1] = flash_target_contents[i];
        } 
    }
    
    // update global ssid and password lists
    memset(ssid_list, 0, sizeof(ssid_list));
    memcpy(ssid_list, t_ssid_list, sizeof(t_ssid_list));

    memset(password_list, 0, sizeof(password_list));
    memcpy(password_list, t_password_list, sizeof(t_password_list));
}

static void attempt_wifi_connection(void) {
    save_credentials(ssid, password);
    read_credentials();
    // Signal the main loop to do the actual connect. We deliberately do not
    // change radio mode here: this runs in lwIP/network context, and all
    // cyw43 mode transitions are owned by the main loop where polling happens.
    connect_requested = true;
}

// Decodes application/x-www-form-urlencoded text in place.
// '+' -> ' ', "%XX" -> byte. dst and src may be the same buffer.
// Trailing spaces are stripped from the result.
static void url_decode(char *dst, const char *src, size_t dst_size) {
    size_t di = 0;
    while (*src && di + 1 < dst_size) {
        char c = *src;
        if (c == '+') {
            dst[di++] = ' ';
            src++;
        } else if (c == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            int hi = src[1] <= '9' ? src[1] - '0' : (src[1] | 0x20) - 'a' + 10;
            int lo = src[2] <= '9' ? src[2] - '0' : (src[2] | 0x20) - 'a' + 10;
            dst[di++] = (char)((hi << 4) | lo);
            src += 3;
        } else {
            dst[di++] = c;
            src++;
        }
    }
    // strip trailing spaces
    while (di > 0 && dst[di - 1] == ' ') {
        di--;
    }
    dst[di] = '\0';
}

// Returns the value of the named CGI parameter, or NULL if not present.
static const char *cgi_param(const char *name, int iNumParams, char *pcParam[], char *pcValue[]) {
    for (int i = 0; i < iNumParams; i++) {
        if (pcParam[i] != NULL && strcmp(pcParam[i], name) == 0) {
            return pcValue[i];
        }
    }
    return NULL;
}

// Extracts the raw (still url-encoded) value of a form parameter from the POST
// body pbuf. Returns a pointer into value_buf (null-terminated) or NULL if the
// parameter is not found / does not fit. Follows the pico-examples httpd pattern.
static char *post_param_value(struct pbuf *p, const char *param_name, char *value_buf, size_t value_buf_len) {
    size_t param_len = strlen(param_name);
    u16_t param_pos = pbuf_memfind(p, param_name, param_len, 0);
    if (param_pos == 0xFFFF) {
        return NULL;
    }
    u16_t value_pos = param_pos + param_len;
    // value runs until the next '&' or the end of the body
    u16_t amp_pos = pbuf_memfind(p, "&", 1, value_pos);
    u16_t value_len = (amp_pos != 0xFFFF) ? (amp_pos - value_pos)
                                          : (p->tot_len - value_pos);
    if (value_len == 0 || value_len >= value_buf_len) {
        return NULL;
    }
    char *result = (char *)pbuf_get_contiguous(p, value_buf, value_buf_len, value_len, value_pos);
    if (result) {
        result[value_len] = '\0';
    }
    return result;
}

// Only one POST is handled at a time; track which connection owns it.
static void *current_post_connection;
// Set true once a POST body has been successfully decoded into ssid/password,
// so httpd_post_finished knows to kick off a connection attempt.
static bool post_credentials_ok;

err_t httpd_post_begin(void *connection, const char *uri, __unused const char *http_request,
                       __unused u16_t http_request_len, __unused int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd) {
    if (strcmp(uri, "/credentials.cgi") == 0) {
        current_post_connection = connection;
        post_credentials_ok = false;
        // default redirect; overwritten on success in httpd_post_finished
        snprintf(response_uri, response_uri_len, "/index.shtml");
        *post_auto_wnd = 1;
        return ERR_OK;
    }
    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    err_t ret = ERR_VAL;
    if (connection == current_post_connection) {
        // For a small two-field form the body arrives in a single pbuf, so we
        // parse it directly here. post_param_value searches this pbuf only.
        char ssid_enc[sizeof(ssid)];
        char password_enc[sizeof(password)];

        char *s = post_param_value(p, "ssid=", ssid_enc, sizeof(ssid_enc));
        char *pw = post_param_value(p, "password=", password_enc, sizeof(password_enc));

        if (s != NULL && pw != NULL) {
            url_decode(ssid, s, sizeof(ssid));
            url_decode(password, pw, sizeof(password));
            printf("SSID AND PASSWORD: >%s< >%s<\n", ssid, password);
            post_credentials_ok = true;
            ret = ERR_OK;
        } else {
            printf("missing ssid/password in POST body\n");
        }
    }
    pbuf_free(p);
    return ret;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
    if (connection == current_post_connection) {
        snprintf(response_uri, response_uri_len, "/index.shtml");
        if (post_credentials_ok) {
            // Store the new credentials and ask the main loop to connect.
            attempt_wifi_connection();
        }
    }
    current_post_connection = NULL;
}

static const char *connect_from_saved_cgi_handler(__unused int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("connect_from_saved_cgi_handler called\n");

    const char *index_str = cgi_param("index", iNumParams, pcParam, pcValue);
    if (num_credentials == 0 || index_str == NULL) {
        printf("no saved network selected\n");
        return "/index.shtml";
    }

    int idx = atoi(index_str);
    if (idx < 0 || idx >= num_credentials) {
        printf("index %d out of range\n", idx);
        return "/index.shtml";
    }

    strncpy(ssid, ssid_list[idx], sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    strncpy(password, password_list[idx], sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';

    attempt_wifi_connection();
    return "/index.shtml";
}

static const char *clear_cgi_handler(__unused int iIndex, __unused int iNumParams, __unused char *pcParam[], __unused char *pcValue[]) {
    printf("clear_cgi_handler called\n");
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    save_credentials("", "");
    read_credentials();
    return "/index.shtml";
}

// Appends s to dst with HTML special chars escaped. Writes at most dst_size-1
// chars plus a null terminator. Returns number of chars written (excl. null).
static int html_escape(char *dst, size_t dst_size, const char *s) {
    size_t di = 0;
    for (; *s && di + 1 < dst_size; s++) {
        const char *rep;
        switch (*s) {
            case '&':  rep = "&amp;";  break;
            case '<':  rep = "&lt;";   break;
            case '>':  rep = "&gt;";   break;
            case '"':  rep = "&quot;"; break;
            case '\'': rep = "&#39;";  break;
            default:
                dst[di++] = *s;
                continue;
        }
        size_t rlen = strlen(rep);
        if (di + rlen >= dst_size) break;   // not enough room for the whole entity
        memcpy(dst + di, rep, rlen);
        di += rlen;
    }
    dst[di] = '\0';
    return (int)di;
}

// Note that the buffer size is limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN, so use LWIP_HTTPD_SSI_MULTIPART to return larger amounts of data
static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
    , uint16_t current_tag_part, uint16_t *next_tag_part
#endif
) {
    int printed = 0;
    switch (iIndex) {
        case 0: { // wifilist
            if (num_credentials == 0) {
                printed = snprintf(pcInsert, iInsertLen, "<p>No saved networks</p>");
                break;
            }
            int i = current_tag_part;
            char esc[sizeof(ssid_list[0]) * 6];   // worst case: every char -> &quot; (6 chars)
            html_escape(esc, sizeof(esc), ssid_list[i]);
            printed = snprintf(pcInsert, iInsertLen,
                "<label><input type=\"radio\" name=\"index\" value=\"%i\"%s> SSID: %s</label><br>",
                i, (i == 0 ? " checked" : ""), esc);
            if (i + 1 < num_credentials) {
                *next_tag_part = i + 1;
            }
            break;
        }
        case 1: { // ssid
            char esc[sizeof(ssid) * 6];
            html_escape(esc, sizeof(esc), ssid);
            printed = snprintf(pcInsert, iInsertLen, "%s", esc);
            break;
        }
        case 2: { // password
            char esc[sizeof(password) * 6];
            html_escape(esc, sizeof(esc), password);
            printed = snprintf(pcInsert, iInsertLen, "%s", esc);
            break;
        }
        default: { // unknown tag
            printed = 0;
            break;
        }
    }
    return printed;
}