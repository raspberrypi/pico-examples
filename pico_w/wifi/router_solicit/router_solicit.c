/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * router_solicitation — Pico W  (no lwIP)
 *
 * Connects to Wi-Fi, builds a raw Ethernet frame containing an ICMPv6
 * Router Solicitation, sends it through the CYW43 driver, and prints
 * the Router Advertisement that comes back.
 *
 * Everything is done by hand: Ethernet header, IPv6 header, ICMPv6
 * payload, and the pseudo-header checksum.  The only thing the CYW43
 * chip adds is the 802.11 encapsulation and the Ethernet FCS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#ifndef WIFI_SSID
#error define WIFI_SSID
#endif

#ifndef WIFI_PASSWORD
#error define WIFI_PASSWORD
#endif

#define TIMEOUT_MS      10000

// Protocol constants
#define ETHERTYPE_IPV6       0x86DDu
#define IPV6_NEXT_ICMPV6     58
#define ICMPV6_TYPE_RS       133
#define ICMPV6_TYPE_RA       134

#define ND_OPT_SRC_LLADDR     1
#define ND_OPT_PREFIX_INFO     3
#define ND_OPT_MTU             5
#define ND_OPT_RDNSS          25

#define ETH_HDR_LEN    14
#define IPV6_HDR_LEN   40

// Receive buffer (shared with the driver callback)
#define RX_BUF_SIZE  1514

static uint8_t         g_rx_buf[RX_BUF_SIZE];
static volatile size_t g_rx_len   = 0;
static volatile bool   g_rx_ready = false;
static bool            g_link_up;

// Our addresses
static uint8_t g_mac[6];
static uint8_t g_link_local[16];

// Byte-order helpers
static inline uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

// IPv6 formatting
static void fmt_ipv6(const uint8_t *a, char *buf, size_t len)
{
    snprintf(buf, len,
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
             a[0],  a[1],  a[2],  a[3],
             a[4],  a[5],  a[6],  a[7],
             a[8],  a[9],  a[10], a[11],
             a[12], a[13], a[14], a[15]);
}

// Derive a link-local IPv6 address from MAC (EUI-64)
static void mac_to_link_local(const uint8_t *mac, uint8_t *out)
{
    memset(out, 0, 16);
    out[0]  = 0xFE;
    out[1]  = 0x80;
    // bytes 2-7 stay zero
    out[8]  = mac[0] ^ 0x02;          // flip the Universal/Local bit
    out[9]  = mac[1];
    out[10] = mac[2];
    out[11] = 0xFF;
    out[12] = 0xFE;
    out[13] = mac[3];
    out[14] = mac[4];
    out[15] = mac[5];
}

// ICMPv6 checksum (RFC 2460 §8.1)
static uint16_t icmpv6_checksum(const uint8_t *src16,
                                const uint8_t *dst16,
                                const uint8_t *payload,
                                uint16_t       payload_len)
{
    uint32_t sum = 0;

    // pseudo-header: source address
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((src16[i] << 8) | src16[i + 1]);

    // pseudo-header: destination address
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((dst16[i] << 8) | dst16[i + 1]);

    // pseudo-header: upper-layer packet length (32-bit field)
    sum += payload_len;

    // pseudo-header: next header = 58
    sum += IPV6_NEXT_ICMPV6;

    // payload (with checksum field treated as zero)
    for (int i = 0; i < payload_len - 1; i += 2)
        sum += (uint16_t)((payload[i] << 8) | payload[i + 1]);
    if (payload_len & 1)
        sum += (uint16_t)(payload[payload_len - 1] << 8);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}

