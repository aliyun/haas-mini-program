/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"

#include "amp_system.h"
#include "amp_defines.h"

#define SYSINFO_DEVICE_NAME "Linux"
#define SYSINFO_VERSION "0.0.1"
#define DEFAULT_TASK_PRIORITY 32

static amp_heap_info_t heap_usage = {0, 0};

void *amp_malloc(unsigned int size)
{
    void *ptr = NULL;
    if((ptr = malloc(size)) == NULL){
        return NULL;
    }

    /* heap usage increase */
    heap_usage.heap_used += malloc_usable_size(ptr);
    return ptr;
}

void amp_free(void *ptr)
{
    if(ptr == NULL){
        return;
    }

    /* heap usage decrease */
    heap_usage.heap_used -= malloc_usable_size(ptr);
    free(ptr);
}

void *amp_calloc(unsigned int nitems, unsigned int size)
{
    void *ptr = NULL;

    if((ptr = calloc((size_t)nitems, (size_t)size)) == NULL){
        return NULL;
    }

    /* heap usage increase */
    heap_usage.heap_used += malloc_usable_size(ptr);
    return ptr;
}

void *amp_realloc(void *ptr, unsigned int size)
{
    void *next = NULL;

    /* ptr = NULL, equal to call malloc(size) */
    if(ptr == NULL){
        if ((next = amp_malloc(size)) == NULL){
            return NULL;
        }
        return next;
    }

    /* size = 0，equal to call free(ptr) */
    if (size == 0){
        amp_free(ptr);
        return NULL;
    }

    /* heap usage decrease */
    heap_usage.heap_used -= malloc_usable_size(ptr);

    if((next = realloc(ptr, (size_t)size)) == NULL){
        return NULL;
    }

    /* heap usage increase */
    heap_usage.heap_used += malloc_usable_size(next);
    return next;
}

unsigned long amp_uptime(void)
{
    unsigned long            time_ms;
    struct timespec     ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_ms = ((unsigned long)ts.tv_sec * (unsigned long)1000) + (ts.tv_nsec / 1000 / 1000);

    return time_ms;
}

void amp_msleep(unsigned int ms)
{
    usleep(1000 * ms);
}

void amp_srandom(unsigned int seed)
{
    srandom(seed);
}

unsigned int amp_random(unsigned int region)
{
    FILE *handle;
    ssize_t ret = 0;
    unsigned int output = 0;
    handle = fopen("/dev/urandom", "r");
    if (handle == NULL) {
        perror("open /dev/urandom failed\n");
        return 0;
    }
    ret = fread(&output, sizeof(unsigned int), 1, handle);
    if (ret != 1) {
        printf("fread error: %d\n", (int)ret);
        fclose(handle);
        return 0;
    }
    fclose(handle);
    return (region > 0) ? (output % region) : output;
}

void amp_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

int amp_snprintf(char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int amp_vsnprintf(char *str, const int len, const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}

void amp_system_reboot(void)
{
    if (system("reboot")) {
        perror("amp_system_reboot failed");
    }
}

void *amp_semaphore_create(void)
{
    sem_t *sem = (sem_t *)malloc(sizeof(sem_t));
    if (NULL == sem) {
        return NULL;
    }

    if (0 != sem_init(sem, 0, 0)) {
        free(sem);
        return NULL;
    }

    return sem;
}

void amp_semaphore_destroy(void *sem)
{
    sem_destroy((sem_t *)sem);
    free(sem);
}

void amp_semaphore_post(void *sem)
{
    sem_post((sem_t *)sem);
}

int amp_semaphore_wait(void *sem, unsigned int timeout_ms)
{
    if (PLATFORM_WAIT_INFINITE == timeout_ms) {
        sem_wait(sem);
        return 0;
    } else {
        struct timespec ts;
        int s;
        /* Restart if interrupted by handler */
        do {
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
                return -1;
            }

            s = 0;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_nsec -= 1000000000;
                s = 1;
            }

            ts.tv_sec += timeout_ms / 1000 + s;

        } while (((s = sem_timedwait(sem, &ts)) != 0) && errno == EINTR);

        return (s == 0) ? 0 : -1;
    }
}

