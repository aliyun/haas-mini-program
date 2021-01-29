/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include "amp_defines.h"
#include "amp_hal_flash.h"
#include "aos/hal/flash.h"

int32_t amp_hal_flash_info_get(amp_partition_id_t in_partition, amp_hal_logic_partition_t *partition)
{
    int32_t ret = -1;
    hal_partition_t tmp_id;
    if(partition != NULL) {
        ret = hal_flash_info_get((hal_partition_t)in_partition, (hal_logic_partition_t*)partition);
    }
    return ret;
}

int32_t amp_hal_flash_read(amp_partition_id_t id, uint32_t *offset, void *buffer, uint32_t buffer_len)
{
    int32_t ret = -1;
    ret = hal_flash_read((hal_partition_t) id, offset, buffer, buffer_len);
    return ret;
}

int32_t amp_hal_flash_write(amp_partition_id_t id, uint32_t *offset, const void *buffer, uint32_t buffer_len)
{
    int32_t ret = -1;
    ret = hal_flash_write((hal_partition_t) id, offset, buffer, buffer_len);
    return ret;
}

int32_t amp_hal_flash_erase(amp_partition_id_t id, uint32_t offset, uint32_t size)
{
    int32_t ret = -1;
    ret = hal_flash_erase((hal_partition_t)id, offset, size);
    return ret;
}