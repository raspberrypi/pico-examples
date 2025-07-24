#include "uf2_family_ids.h"

#define UF2_FAMILY_ID_HEX_SIZE   (2 + 8 * 2 + 1)

static void _add(uf2_family_ids_t *ids, const char *str) {
    ids->items = realloc(ids->items, (ids->count + 1) * sizeof(char *));
    ids->items[ids->count] = strdup(str);
    if (ids->items[ids->count] == NULL) {
        perror("strdup");
        return;
    }
    ids->count++;
}

static void _add_default_families(uf2_family_ids_t *ids, uint32_t flags) {
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_ABSOLUTE_BITS)
        _add(ids, "absolute");
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_RP2040_BITS)
        _add(ids, "rp2040");
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_RP2350_ARM_S_BITS)
        _add(ids, "rp2350-arm-s");
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_RP2350_ARM_NS_BITS)
        _add(ids, "rp2350-arm-ns");
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_RP2350_RISCV_BITS)
        _add(ids, "rp2350-riscv");
    if (flags & PICOBIN_PARTITION_FLAGS_ACCEPTS_DEFAULT_FAMILY_DATA_BITS)
        _add(ids, "data");
}

uf2_family_ids_t *uf2_family_ids_new(uint32_t flags) {
    uf2_family_ids_t *ids = malloc(sizeof(uf2_family_ids_t));
    ids->count = 0;
    ids->items = NULL;
    _add_default_families(ids, flags);
    return ids;
}

char *uf2_family_ids_join(const uf2_family_ids_t *ids, const char *sep) {
    size_t total_length = 0;
    size_t sep_length = strlen(sep);

    for (size_t i = 0; i < ids->count; i++) {
        total_length += strlen(ids->items[i]);
        if (i < ids->count - 1)
            total_length += sep_length;
    }

    char *result = calloc(1, total_length + 1);
    if (!result) {
        perror("calloc");
        return NULL;
    }

    result[0] = '\0';
    for (size_t i = 0; i < ids->count; i++) {
        strcat(result, ids->items[i]);
        if (i < ids->count - 1)
            strcat(result, sep);
    }

    return result;
}

void uf2_family_ids_free(uf2_family_ids_t *ids) {
    for (size_t i = 0; i < ids->count; i++) {
        free(ids->items[i]);
    }
    free(ids->items);
    free(ids);
}

void uf2_family_ids_add_extra_family_id(uf2_family_ids_t *ids, uint32_t family_id) {
    char hex_id[UF2_FAMILY_ID_HEX_SIZE];
    switch (family_id) {
        case CYW43_FIRMWARE_FAMILY_ID:
            _add(ids, "cyw43-firmware");
            break;
        default:
            sprintf(hex_id, "0x%08x", family_id);
            _add(ids, hex_id);
            break;
    }
}
