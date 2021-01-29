/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "kv_conf.h"
#include "kv_adapt.h"

#include "amp_system.h"
#include "amp_defines.h"
#include "amp_hal_flash.h"


int32_t kv_flash_read(uint32_t offset, void *buf, uint32_t nbytes)
{
    return amp_hal_flash_read(AMP_PARTITION_ID_KV, &offset, buf, nbytes);
}

int32_t kv_flash_write(uint32_t offset, void *buf, uint32_t nbytes)
{
    return amp_hal_flash_write(AMP_PARTITION_ID_KV, &offset, buf, nbytes);
}

int32_t kv_flash_erase(uint32_t offset, uint32_t size)
{
    return amp_hal_flash_erase(AMP_PARTITION_ID_KV, offset, size);
}

void *kv_lock_create(void)
{
    return amp_mutex_create();
}

int32_t kv_lock_free(void *lock)
{
    amp_mutex_destroy(lock);
	return 0;
}

int32_t kv_lock(void *lock)
{
    amp_mutex_lock(lock);
	return 0;
}

int32_t kv_unlock(void *lock)
{
    amp_mutex_unlock(lock);
	return 0;
}

void *kv_sem_create(void)
{
    return amp_semaphore_create();
}

int32_t kv_sem_free(void *sem)
{
    amp_semaphore_destroy(sem);
    return 0;
}

int32_t kv_sem_wait(void *sem)
{
    return amp_semaphore_wait(sem, PLATFORM_WAIT_INFINITE);
}

int32_t kv_sem_post_all(void *sem)
{
    amp_semaphore_post(sem);
	return 0;
}

int32_t kv_start_task(const char *name, void *(*fn)(void *), void *arg,
                      uint32_t stack)
{
    void *task_handle = NULL;
	amp_os_thread_param_t param;
	int stack_used = stack;

	memset(&param, 0, sizeof(param));
    param.name = name;
    param.priority = amp_get_default_task_priority();
    param.stack_size = stack;
	return amp_thread_create(&task_handle, fn, arg, &param, &stack_used);
}

void kv_delete_task(void)
{
}

void *kv_malloc(uint32_t size)
{
    return amp_malloc(size);
}

void kv_free(void *ptr)
{
    amp_free(ptr);
}
