/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/sha256.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "boot/picoboot.h"
#include "boot/uf2.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 4242
// #define DEBUG_printf(...) printf(__VA_ARGS__)
#define DEBUG_printf(...)
#define BUF_SIZE 2048
#define POLL_TIME_S 5

#define FLASH_SECTOR_ERASE_SIZE 4096u

typedef struct TCP_UPDATE_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    __attribute__((aligned(4))) uint8_t buffer_sent[SHA256_RESULT_BYTES];
    __attribute__((aligned(4))) uint8_t buffer_recv[BUF_SIZE];
    int sent_len;
    int recv_len;
    int num_blocks;
    int blocks_done;
    uint32_t family_id;
    uint32_t flash_update;
    int32_t write_offset;
    uint32_t write_size;
    uint32_t highest_erased_sector;
} TCP_UPDATE_SERVER_T;

typedef struct uf2_block uf2_block_t;

static TCP_UPDATE_SERVER_T* tcp_update_server_init(void) {
    TCP_UPDATE_SERVER_T *state = calloc(1, sizeof(TCP_UPDATE_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}


static __attribute__((aligned(4))) uint8_t workarea[4 * 1024];

static err_t tcp_update_server_close(void *arg) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

static err_t tcp_update_server_result(void *arg, int status) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_update_server_close(arg);
}

static err_t tcp_update_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    DEBUG_printf("tcp_update_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= SHA256_RESULT_BYTES) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_update_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;

    state->sent_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n", SHA256_RESULT_BYTES);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, SHA256_RESULT_BYTES, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_update_server_result(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_update_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    if (!p) {
        return tcp_update_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_update_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE) {

        for (int i=0; i < BUF_SIZE/sizeof(uf2_block_t); i++) {
            // check it matches
            uf2_block_t* block;
            block = (uf2_block_t*)(state->buffer_recv + i * sizeof(uf2_block_t));

            if (state->num_blocks == 0) {
                state->num_blocks = block->num_blocks;
                state->family_id = block->file_size; // or familyID;

                resident_partition_t uf2_target_partition;
                rom_flash_flush_cache();
                rom_get_uf2_target_partition(workarea, sizeof(workarea), state->family_id, &uf2_target_partition);
                printf("Code Target partition is %lx %lx\n", uf2_target_partition.permissions_and_location, uf2_target_partition.permissions_and_flags);

                uint16_t first_sector_number = (uf2_target_partition.permissions_and_location & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB;
                uint16_t last_sector_number = (uf2_target_partition.permissions_and_location & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB;
                uint32_t code_start_addr = first_sector_number * 0x1000;
                uint32_t code_end_addr = (last_sector_number + 1) * 0x1000;
                uint32_t code_size = code_end_addr - code_start_addr;
                printf("Start %lx, End %lx, Size %lx\n", code_start_addr, code_end_addr, code_size);

                state->flash_update = code_start_addr + XIP_BASE;
                state->write_offset = code_start_addr + XIP_BASE - block->target_addr;
                state->write_size = code_size;
                DEBUG_printf("Write Offset %lx, Size %lx\n", state->write_offset, state->write_size);
            }

            if (state->blocks_done != block->block_no) {
                DEBUG_printf("block number mismatch - expected %d, got %d\n", state->blocks_done, block->block_no);
                return tcp_update_server_result(arg, -1);
            }
            if (state->family_id != block->file_size) {
                DEBUG_printf("family id mismatch\n");
                return tcp_update_server_result(arg, -1);
            }
            DEBUG_printf("tcp_update_server_recv buffer ok\n");

            // Write to flash
            struct cflash_flags flags;
            int8_t ret;
            (void)ret;
            if (block->target_addr / FLASH_SECTOR_ERASE_SIZE > state->highest_erased_sector) {
                flags.flags =
                    (CFLASH_OP_VALUE_ERASE << CFLASH_OP_LSB) | 
                    (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
                    (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
                ret = rom_flash_op(flags,
                    block->target_addr + state->write_offset,
                    FLASH_SECTOR_ERASE_SIZE, NULL);
                state->highest_erased_sector = block->target_addr / FLASH_SECTOR_ERASE_SIZE;
                DEBUG_printf("Checked Erase Returned %d, start %x, size %x, highest erased %x\n", ret, block->target_addr + state->write_offset, FLASH_SECTOR_ERASE_SIZE, state->highest_erased_sector);
            }
            flags.flags =
                (CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB) | 
                (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
                (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
            ret = rom_flash_op(flags,
                block->target_addr + state->write_offset,
                256, (void*)block->data);
            DEBUG_printf("Checked Program Returned %d, start %x, size %x\n", ret, block->target_addr + state->write_offset, 256);

            // Download complete?
            state->blocks_done++;
            if (state->blocks_done >= state->num_blocks) {
                tcp_update_server_result(arg, 0);
                return ERR_OK;
            }
        }

        // Hash the received data
        pico_sha256_state_t sha_state;
        int rc = pico_sha256_start_blocking(&sha_state, SHA256_BIG_ENDIAN, true); // using some DMA system resources
        hard_assert(rc == PICO_OK);
        pico_sha256_update_blocking(&sha_state, (const uint8_t*)state->buffer_recv, sizeof(state->buffer_recv));

        // Get the result of the sha256 calculation
        sha256_result_t* result;
        result = (sha256_result_t*)state->buffer_sent;
        pico_sha256_finish(&sha_state, result);

        // Send another buffer
        return tcp_update_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

static err_t tcp_update_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_update_server_poll_fn\n");
    return tcp_update_server_result(arg, -1); // no response is an error?
}

static void tcp_update_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_update_server_result(arg, err);
    }
}

static err_t tcp_update_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_update_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_update_server_sent);
    tcp_recv(client_pcb, tcp_update_server_recv);
    tcp_poll(client_pcb, tcp_update_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_update_server_err);

    return ERR_OK;
}

static bool tcp_update_server_open(void *arg) {
    TCP_UPDATE_SERVER_T *state = (TCP_UPDATE_SERVER_T*)arg;
    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_update_server_accept);

    return true;
}

int main() {
    stdio_init_all();

#ifdef __riscv
    // Increased bootrom stack is required for some of the functions in this example
    bootrom_stack_t stack = {
        .base = malloc(0x400),
        .size = 0x400
    };
    rom_set_bootrom_stack(&stack);
#endif

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }

    boot_info_t boot_info = {};
    int ret = rom_get_boot_info(&boot_info);
    printf("Boot partition was %d\n", boot_info.partition);

    if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
        printf("Someone updated into me\n");
        if (boot_info.reboot_params[0]) printf("Flash update base was %x\n", boot_info.reboot_params[0]);
        if (boot_info.tbyb_and_update_info) printf("Update info %x\n", boot_info.tbyb_and_update_info);
        ret = rom_explicit_buy(workarea, sizeof(workarea));
        if (ret) printf("Buy returned %d\n", ret);
        ret = rom_get_boot_info(&boot_info);
        if (boot_info.tbyb_and_update_info) printf("Update info now %x\n", boot_info.tbyb_and_update_info);
    }


    TCP_UPDATE_SERVER_T *state = tcp_update_server_init();
    if (!state) {
        return -1;
    }
    if (!tcp_update_server_open(state)) {
        tcp_update_server_result(state, -1);
        return -1;
    }

    bool led_state = false;
    while(!state->complete) {
        // Do your application code here
        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
        sleep_ms(250);
    }

    cyw43_arch_deinit();
    ret = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, 1000, state->flash_update, 0);
    printf("Done - rebooting for a flash update boot %d\n", ret);
    free(state);
    sleep_ms(2000);
    return 0;
}
