#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "boot/picobin.h"
#include "boot/uf2.h"


#define PARTITION_EXTRA_FAMILY_ID_MAX      3

typedef struct {
    size_t count;
    char **items;
} uf2_family_ids_t;


uf2_family_ids_t *uf2_family_ids_new(uint32_t flags);
char *uf2_family_ids_join(const uf2_family_ids_t *ids, const char *sep);
void uf2_family_ids_free(uf2_family_ids_t *ids);

void uf2_family_ids_add_extra_family_id(uf2_family_ids_t *ids, uint32_t family_id);
