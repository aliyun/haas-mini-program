/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include "amp_defines.h"
#include "amp_hal_flash.h"

#define LINUX_STOER_SIZE                 (10*1024*1024) //10M
static unsigned char linux_store_buf[LINUX_STOER_SIZE];

/* Logic partition on flash devices */
const static amp_hal_logic_partition_t hal_partitions[] =
{

    [AMP_PARTITION_APPLICATION] =
    {
        .partition_owner            = AMP_FLASH_EMBEDDED,
        .partition_description      = "Application",
        .partition_start_addr       = 0x00000000,
        .partition_length           = 0x400000, /*4M*/
        .partition_options          = 3,
    },
    [AMP_PARTITION_OTA_TEMP] =
    {
        .partition_owner            = AMP_FLASH_EMBEDDED,
        .partition_description      = "OTA temp",
        .partition_start_addr       = 0x400000,
        .partition_length           = 0x400000, /* 4M */
        .partition_options          = 3,
    },
    [AMP_PARTITION_PARAMER1] =
    {
        .partition_owner            = AMP_FLASH_EMBEDDED,
        .partition_description      = "PARAMETER1",
        .partition_start_addr       = 0x900000,
        .partition_length           = 0x1000, /* 4k bytes */
        .partition_options          = 3,
    },
};

int32_t amp_hal_flash_info_get(amp_partition_id_t in_partition, amp_hal_logic_partition_t *partition)
{
    int ret = -1;
    amp_hal_logic_partition_t *logic_partition;
    logic_partition = (amp_hal_logic_partition_t *)&hal_partitions[in_partition];
    if(logic_partition != NULL) {
        ret = 0;
        memcpy(partition, logic_partition, sizeof(amp_hal_logic_partition_t));
    }
    return ret;
}

int32_t amp_hal_flash_read(amp_partition_id_t id, uint32_t *offset, void *buffer, uint32_t buffer_len)
{
    int32_t ret = 0;
    uint32_t start_addr = 0;
    amp_hal_logic_partition_t  partition_info;
    amp_hal_logic_partition_t *p_partition_info = NULL;
    if((offset == NULL) || (buffer == NULL)) {
        ret = -1;
    }
    else {
        p_partition_info = &partition_info;
        memset(p_partition_info, 0, sizeof(amp_hal_logic_partition_t));
        ret = amp_hal_flash_info_get(id, p_partition_info);
        printf("p_partiton = %s\r\n", p_partition_info->partition_description);      
        if(ret == 0) {
            start_addr = p_partition_info->partition_start_addr + *offset;
            if(buffer_len + *offset > p_partition_info->partition_length) {
                printf("linux read data too long!\r\n");
                ret = -2;
            }
            else {
                memcpy(buffer, (void*)&linux_store_buf[start_addr], buffer_len);
                *offset += buffer_len;
            }
        }
    }
    return ret;
}

int32_t amp_hal_flash_write(amp_partition_id_t id, uint32_t *offset, const void *buffer, uint32_t buffer_len)
{
    int32_t ret = 0;
    uint32_t start_addr = 0;
    amp_hal_logic_partition_t  partition_info;
    amp_hal_logic_partition_t *p_partition_info;
    if((offset == NULL) || (buffer == NULL)) {
        ret = -1;
    }
    else {
        p_partition_info = &partition_info;
        memset(p_partition_info, 0, sizeof(amp_hal_logic_partition_t));
        ret = amp_hal_flash_info_get(id, p_partition_info);
        if(ret == 0) {
            start_addr = p_partition_info->partition_start_addr + *offset;
            if(buffer_len + *offset > p_partition_info->partition_length) {
                printf("linux write data too long!\r\n");
                ret = -2;
            }
            else {
                memcpy((void*)&linux_store_buf[start_addr], buffer, buffer_len);
                *offset += buffer_len;
            }
        }
    }
    return ret;
}

int32_t amp_hal_flash_erase(amp_partition_id_t id, uint32_t offset, uint32_t size)
{
    uint32_t i = 0;
    int32_t ret = 0;
    uint32_t addr, page_size;
    uint32_t start_addr, end_addr;
    amp_hal_logic_partition_t  partition_info;
    amp_hal_logic_partition_t *p_partition_info;
    p_partition_info = &partition_info;
    memset(p_partition_info, 0, sizeof(amp_hal_logic_partition_t));
    ret = amp_hal_flash_info_get(id, p_partition_info);
    if(ret == 0) {
        start_addr = p_partition_info->partition_start_addr + offset;
        if(size + offset > p_partition_info->partition_length) {
            ret = -1;
        }
        else {
            for(i = start_addr; i < start_addr + size; i++) {
                linux_store_buf[i] = 0xff;
            }
        }
    }
    return ret;
}