int amp_thread_create(
            void **thread_handle,
            void *(*work_routine)(void *),
            void *arg,
            amp_os_thread_param_t *amp_os_thread_param,
            int *stack_used)
{
	pthread_t handler;
    int ret = -1;

    if (stack_used) {
        *stack_used = 0;
    }

    ret = pthread_create((pthread_t *)&handler, NULL, work_routine, arg);
    if (ret != 0) {
        printf("pthread_create error: %d\n", (int)ret);
        return -1;
    }
	//*thread_handle = handler;
    return 0;
}

int amp_thread_delete(void *thread_handle)
{
    pthread_exit(0);
    return 0;
}

int amp_get_default_task_priority()
{
    return DEFAULT_TASK_PRIORITY;
}

void *amp_mutex_create(void)
{
    int err_num;
    pthread_mutex_t *mutex = (pthread_mutex_t *)amp_malloc(sizeof(pthread_mutex_t));
    if (NULL == mutex) {
        return NULL;
    }

    if (0 != (err_num = pthread_mutex_init(mutex, NULL))) {
        perror("create mutex failed\n");
        amp_free(mutex);
        return NULL;
    }

    return mutex;
}

void amp_mutex_destroy(void *mutex)
{
    int err_num;

    if (!mutex) {
        perror("mutex want to destroy is NULL!\n");
        return;
    }
    if (0 != (err_num = pthread_mutex_destroy((pthread_mutex_t *)mutex))) {
        perror("destroy mutex failed\n");
    }

    amp_free(mutex);
}

void amp_mutex_lock(void *mutex)
{
    int err_num;
    if (0 != (err_num = pthread_mutex_lock((pthread_mutex_t *)mutex))) {
        printf("lock mutex failed: - '%s' (%d)\n", strerror(err_num), err_num);
    }
}

void amp_mutex_unlock(void *mutex)
{
    int err_num;
    if (0 != (err_num = pthread_mutex_unlock((pthread_mutex_t *)mutex))) {
        printf("unlock mutex failed - '%s' (%d)\n", strerror(err_num), err_num);
    }
}

void *amp_timer_create(const char *name, void (*func)(void *), void *user_data)
{
    timer_t *timer = NULL;

    struct sigevent ent;

    /* check parameter */
    if (func == NULL) {
        return NULL;
    }

    timer = (timer_t *)malloc(sizeof(time_t));
    if (timer == NULL) {
        return NULL;
    }

    /* Init */
    memset(&ent, 0x00, sizeof(struct sigevent));

    /* create a timer */
    ent.sigev_notify = SIGEV_THREAD;
    ent.sigev_notify_function = (void (*)(union sigval))func;
    ent.sigev_value.sival_ptr = user_data;

    if (timer_create(CLOCK_MONOTONIC, &ent, timer) != 0) {
        free(timer);
        return NULL;
    }

    return (void *)timer;
}

void *amp_get_timer_params(void *timerHandle)
{
    return timerHandle;
}

int amp_timer_start(void *timer, int ms, amp_timer_type repeat)
{
    struct itimerspec ts;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    if (repeat) {
        /* it_value=0: stop timer */
        ts.it_interval.tv_sec = ms / 1000;
        ts.it_interval.tv_nsec = (ms % 1000) * 1000000;
    } else {
        /* it_interval=0: timer run only once */
        ts.it_interval.tv_sec = 0;;
        ts.it_interval.tv_nsec = 0;
    }

    /* it_value=0: stop timer */
    ts.it_value.tv_sec = ms / 1000;
    ts.it_value.tv_nsec = (ms % 1000) * 1000000;

    return timer_settime(*(timer_t *)timer, 0, &ts, NULL);
}

int amp_timer_stop(void *timer)
{
    struct itimerspec ts;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    /* it_interval=0: timer run only once */
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    /* it_value=0: stop timer */
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;

    return timer_settime(*(timer_t *)timer, 0, &ts, NULL);
}

int amp_timer_delete(void *timer)
{
    int ret = 0;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    ret = timer_delete(*(timer_t *)timer);

    free(timer);

    return ret;
}

