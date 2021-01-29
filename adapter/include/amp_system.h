/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef AMP_SYSTEM_H
#define AMP_SYSTEM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "stdarg.h"

typedef enum {
    TimerRunOnce     = 0, /* one-shot timer */
    TimerRunPeriodic = 1  /* repeating timer */
} amp_timer_type;

typedef struct amp_heap_info {
    unsigned long heap_total; /* total heap memory */
    unsigned long heap_used;  /* used heap memory */
} amp_heap_info_t;

typedef void *amp_osTimerId;
typedef void *amp_osMessageQId;
typedef void (*os_timer_cb)(void *argument);

typedef struct _amp_os_thread {
    unsigned int            priority;     /*initial thread priority */
    void                    *stack_addr;   /* thread stack address malloced by caller, use system stack by . */
    int                     stack_size;   /* stack size requirements in bytes; 0 is default stack size */
    int                     detach_state; /* 0: not detached state; otherwise: detached state. */
    char                    *name;         /* thread name. */
} amp_os_thread_param_t;

typedef struct _amp_wireless_info_t {
    int rssi; /* Received Signal Strength Indication */
    int snr;  /* Signal to Noise Ratio */
    int per;  /* Packet Error Rate (Unit: PPM, Part Per Million) */
} amp_wireless_info_t;

/**
 * Alloc memory.
 *
 * @param[in]  size  size of the mem to malloc.
 *
 * @return  NULL: error.
 */
void *amp_malloc(unsigned int size);

/**
 * Free memory.
 *
 * @param[in]  ptr  address point of the mem.
 *
 * @return  none.
 */
void amp_free(void *ptr);

/**
 * Alloc memory and clear to zero.
 *
 * @param[in]  nitems  number of items to malloc.
 * @param[in]  size    size of one item to malloc.
 *
 * @return  NULL: error.
 */
void *amp_calloc(unsigned int nitems, unsigned int size);

/**
 * Realloc memory.
 *
 * @param[in]  ptr   current memory address point.
 * @param[in]  size  new size of the mem to remalloc.
 *
 * @return  NULL: error.
 */
void *amp_realloc(void *ptr, unsigned int size);

/**
 * Get current time in mini seconds.
 *
 * @return  type long      elapsed time in mini seconds from system starting.
 */
unsigned long amp_uptime(void);

/**
 * Msleep. Sleep current task for ms.
 *
 * @param[in]  ms  sleep time in milliseconds.
 *
 * @return  none.
 */
void amp_msleep(unsigned int ms);

/**
 * srand function.
 *
 * @param[in]  seed  The seed number to use to generate the random sequence.
 *
 * @return  none.
 */
void amp_srandom(unsigned int seed);

/**
 * rand function.
 *
 * @return  random value.
 */
unsigned int amp_random(unsigned int region);

/**
 * @brief   printf function.
 *
 * @return  none
 */
void amp_printf(const char *fmt, ...);

/**
 * @brief   snprintf function.
 *
 * @return  
 */
int amp_snprintf(char *str, const int len, const char *fmt, ...);

/**
 * @brief   vsnprintf function.
 *
 * @return  
 */
int amp_vsnprintf(char *str, const int len, const char *format, va_list ap);

/**
 * Alloc a semaphore.
 *
 * @return    pointer of semaphore object, semaphore object must be
 *            alloced, hdl pointer in aos_sem_t will refer a kernel obj internally.
 */
void *amp_semaphore_create(void);

/**
 * Destroy a semaphore.
 *
 * @param[in]  sem  pointer of semaphore object, mem refered by hdl pointer
 *                  in aos_sem_t will be freed internally.
 *
 * @return  none.
 */
void amp_semaphore_destroy(void *sem);

/**
 * Release a semaphore.
 *
 * @param[in]  sem  semaphore object, it contains kernel obj pointer which aos_sem_new alloced.
 *                  It will wakeup one task which is waiting for the sem according to task priority.
 *
 * @return  none.
 */
void amp_semaphore_post(void *sem);

/**
 * Acquire a semaphore.
 *
 * @param[in]  sem         semaphore object, it contains kernel obj pointer
 *                         which aos_sem_new alloced.
 * @param[in]  timeout_ms  waiting until timeout in milliseconds.
 *                         value:  AOS_WAIT_FOREVER: wait mutex for ever.
 *                              0: return immediately if not get mutex.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_semaphore_wait(void *sem, unsigned int timeout_ms);

/**
 * Create a task.
 *
 * @param[in]  thread_handle        handle.
 * @param[in]  work_routine         task function.
 * @param[in]  arg                  argument of the function.
 * @param[in]  amp_os_thread_param  param of the thread.
 * @param[in]  stack_used           stack-buf: if stack_buf==NULL, provided by kernel.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_thread_create(
            void **thread_handle,
            void *(*work_routine)(void *),
            void *arg,
            amp_os_thread_param_t *amp_os_thread_param,
            int *stack_used);

/**
 * Delete a task by name.
 *
 * @param[in]  thread_handle  thread handle.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_thread_delete(void *thread_handle);

/**
 * Get task name.
 *
 * @return  the name of the task
 */
