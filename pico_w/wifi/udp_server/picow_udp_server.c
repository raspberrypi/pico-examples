#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Connection timeout in ms
#define WIFI_CONNECT_TIMEOUT_MS 30000

static bool wifi_connected = false;
static absolute_time_t connection_start_time;

// Default UDP port
#ifndef UDP_PORT    
#define UDP_PORT 12345
#endif

// Maximum message size
#define MAX_UDP_MSG_SIZE 1024

// Initialize WiFi hardware
int wifi_init(void) {
    // Initialize the CYW43 driver
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43 driver\n");
        return -1;
    }

    // Enable station mode
    cyw43_arch_enable_sta_mode();
    
    printf("WiFi initialized successfully\n");
    return 0;
}

#define WIFI_CONNECT_TIMEOUT_MS 30000

int wifi_connect(const char *ssid, const char *password) {
    if (!ssid || !password) {
        ssid = WIFI_SSID;   
        password = WIFI_PASSWORD;
    }

    printf("Connecting to WiFi network: %s\n", ssid);

    cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);

    int ssid_len = strlen(ssid);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
         CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
        wifi_connected = true;
        wifi_update_led();
    }

    printf("WiFi link is up, starting client...\n");


    printf("DHCP assigned IP: %s\n", ip4addr_ntoa(
                                        netif_ip4_addr(netif_default)));
    printf("Netmask: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif_default)));
    printf("Gateway: %s\n", ip4addr_ntoa(netif_ip4_gw(netif_default)));

    return 0;
}

// Disconnect from WiFi
void wifi_disconnect(void) {
    if (wifi_connected) {
        cyw43_arch_wifi_connect_async("", "", CYW43_AUTH_OPEN);
        wifi_connected = false;
        wifi_update_led();
        printf("WiFi disconnected\n");
    }
}

// Check WiFi connection status
bool wifi_is_connected(void) {
    // Check hardware link status
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    
    if (link_status == CYW43_LINK_UP) {
        wifi_connected = true;
        wifi_update_led();
        return true;
    } else {
        wifi_connected = false;
        wifi_update_led();
        return false;
    }
}

// Get WiFi connection status string
const char* wifi_get_status_string(void) {
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    
    switch (link_status) {
        case CYW43_LINK_DOWN:
            return "Link Down";
        case CYW43_LINK_JOIN:
            return "Joining";
        case CYW43_LINK_NOIP:
            return "Connected (No IP)";
        case CYW43_LINK_UP:
            return "Connected";
        case CYW43_LINK_FAIL:
            return "Connection Failed";
        case CYW43_LINK_NONET:
            return "No Network";
        case CYW43_LINK_BADAUTH:
            return "Bad Authentication";
        default:
            return "Unknown";
    }
}

// Get IP address as string
const char* wifi_get_ip_address(void) {
    if (wifi_connected) {
        return ip4addr_ntoa(netif_ip4_addr(netif_default));
    }
    printf("Not connected.");
    return "0.0.0.0";
}

// Attempt to reconnect if connection is lost
int wifi_check_and_reconnect(void) {
    if (!wifi_is_connected()) {
        printf("WiFi connection lost, attempting to reconnect...\n");
        return wifi_connect(NULL, NULL);
    }
    wifi_update_led();
    return 0;
}

// Clean up WiFi resources
void wifi_deinit(void) {
    wifi_disconnect();
    wifi_update_led();
    cyw43_arch_deinit();
    printf("WiFi deinitialized\n");
}

// Set WiFi LED based on connection status
void wifi_update_led(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, wifi_connected ? 1 : 0);
}

// Periodic WiFi maintenance (call from main loop)
void wifi_poll(void) {
    // Update LED status
    wifi_update_led();
    
    // Check connection periodically
    static absolute_time_t last_check = 0;
    absolute_time_t now = get_absolute_time();
    
    if (absolute_time_diff_us(last_check, now) > 5000000) {
        // Check every 5 seconds
        wifi_check_and_reconnect();
        last_check = now;
    }
    
    // Poll CYW43 driver
    cyw43_arch_poll();
}

// UDP control block
static struct udp_pcb *udp_pcb = NULL;
static uint16_t listen_port = UDP_PORT;
static udp_data_callback_t data_callback = NULL;
static bool udp_server_active = false;

// Buffer for incoming data
static uint8_t rx_buffer[MAX_UDP_MSG_SIZE];

// Callback function for receiving UDP data
static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, 
                              const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        // Copy data to buffer
        size_t data_len = p->tot_len;
        if (data_len > MAX_UDP_MSG_SIZE) {
            data_len = MAX_UDP_MSG_SIZE;
        }
        
        // Copy pbuf data to our buffer
        pbuf_copy_partial(p, rx_buffer, data_len, 0);
        
        printf("UDP received %d bytes from %s:%d\n", data_len, 
               ip4addr_ntoa(addr), port);
        
        // Call user callback if registered
        if (data_callback) {
            data_callback(rx_buffer, data_len, addr, port);
        }
        
        // Free the pbuf
        pbuf_free(p);
    }
}

