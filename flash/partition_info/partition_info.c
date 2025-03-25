/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"


#define PART_LOC_FIRST(x) ( ((x) & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB )
#define PART_LOC_LAST(x)  ( ((x) & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS)  >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB )

#define PARTITION_LOCATION_AND_FLAGS_SIZE  2
#define PARTITION_ID_SIZE                  2
#define PARTITION_NAME_MAX                 127
#define PARTITION_TABLE_FIXED_INFO_SIZE    (4 + PARTITION_TABLE_MAX_PARTITIONS * (PARTITION_LOCATION_AND_FLAGS_SIZE + PARTITION_ID_SIZE))

/*
 * Stores partition table information and data read status
 */
typedef struct {
    uint32_t table[PARTITION_TABLE_FIXED_INFO_SIZE];
    uint32_t fields;
    int partition_count;
    uint32_t unpartitioned_space_first_sector;
    uint32_t unpartitioned_space_last_sector;
    uint32_t flags_and_permissions;
    int current_partition;
    size_t pos;
    int status;
} pico_partition_table_t;

/*
 * Stores information on each partition
 */
typedef struct {
    uint32_t first_sector;
    uint32_t last_sector;
    uint32_t flags_and_permissions;
    uint64_t partition_id;
    char name[PARTITION_NAME_MAX + 1];  // name length is indicated by 7 bits
} pico_partition_t;


/*
 * Read the partition table information.
 *
 * See the RP2350 datasheet 5.1.2, 5.4.8.16 for flags and structures that can be specified.
 */
int open_partition_table(pico_partition_table_t *pt) {
    // Reads fixed size fields
    int rc = rom_get_partition_table_info(pt->table, sizeof(pt->table),
                                          (PT_INFO_PT_INFO |
                                           PT_INFO_PARTITION_LOCATION_AND_FLAGS |
                                           PT_INFO_PARTITION_ID));
    if (rc < 0) {
        pt->status = rc;
        return rc;
    }

    size_t pos = 0;
    pt->fields = pt->table[pos++];
    pt->partition_count = pt->table[pos++] & 0x000000FF;
    uint32_t location = pt->table[pos++];
    pt->unpartitioned_space_first_sector = PART_LOC_FIRST(location);
    pt->unpartitioned_space_last_sector = PART_LOC_LAST(location);
    pt->flags_and_permissions = pt->table[pos++];
    pt->current_partition = 0;
    pt->pos = pos;
    pt->status = 0;

    return 0;
}

/*
 * Extract each partition information
 */
bool read_next_partition(pico_partition_table_t *pt, pico_partition_t *p) {
    if (pt->current_partition >= pt->partition_count) {
        return false;
    }

    size_t pos = pt->pos;
    uint32_t location = pt->table[pos++];
    p->first_sector = PART_LOC_FIRST(location);
    p->last_sector = PART_LOC_LAST(location);
    p->flags_and_permissions = pt->table[pos++];

    if (p->flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS) {
        uint32_t id_low  = pt->table[pos++];
        uint32_t id_high = pt->table[pos++];
        p->partition_id = ((uint64_t)id_high << 32) | id_low;
    } else {
        p->partition_id = 0;
    }
    pt->pos = pos;

    if (p->flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_NAME_BITS) {
        // Read variable length fields
        uint32_t name_buf[(PARTITION_NAME_MAX + 1) / sizeof(uint32_t)] = {0};
        int rc = rom_get_partition_table_info(name_buf, sizeof(name_buf),
                                              (pt->current_partition << 24 |
                                               PT_INFO_SINGLE_PARTITION |
                                               PT_INFO_PARTITION_NAME));
        if (rc < 0) {
            pt->status = rc;
            return false;
        }
        uint32_t __attribute__((unused)) fields = name_buf[0];
        uint8_t *name_buf_u8 = (uint8_t *)&name_buf[1];
        uint8_t name_length = *name_buf_u8++;
        memcpy(p->name, name_buf_u8, name_length);
        p->name[name_length] = '\0';
    } else {
        p->name[0] = '\0';
    }

    pt->current_partition++;
    return true;
}

int main() {
    stdio_init_all();

    pico_partition_table_t pt;
    open_partition_table(&pt);
    printf("un-partitioned_space: S(%s%s) NSBOOT(%s%s) NS(%s%s)\n",
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_S_R_BITS ? "r" : ""),
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_S_W_BITS ? "w" : ""),
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NSBOOT_R_BITS ? "r" : ""),
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NSBOOT_W_BITS ? "w" : ""),
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NS_R_BITS ? "r" : ""),
           (pt.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NS_W_BITS ? "w" : ""));
    printf("partitions:\n");
    pico_partition_t p;
    while (read_next_partition(&pt, &p)) {
        printf("%3d:", pt.current_partition - 1);
        printf("    %08x->%08x S(%s%s) NSBOOT(%s%s) NS(%s%s)",
               p.first_sector * 4096, (p.last_sector + 1) * 4096,
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_S_R_BITS ? "r" : ""),
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_S_W_BITS ? "w" : ""),
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NSBOOT_R_BITS ? "r" : ""),
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NSBOOT_W_BITS ? "w" : ""),
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NS_R_BITS ? "r" : ""),
               (p.flags_and_permissions & PICOBIN_PARTITION_PERMISSION_NS_W_BITS ? "w" : ""));
        if (p.flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS) {
            printf(", id=%016llx", p.partition_id);
        }
        if (p.flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_NAME_BITS) {
            printf(", \"%s\"", p.name);
        }
        printf("\n");
    }
    if (pt.status != 0) {
        panic("rom_get_partition_table_info returned %d", pt.status);
    }

    return 0;
}
