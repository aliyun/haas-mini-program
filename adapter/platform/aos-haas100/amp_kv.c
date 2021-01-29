/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_kv.h"
#include "aos/kv.h"

int amp_kv_set(const char *key, const void *val, int len, int sync)
{
    int   ret = 0;
    ret = aos_kv_set(key, val, len, sync);
    return ret;
}

int amp_kv_get(const char *key, void *buffer, int *buffer_len)
{
    int   ret = 0;
    ret = aos_kv_get(key, buffer, buffer_len);
    return ret;
}

int amp_kv_del(const char *key)
{
    int   ret = 0;
    ret = aos_kv_del(key);
    return ret;
}

int amp_kv_init()
{
    int   ret = 0;
    ret = aos_kv_init();
    return ret;
}