char *amp_get_thread_name(void **thread_handle);

/**
 * @brief   get RTOS default priority
 *
 * @return  default priority
 */
int amp_get_default_task_priority();


/**
 * Alloc a mutex.
 *
 * @return    pointer of mutex object, mutex object must be alloced,
 *            hdl pointer in aos_mutex_t will refer a kernel obj internally.
 */
void *amp_mutex_create(void);

/**
 * Free a mutex.
 *
 * @param[in]  mutex  mutex object, mem refered by hdl pointer in
 *                    aos_mutex_t will be freed internally.
 *
 * @return  none.
 */
void amp_mutex_destroy(void *mutex);

/**
 * Lock a mutex.
 *
 * @param[in]  mutex    mutex object, it contains kernel obj pointer.
 *
 * @return  none.
 */
void amp_mutex_lock(void *mutex);

/**
 * Unlock a mutex.
 *
 * @param[in]  mutex  mutex object, it contains kernel obj pointer.
 *
 * @return  none.
 */
void amp_mutex_unlock(void *mutex);

/**
 * This function will create a timer and run auto.
 *
 * @param[in]  name    pointer to the timer.
 * @param[in]  fn      callbak of the timer.
 * @param[in]  arg     the argument of the callback.
 *
 * @return  0: success, otherwise: fail.
 */
void *amp_timer_create(const char *name, void (*func)(void *), void *user_data);

/**
 * @brief   get timer params.
 *
 * @return  none
 */
void *amp_get_timer_params(void *timer_handle);

/**
 * This function will start a timer.
 *
 * @param[in]  timer  pointer to the timer.
 * @param[in]  ms     ms of the normal timer triger.
 * @param[in]  repeat repeat or not when the timer is expired.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_timer_start(void *timer, int ms, amp_timer_type repeat);

/**
 * This function will stop a timer.
 *
 * @param[in]  timer  pointer to the timer.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_timer_stop(void *timer);

/**
 * This function will delete a timer.
 *
 * @param[in]  timer  pointer to a timer.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_timer_delete(void *timer);

/**
 * This function will create a queue.
 *
 * @param[in]  queue_length  the bytes of the buf.
 * @param[in]  item_size     the max size for one msg.
 *
 * @return  pointer to the queue(the space is provided by user).
 */
void *amp_queue_create(int queue_length, int item_size);

/**
 * This function will delete a queue.
 *
 * @param[in]  queue  pointer to the queue.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_queue_delete(void *queue);

/**
 * This function will send a msg to the front of a queue.
 *
 * @param[in]  queue  pointer to the queue.
 * @param[in]  msg    msg to send.
 * @param[in]  size   size of the msg.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_queue_send(void *queue, void *p_info, unsigned int size, unsigned int millisec);

/**
 * This function will receive msg from a queue.
 *
 * @param[in]   queue  pointer to the queue.
 * @param[in]   ms     ms to wait before receive.
 * @param[out]  msg    buf to save msg.
 * @param[out]  size   size of the msg.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_queue_recv(void *mq, void *p_info, unsigned int size, unsigned int millisec);

/**
 * Reboot system.
 *
 * @return none.
 */
void amp_system_reboot(void);

/**
 * Get system version.
 *
 * @return  sysinfo_version.
 */
const char *amp_get_system_version(void);

/**
 * Get system platform type.
 *
 * @return  platform_type.
 */
const char *amp_get_system_platform_type(void);

/**
 * Get device name.
 *
 * @return  device_name.
 */
const char *amp_get_device_name(void);

/**
 * @brief   system sleep.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_system_sleep(void);

/**
 * @brief   get wireless infomation.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_get_wireless_info(struct _amp_wireless_info_t *info);

/**
 * @brief   get network status.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_get_network_status(void);

/**
 * @brief   get ip address.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_get_ip(char *ip);

/**
 * @brief   get mac address.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_get_mac_addr(unsigned char mac[6]);

/**
 * @brief   get system heap memory info.
 *
 * @return  0:SUCCESS  other:FAIL
 */
int amp_heap_memory_info(amp_heap_info_t *heap_info);

/**
 * Initialize system
 */
int amp_system_init(void);

/**
 * @brief   get module hardware version.
 *
 * @return  hardware version value.
 */
const char *amp_get_module_hardware_version(void);

/**
 * @brief   get module software versioin.
 *
 * @return  software version value.
 */
const char *amp_get_module_software_version(void);

#if defined(__cplusplus)
}
#endif

#endif /* AMP_SYSTEM_H */