// Parse and pretty-print a Router Advertisement
static void parse_ra(const uint8_t *data, uint16_t len)
{
    if (len < 16) {
        printf("  Packet too short for an RA (%u bytes)\n", len);
        return;
    }

    uint8_t  hop_limit      = data[4];
    uint8_t  flags           = data[5];
    uint16_t router_lifetime = rd16(&data[6]);
    uint32_t reachable_time  = rd32(&data[8]);
    uint32_t retrans_timer   = rd32(&data[12]);

    printf("\n--- Router Advertisement ---\n");
    printf("  Hop Limit      : %u\n",    hop_limit);
    printf("  Managed (M)    : %s\n",    (flags & 0x80) ? "true" : "false");
    printf("  Other   (O)    : %s\n",    (flags & 0x40) ? "true" : "false");
    printf("  Router Lifetime: %u s\n",  router_lifetime);
    printf("  Reachable Time : %u ms\n", reachable_time);
    printf("  Retrans Timer  : %u ms\n", retrans_timer);

    uint16_t off = 16;
    char abuf[48];

    while (off + 2 <= len) {
        uint8_t  otype  = data[off];
        uint8_t  olen8  = data[off + 1];
        if (olen8 == 0) break;
        uint16_t obytes = (uint16_t)olen8 * 8;
        if (off + obytes > len) break;

        switch (otype) {

        case ND_OPT_SRC_LLADDR:
            if (obytes >= 8) {
                const uint8_t *m = &data[off + 2];
                printf("  Src MAC        : "
                       "%02x:%02x:%02x:%02x:%02x:%02x\n",
                       m[0], m[1], m[2], m[3], m[4], m[5]);
            }
            break;

        case ND_OPT_PREFIX_INFO:
            if (obytes >= 32) {
                uint8_t  plen   = data[off + 2];
                uint8_t  pflags = data[off + 3];
                uint32_t valid  = rd32(&data[off + 4]);
                uint32_t pref   = rd32(&data[off + 8]);
                fmt_ipv6(&data[off + 16], abuf, sizeof(abuf));
                printf("  Prefix         : %s/%u\n", abuf, plen);
                printf("    On-Link  (L) : %s\n",
                       (pflags & 0x80) ? "true" : "false");
                printf("    Auto     (A) : %s\n",
                       (pflags & 0x40) ? "true" : "false");
                printf("    Valid        : %u s\n", valid);
                printf("    Preferred    : %u s\n", pref);
            }
            break;

        case ND_OPT_MTU:
            if (obytes >= 8) {
                uint32_t mtu = rd32(&data[off + 4]);
                printf("  MTU            : %u\n", mtu);
            }
            break;

        case ND_OPT_RDNSS:
            if (obytes >= 24) {
                uint32_t lifetime = rd32(&data[off + 4]);
                int n_addrs = (obytes - 8) / 16;
                printf("  RDNSS (ttl=%u s):\n", lifetime);
                for (int i = 0; i < n_addrs; i++) {
                    fmt_ipv6(&data[off + 8 + i * 16], abuf, sizeof(abuf));
                    printf("    %s\n", abuf);
                }
            }
            break;

        default:
            printf("  Option type=%u  len=%u bytes\n", otype, obytes);
            break;
        }

        off += obytes;
    }
}

/*  CYW43 Ethernet receive callback
 *
 * Called by the CYW43 driver every time a frame arrives from the Wi-Fi
 * chip.  This may run in interrupt context (depending on the SDK's
 * async_context variant), so we just do a quick filter and memcpy here;
 * all the heavy printing happens in the main loop.
 * */
void cyw43_cb_process_ethernet(void *cb_data, int itf,
                               size_t len, const uint8_t *buf)
{
    (void)cb_data;
    (void)itf;

    // Already have an unprocessed RA — skip.
    if (g_rx_ready)
        return;

    // Minimum: Eth(14) + IPv6(40) + ICMPv6 RA fixed header(16)
    if (len < ETH_HDR_LEN + IPV6_HDR_LEN + 16)
        return;

    // Quick protocol filter (no printf, no malloc)

    // EtherType must be IPv6
    if (rd16(&buf[12]) != ETHERTYPE_IPV6)
        return;

    const uint8_t *ipv6 = buf + ETH_HDR_LEN;

    // Version nibble must be 6
    if ((ipv6[0] >> 4) != 6)
        return;

    // Next Header must be ICMPv6
    if (ipv6[6] != IPV6_NEXT_ICMPV6)
        return;

    // ICMPv6 type must be RA (134)
    const uint8_t *icmpv6 = ipv6 + IPV6_HDR_LEN;
    if (icmpv6[0] != ICMPV6_TYPE_RA)
        return;

    // Looks like an RA — stash for the main loop
    size_t copy = (len <= RX_BUF_SIZE) ? len : RX_BUF_SIZE;
    memcpy(g_rx_buf, buf, copy);
    g_rx_len   = copy;
    g_rx_ready = true;                 // atomic on Cortex-M0+
}