#define MESSAGE_QUEUE_NAME   "/amp"
#define QUEUE_PERMISSIONS     0666
void *amp_queue_create(int queue_length, int item_size)
{
    mqd_t mq;
    struct mq_attr attr;

    /* config a message queue */
    attr.mq_flags = 0;
    attr.mq_maxmsg = queue_length;
    attr.mq_msgsize = item_size;
    attr.mq_curmsgs = 0;

    if ((mq = mq_open (MESSAGE_QUEUE_NAME, O_RDWR | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("HAL: mq_open (mq)");
        exit (1);
    }

    return (void *)(uint64_t)mq;
}


void *amp_queue_create_name(char *name, int32_t queue_length, int32_t item_size, int block)
{
    mqd_t mq;
    struct mq_attr attr;

    /* config a message queue */

    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = queue_length;
    attr.mq_msgsize = item_size;
    attr.mq_curmsgs = 0;
    if ((mq = mq_open (name, O_RDWR | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("HAL: mq_open (mq)");
        exit (1);
    }

    if (block != 0) {
        mq_getattr(mq, &attr);
        attr.mq_flags = O_NONBLOCK;
        mq_setattr(mq, &attr, NULL);
    }

    return (void *)(uint64_t)mq;
}

int amp_queue_send(void *mq, void *p_info, unsigned int size, unsigned int millisec)
{
    int ret = -1;

    if (mq == NULL || p_info == NULL) {
        return ret;
    }
    if (mq_send((mqd_t)(uint64_t)mq, p_info, size, 0) == -1) {
        perror ("HAL: Not able to send message");
        return ret;
    }

    return 0;
}

int amp_queue_recv(void *mq, void *p_info, unsigned int size, unsigned int millisec)
{
    int32_t ret = -1;
    uint64_t time = amp_uptime();

    if (mq == NULL || p_info == NULL) {
        return ret;
    }
    while(1) {
        if (mq_receive((mqd_t)(uint64_t)mq, p_info, size, 0) == -1) {
            if ((uint32_t)(amp_uptime() - time) >= millisec)
            {
                return -1;
            }
        }
		else {
			break;
		}
    }

    return 0;
}

int amp_queue_delete(void *mq)
{
    int32_t ret = -1;
    mqd_t handle = (mqd_t)(uint64_t)mq;

    if (mq == NULL) {
        return ret;
    }

    if (mq_close(handle) == -1) {
            perror ("HAL: Not able to close message");
            return ret;
    }

    if (mq_unlink (MESSAGE_QUEUE_NAME) == -1) {
        perror ("Client: mq_unlink");
        return ret;
    }

    return 0;
}

int HAL_GetFirmwareVersion(char *version)
{
    return 0;
}

const char *amp_get_system_version(void)
{
    return SYSINFO_VERSION;
}

const char *amp_get_platform_type(void)
{
    return SYSINFO_DEVICE_NAME;
}

int amp_get_ip(char *ip)
{
    return 0;
}

int amp_get_mac_addr(unsigned char mac[8])
{
    memset(mac, 0, 8);
    char default_value[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    memcpy(mac, default_value, 8);
    return 0;
}

int amp_network_status_registercb(void (*cb)(int status, void *), void *arg)
{
    return 0;
}

int amp_get_network_status(void)
{
    return 1;
}

int amp_get_wireless_info(struct _amp_wireless_info_t *info)
{
	return 0;
}

const char *amp_get_device_name(void)
{
    return "linux_test_device";
}

int amp_heap_memory_info(amp_heap_info_t *heap_info)
{
    int ret = -1;

    struct rlimit rlim;
    if(getrlimit(RLIMIT_STACK, &rlim) != 0){
        perror("getrlimit failed\r\n");
        return -1;
    }

    heap_info->heap_total = (size_t)rlim.rlim_cur;
    heap_info->heap_used = heap_usage.heap_used;

    return 0;
}

/* system init */
int amp_system_init(void)
{
    /* fs init */
    amp_fs_init();

    return 0;
}

const char *amp_get_module_hardware_version(void)
{
    return "Module_Hardware_version";
}

const char *amp_get_module_software_version(void)
{
    return "Module_Software_version";
}