// Initialize UDP handler
int udp_server_init(udp_data_callback_t callback) {
    printf("Initializing UDP handler...\n");
    
    if (!callback) {
        printf("Error: NULL callback passed to udp_server_init\n");
        return -1;
    }

    data_callback = callback; // Save the app-level callback
    return udp_server_start(UDP_PORT, callback); // Start server via saved port
}


// Start UDP server on specified port
int udp_server_start(uint16_t port, udp_data_callback_t callback) {
    if (udp_server_active) {
        printf("UDP server already active\n");
        return -1;
    }

    // Create PCB
    udp_pcb = udp_new();
    if (!udp_pcb) {
        printf("Failed to create UDP PCB\n");
        return -1;
    }

    if (udp_bind(udp_pcb, IP_ADDR_ANY, port) != ERR_OK) {
        printf("Failed to bind UDP port %d\n", port);
        udp_remove(udp_pcb);
        udp_pcb = NULL;
        return -1;
    }

    // Register internal receive callback
    udp_recv(udp_pcb, udp_recv_callback, NULL);

    listen_port = port;
    udp_server_active = true;

    printf("UDP server started on port %d\n", port);
    return 0;
}


// Stop UDP server
void udp_server_stop(void) {
    if (udp_pcb) {
        udp_remove(udp_pcb);
        udp_pcb = NULL;
    }
    
    udp_server_active = false;
    data_callback = NULL;
    printf("UDP server stopped\n");
}

// Send UDP data to specified address and port
int udp_send_data(const ip_addr_t *dest_addr, uint16_t dest_port, 
                  const uint8_t *data, size_t data_len) {
    
    if (data_len > MAX_UDP_MSG_SIZE) {
        printf("Data too large for UDP packet\n");
        return -1;
    }
    
    // Create a temporary PCB for sending if no server running
    struct udp_pcb *send_pcb = udp_pcb;
    bool temp_pcb = false;
    
    if (!send_pcb) {
        send_pcb = udp_new();
        if (!send_pcb) {
            printf("Failed to create UDP PCB for sending\n");
            return -1;
        }
        temp_pcb = true;
    }
    
    // Allocate pbuf for data
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);
    if (!p) {
        printf("Failed to allocate pbuf\n");
        if (temp_pcb) {
            udp_remove(send_pcb);
        }
        return -1;
    }
    
    // Copy data to pbuf
    memcpy(p->payload, data, data_len);
    
    // Send the data
    err_t err = udp_sendto(send_pcb, p, dest_addr, dest_port);
    
    // Free pbuf
    pbuf_free(p);
    
    // Clean up temporary PCB
    if (temp_pcb) {
        udp_remove(send_pcb);
    }
    
    if (err != ERR_OK) {
        printf("Failed to send UDP data, error: %d\n", err);
        return -1;
    }
    
    printf("UDP sent %d bytes to %s:%d\n", data_len, ip4addr_ntoa(dest_addr), 
           dest_port);
    return 0;
}

// Send UDP data to IP address string
int udp_send_to_ip_string(const char *ip_str, uint16_t dest_port, 
                          const uint8_t *data, size_t data_len) {
    ip_addr_t dest_addr;
    
    if (!ip4addr_aton(ip_str, &dest_addr)) {
        printf("Invalid IP address: %s\n", ip_str);
        return -1;
    }
    
    return udp_send_data(&dest_addr, dest_port, data, data_len);
}

// Get current listen port
uint16_t udp_get_listen_port(void) {
    return listen_port;
}

// Check if UDP server is active
bool udp_is_server_active(void) {
    return udp_server_active;
}

// Process UDP (call from main loop)
void udp_poll(void) {
    // lwIP polling is handled by cyw43_arch_poll() in wifi_handler
    // This function can be used for any UDP-specific maintenance
    
    // Check if we need to restart server after WiFi reconnection
    if (!udp_server_active && wifi_is_connected() && data_callback) {
        printf("WiFi reconnected, restarting UDP server...\n");
        udp_server_start(listen_port, data_callback);
    }
}

// Cleanup UDP resources
void udp_deinit(void) {
    udp_server_stop();
    printf("UDP handler deinitialized\n");
}

int main() {
    stdio_init_all();
    sleep_ms(5000);
    printf("Pico UDP Server Demo Starting...\n");
    
    // Initialize WiFi
    if (wifi_init() != 0) return -1;
    if (wifi_connect(NULL, NULL) != 0) {
        wifi_deinit();
        return -1;
    }

    printf("IP Address: %s\n", wifi_get_ip_address());
    printf("%s\n", wifi_get_status_string());

    // Initialize UDP server
    if (udp_server_init(handle_udp_message) != 0) {
        printf("Failed to start UDP server\n");
        wifi_deinit();
        return -1;
    }

    printf("System ready - waiting for UDP messages...\n");

    // Main loop
    while (1) {
        // Work goes here.
        sleep_ms(10);
    }

    // Cleanup - halts server in event of WiFi loss or loop break
    udp_deinit();
    wifi_deinit();
    return 0;
}