void cyw43_cb_tcpip_set_link_up(__unused cyw43_t *self, __unused int itf) {
    g_link_up = true;
}

void cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf) {
    g_link_up = false;
}

int cyw43_tcpip_link_status(cyw43_t *self, int itf) {
    if (g_link_up) {
        return CYW43_LINK_UP;
    }
    return cyw43_wifi_link_status(self, itf);
}

// This is only called if cyw43_send_ethernet pass true for is_pbuf
struct pbuf;
uint16_t pbuf_copy_partial(__unused const struct pbuf *p, __unused void *dataptr, __unused uint16_t len, __unused uint16_t offset) {
    panic_unsupported();
}

// Build a complete Ethernet frame containing an ICMPv6 RS
static bool send_rs(void)
{
    // ff02::2  (all-routers link-local multicast)
    static const uint8_t dst_ip[16] = {
        0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };

    // IPv6 multicast → Ethernet: 33:33 + last four bytes of the group
    static const uint8_t dst_mac[6] = {
        0x33, 0x33, 0x00, 0x00, 0x00, 0x02
    };

    /*
     * RS layout (16 bytes):
     *   0      Type        133
     *   1      Code        0
     *   2-3    Checksum    (computed below)
     *   4-7    Reserved    0
     *  --- Source Link-Layer Address option (RFC 4861 §4.6.1) ---
     *   8      Type        1
     *   9      Length       1   (units of 8 bytes)
     *  10-15   MAC address
     *
     * Including the SLLA option tells the router our MAC so it can
     * unicast the RA straight back to us.
     */
    const uint16_t icmpv6_len = 16;

    // Total frame = Eth(14) + IPv6(40) + ICMPv6(16) = 70 bytes
    uint8_t frame[ETH_HDR_LEN + IPV6_HDR_LEN + 16];
    memset(frame, 0, sizeof(frame));

    // Ethernet header
    uint8_t *eth = frame;
    memcpy(&eth[0], dst_mac, 6);              // destination MAC
    memcpy(&eth[6], g_mac,   6);              // source MAC
    wr16(&eth[12], ETHERTYPE_IPV6);           // EtherType

    // IPv6 header
    uint8_t *ip = frame + ETH_HDR_LEN;
    ip[0] = 0x60;                             // version 6, TC = 0
    // ip[1..3] = 0 — traffic class (cont.) + flow label
    wr16(&ip[4], icmpv6_len);                 // payload length
    ip[6] = IPV6_NEXT_ICMPV6;                 // next header
    ip[7] = 255;                              // hop limit (RFC 4861)
    memcpy(&ip[8],  g_link_local, 16);        // source address
    memcpy(&ip[24], dst_ip,       16);        // destination address

    // ICMPv6 Router Solicitation
    uint8_t *rs = ip + IPV6_HDR_LEN;
    rs[0] = ICMPV6_TYPE_RS;                   // type
    // rs[1] = 0;   code
    // rs[2..3] = 0;  checksum placeholder
    // rs[4..7] = 0;  reserved

    // Source Link-Layer Address option
    rs[8]  = ND_OPT_SRC_LLADDR;              // option type
    rs[9]  = 1;                               // length = 1 × 8 bytes
    memcpy(&rs[10], g_mac, 6);                // our MAC

    // Checksum
    uint16_t cksum = icmpv6_checksum(g_link_local, dst_ip, rs, icmpv6_len);
    wr16(&rs[2], cksum);

    // Transmit
    int ret = cyw43_send_ethernet(&cyw43_state, CYW43_ITF_STA,
                                  sizeof(frame), frame, false);
    if (ret != 0) {
        printf("cyw43_send_ethernet failed (%d)\n", ret);
        return false;
    }
    return true;
}

