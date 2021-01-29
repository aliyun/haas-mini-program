/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef UND_PLATFORM_H
#define UND_PLATFORM_H

#include <stdio.h>
#include <string.h>

#ifdef BUILD_AOS
#include "aos/kernel.h"
#include "aos/kv.h"
#else
#include "wrappers.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BUILD_AOS
#define aos_malloc(size)     amp_malloc(size)
#define aos_free(ptr)        amp_free(ptr)
#define aos_kv_set           amp_kv_set
#define aos_kv_get           amp_kv_get
#define aos_kv_del           amp_kv_del
#endif

#define aos_timer_create     amp_timer_create
#define aos_timer_start      amp_timer_start
#define aos_timer_stop       amp_timer_stop
#define aos_timer_delete     amp_timer_delete
#define aos_memset           memset
#define aos_strlen           strlen
#define aos_snprintf         amp_snprintf

#define undp_mutex_lock      amp_mutex_lock
#define undp_mutex_unlock    amp_mutex_unlock
#define undp_mutex_new       amp_mutex_create
#define undp_mutex_free      amp_mutex_destroy
#define undp_get_product_key HAL_GetProductKey
#define undp_get_device_name HAL_GetDeviceName

#ifdef __cplusplus
}
#endif

#endif /* UND_PLATFORM_H */
