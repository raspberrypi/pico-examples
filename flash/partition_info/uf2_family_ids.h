#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "boot/picobin.h"


typedef struct {
    size_t count;
    char **items;
} uf2_family_ids_t;


uf2_family_ids_t *uf2_family_ids_new(void);
void uf2_family_ids_add(uf2_family_ids_t *ids, const char *str);
char *uf2_family_ids_join(const uf2_family_ids_t *ids, const char *sep);
void uf2_family_ids_free(uf2_family_ids_t *ids);

void uf2_family_ids_add_default_families(uf2_family_ids_t *ids, uint32_t flags);
void uf2_family_ids_add_extra_family_id(uf2_family_ids_t *ids, uint32_t family_id);