// Entry point
int main(void)
{
    stdio_init_all();
    sleep_ms(2000);                    // let USB-serial attach

    printf("\n=============================\n");
    printf(" Pico W  Router Solicitation\n");
    printf(" (raw Ethernet - no lwIP)\n");
    printf("=============================\n\n");

    // 1. Initialise the CYW43 Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("cyw43_arch_init failed\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    // 2. Read our MAC and derive the link-local address
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, g_mac);
    mac_to_link_local(g_mac, g_link_local);

    char abuf[48];
    printf("MAC        : %02x:%02x:%02x:%02x:%02x:%02x\n",
           g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
    fmt_ipv6(g_link_local, abuf, sizeof(abuf));
    printf("Link-local : %s\n\n", abuf);

    // 3. Connect to Wi-Fi
    printf("Connecting to \"%s\" ...\n", WIFI_SSID);
    int rc = cyw43_arch_wifi_connect_timeout_ms(
                 WIFI_SSID, WIFI_PASSWORD,
                 CYW43_AUTH_WPA2_AES_PSK, 30000);
    if (rc) {
        printf("Wi-Fi connection failed (%d)\n", rc);
        cyw43_arch_disable_sta_mode();
        cyw43_arch_deinit();
        return 1;
    }
    printf("Associated.\n");

    /*  4. Register the IPv6 all-nodes multicast MAC
     *
     * RAs are normally sent to ff02::1 which maps to Ethernet multicast
     * 33:33:00:00:00:01.  The Wi-Fi chip needs to know we want those
     * frames; without this it may silently drop them.
     *
     * cyw43_wifi_update_multicast_filter() is part of the CYW43 driver
     * (not lwIP).
     * */
    uint8_t all_nodes_mac[6] = { 0x33, 0x33, 0x00, 0x00, 0x00, 0x01 };
    cyw43_wifi_update_multicast_filter(&cyw43_state, all_nodes_mac, true);

    sleep_ms(500);                     // let the association settle

    // 5. Send the Router Solicitation
    printf("Sending RS to ff02::2 ...\n");
    if (!send_rs()) {
        cyw43_arch_disable_sta_mode();
        cyw43_arch_deinit();
        return 1;
    }

    // 6. Wait for the RA
    printf("Waiting for Router Advertisement ...\n");

    absolute_time_t deadline = make_timeout_time_ms(TIMEOUT_MS);
    while (!g_rx_ready &&
           absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
#endif
        sleep_ms(1);
    }

    if (g_rx_ready) {
        // Parse the stashed frame in main-thread context where printf
        // is safe regardless of the async_context variant.
        const uint8_t *ipv6   = g_rx_buf + ETH_HDR_LEN;
        uint16_t payload_len  = rd16(&ipv6[4]);
        const uint8_t *icmpv6 = ipv6 + IPV6_HDR_LEN;

        fmt_ipv6(&ipv6[8], abuf, sizeof(abuf));
        printf("\nReceived RA (%u bytes) from %s\n", payload_len, abuf);

        // Also print the router's Ethernet MAC for reference
        const uint8_t *src_mac = g_rx_buf + 6;
        printf("Router MAC : %02x:%02x:%02x:%02x:%02x:%02x\n",
               src_mac[0], src_mac[1], src_mac[2],
               src_mac[3], src_mac[4], src_mac[5]);

        parse_ra(icmpv6, payload_len);
    } else {
        printf("\nNo RA received within %d ms.\n", TIMEOUT_MS);
        printf("Check that your router has IPv6 enabled.\n");
    }

    // 7. Tidy up
    cyw43_arch_disable_sta_mode();
    cyw43_arch_deinit();
    printf("\nDone.\n");
    return 0;
}
