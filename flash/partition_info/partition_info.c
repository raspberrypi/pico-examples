/**
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/bootrom.h>
#include "boot/picobin.h"


#define PART_LOC_FIRST(x) ( ((x) & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB )
#define PART_LOC_LAST(x)  ( ((x) & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS)  >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB )

/*
 * Stores partition table information and data read status
 */
typedef struct {
    int flags;
    uint32_t *table;
    size_t table_size;
    int partition_count;
    uint32_t unpartitioned_space_first_sector;
    uint32_t unpartitioned_space_last_sector;
    uint32_t permission;
    int current_partition;
    size_t idx;
} pico_partition_t;

/*
 * Stores information on each partition
 */
typedef struct {
    uint32_t first_sector;
    uint32_t last_sector;
    uint32_t permission;
    uint64_t partition_id;
    char name[127 + 1];
} pico_partition_entry_t;


/*
 * Read the partition table information.
 * See the RP2350 datasheet 5.1.2, 5.4.8.16 for flags and structures that can be specified.
 */
int pico_partitions_open(pico_partition_t *pt, int flags) {
    pt->flags = flags;
    static __attribute__((aligned(4))) uint32_t workarea[816] = {0};
    int rc = rom_get_partition_table_info(workarea, sizeof(workarea), flags);
    if (rc < 0) {
        return rc;
    }
    pt->table = workarea;
    pt->table_size = rc * 4;  // word to bytes
    pt->partition_count = ((uint8_t*)workarea)[4];
    uint32_t location = workarea[2];
    pt->unpartitioned_space_first_sector = PART_LOC_FIRST(location);
    pt->unpartitioned_space_last_sector = PART_LOC_LAST(location);
    pt->permission = workarea[3];
    pt->idx = 4;
    pt->current_partition = 0;

    return 0;
}

/*
 * Parse a partition table and extract information
 */
size_t pico_partitions_parse(pico_partition_t *pt, pico_partition_entry_t *p) {
    size_t idx = pt->idx;
    if (pt->flags & PT_INFO_PARTITION_LOCATION_AND_FLAGS) {
        uint32_t location = pt->table[idx++];
        p->first_sector = PART_LOC_FIRST(location);
        p->last_sector  = PART_LOC_LAST(location);
        p->permission   = pt->table[idx++];
    } else {
        p->first_sector = 0;
        p->last_sector = 0;
        p->permission = 0;
    }

    if (pt->flags & PT_INFO_PARTITION_ID) {
        uint32_t id_low  = pt->table[idx++];
        uint32_t id_high = pt->table[idx++];
        p->partition_id = ((uint64_t)id_high << 32) | id_low;
    } else {
        p->partition_id = 0;
    }

    if (pt->flags & PT_INFO_PARTITION_NAME) {
        uint8_t *name_field = (uint8_t *)&pt->table[idx];
        uint8_t name_length = name_field[0];
        uint8_t *name_src = name_field + 1;
        memcpy(p->name, name_src, name_length);
        p->name[name_length] = '\0';
        size_t total_name_bytes = 1 + name_length;
        size_t padded_bytes = (total_name_bytes + 3) & ~((size_t)3);
        idx += padded_bytes / 4;
    } else {
        p->name[0] = '\0';
    }
    return idx;
}

/*
 * Extract one partition information
 */
bool pico_partitions_next(pico_partition_t *pt, pico_partition_entry_t *p) {
    if (pt->current_partition >= pt->partition_count) {
        return false;
    }
    pt->idx = pico_partitions_parse(pt, p);
    pt->current_partition++;
    return true;
}

int main() {
    stdio_init_all();

    pico_partition_t pt;
    pico_partitions_open(&pt, PT_INFO_PT_INFO | PT_INFO_PARTITION_LOCATION_AND_FLAGS |
                              PT_INFO_PARTITION_ID | PT_INFO_PARTITION_NAME);
    printf("un-partitioned_space: S(%s%s) NSBOOT(%s%s) NS(%s%s)\n",
           (pt.permission & PICOBIN_PARTITION_PERMISSION_S_R_BITS ? "r" : ""),
           (pt.permission & PICOBIN_PARTITION_PERMISSION_S_W_BITS ? "w" : ""),
           (pt.permission & PICOBIN_PARTITION_PERMISSION_NSBOOT_R_BITS ? "r" : ""),
           (pt.permission & PICOBIN_PARTITION_PERMISSION_NSBOOT_W_BITS ? "w" : ""),
           (pt.permission & PICOBIN_PARTITION_PERMISSION_NS_R_BITS ? "r" : ""),
           (pt.permission & PICOBIN_PARTITION_PERMISSION_NS_W_BITS ? "w" : ""));
    printf("patitions:\n");
    pico_partition_entry_t p;
    while (pico_partitions_next(&pt, &p)) {
        printf("%3d:", pt.current_partition - 1);
        if (pt.flags & PT_INFO_PARTITION_LOCATION_AND_FLAGS) {
            printf("    %08x->%08x S(%s%s) NSBOOT(%s%s) NS(%s%s)",
                   p.first_sector * 4096, p.last_sector * 4096,
                   (p.permission & PICOBIN_PARTITION_PERMISSION_S_R_BITS ? "r" : ""),
                   (p.permission & PICOBIN_PARTITION_PERMISSION_S_W_BITS ? "w" : ""),
                   (p.permission & PICOBIN_PARTITION_PERMISSION_NSBOOT_R_BITS ? "r" : ""),
                   (p.permission & PICOBIN_PARTITION_PERMISSION_NSBOOT_W_BITS ? "w" : ""),
                   (p.permission & PICOBIN_PARTITION_PERMISSION_NS_R_BITS ? "r" : ""),
                   (p.permission & PICOBIN_PARTITION_PERMISSION_NS_W_BITS ? "w" : ""));

        }
        if (pt.flags & PT_INFO_PARTITION_ID) {
            printf(", id=%016llx", p.partition_id);
        }
        if (pt.flags & PT_INFO_PARTITION_NAME) {
            printf(", \"%s\"", p.name);
        }
        printf("\n");
    }

    return 0;
}
