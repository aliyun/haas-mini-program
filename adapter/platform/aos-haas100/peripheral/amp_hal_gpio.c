/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include "k_api.h"
#include "aos/list.h"
#include "amp_hal_gpio.h"
#include "amp_system.h"

typedef struct {
    slist_t next;
    gpio_dev_t st_gpioinfo;
    gpio_irq_trigger_t trigger;
    gpio_irq_handler_t handle;
    void *arg;
}haas100_gpio_irq_info_t;

static kspinlock_t gpio_lock;
static int32_t init_flag = 0;
static slist_t gpio_irq_info = {0};

int32_t amp_hal_gpio_init(gpio_dev_t *gpio)
{
    if (init_flag == 0) {
        krhino_spin_lock_init(&gpio_lock);
        slist_init(&gpio_irq_info);
        init_flag = 1;
    }

    return hal_gpio_init(gpio);
}

int32_t amp_hal_gpio_output_high(gpio_dev_t *gpio)
{
    return hal_gpio_output_high(gpio);
}

int32_t amp_hal_gpio_output_low(gpio_dev_t *gpio)
{
    return hal_gpio_output_low(gpio);
}

int32_t amp_hal_gpio_output_toggle(gpio_dev_t *gpio)
{
    return hal_gpio_output_toggle(gpio);
}

int32_t amp_hal_gpio_input_get(gpio_dev_t *gpio, uint32_t *value)
{
    return hal_gpio_input_get(gpio, value);
}

static void amp_haas_gpio_irq_callback(void *arg)
{
    int32_t  ret = 0;
    int32_t  flags;
    uint32_t value = 0;
    slist_t  *cur = NULL;
    haas100_gpio_irq_info_t *pstinfo = (haas100_gpio_irq_info_t *)arg;
    haas100_gpio_irq_info_t *pst_gpio_irq = NULL; 
    if (NULL == pstinfo) {
        return ;
    }

    krhino_spin_lock_irq_save(&gpio_lock, flags);
    slist_for_each_entry_safe(&gpio_irq_info, cur, pst_gpio_irq, haas100_gpio_irq_info_t, next) {
        if (pst_gpio_irq->st_gpioinfo.port == pstinfo->st_gpioinfo.port) {
            break;
        }
    }
    krhino_spin_unlock_irq_restore(&gpio_lock, flags);

    if (NULL == pst_gpio_irq) {
        return ;
    }

    if (pst_gpio_irq->trigger == IRQ_TRIGGER_BOTH_EDGES) {
        ret = amp_hal_gpio_input_get(&pst_gpio_irq->st_gpioinfo, &value);
        if (ret) {
            pst_gpio_irq->handle(pst_gpio_irq->arg);
            krhino_spin_unlock_irq_restore(&gpio_lock, flags);
            return;
        } 

        if (value == GPIO_PinState_Set) {
            ret = hal_gpio_enable_irq(&pst_gpio_irq->st_gpioinfo, IRQ_TRIGGER_FALLING_EDGE, amp_haas_gpio_irq_callback, pst_gpio_irq);
        } else {
            ret = hal_gpio_enable_irq(&pst_gpio_irq->st_gpioinfo, IRQ_TRIGGER_RISING_EDGE, amp_haas_gpio_irq_callback, pst_gpio_irq);
        }
    }
    
    pst_gpio_irq->handle(pst_gpio_irq->arg);
    
    
}

int32_t amp_hal_gpio_enable_irq(gpio_dev_t *gpio, gpio_irq_trigger_t trigger,
                            gpio_irq_handler_t handler, void *arg)
{
    uint32_t value = 0;
    uint32_t flags = 0;
    int32_t  ret = 0;
    haas100_gpio_irq_info_t *pst_gpio_irq = NULL;

    if (init_flag == 0) {
        return -1;
    }

    pst_gpio_irq = amp_malloc(sizeof(haas100_gpio_irq_info_t));
    if (NULL == pst_gpio_irq) {
        return -1;
    }

    memset(pst_gpio_irq, 0, sizeof(haas100_gpio_irq_info_t));
    memcpy(&pst_gpio_irq->st_gpioinfo, gpio, sizeof(gpio_dev_t));
    pst_gpio_irq->handle = handler;
    pst_gpio_irq->arg = arg;
    pst_gpio_irq->trigger = trigger;

    /*disable irq*/
    krhino_spin_lock_irq_save(&gpio_lock, flags);
    slist_add_tail(&pst_gpio_irq->next, &gpio_irq_info);
    krhino_spin_unlock_irq_restore(&gpio_lock, flags);

    if (trigger == IRQ_TRIGGER_RISING_EDGE || trigger == IRQ_TRIGGER_RISING_EDGE) {
        ret = hal_gpio_enable_irq(gpio, trigger, handler, pst_gpio_irq);
    }
    /*for haas 100 gpio doesn't support both edge*/
    if (trigger == IRQ_TRIGGER_BOTH_EDGES) {
        ret = amp_hal_gpio_input_get(gpio, &value);
        if (ret) {
            return -1;
        }
        if (value == GPIO_PinState_Set) {
            ret = hal_gpio_enable_irq(gpio, IRQ_TRIGGER_FALLING_EDGE, amp_haas_gpio_irq_callback, pst_gpio_irq);
        } else {
            ret = hal_gpio_enable_irq(gpio, IRQ_TRIGGER_RISING_EDGE, amp_haas_gpio_irq_callback, pst_gpio_irq);
        }
    }

    if (ret) {
        /*shoud disable interrupt*/
        krhino_spin_lock_irq_save(&gpio_lock, flags);
        slist_del(&pst_gpio_irq->next, &gpio_irq_info);
        krhino_spin_unlock_irq_restore(&gpio_lock, flags);
        amp_free(pst_gpio_irq);
        return -1;
    }

    return 0;
}

int32_t amp_hal_gpio_disable_irq(gpio_dev_t *gpio)
{
    int32_t flags;
    int32_t ret;
    slist_t *cur = NULL;
    haas100_gpio_irq_info_t *pst_gpio_irq = NULL;
    if (init_flag == 0) {
        return -1;
    }

    ret = hal_gpio_disable_irq(gpio);
    krhino_spin_lock_irq_save(&gpio_lock, flags);
    slist_for_each_entry_safe(&gpio_irq_info, cur, pst_gpio_irq, haas100_gpio_irq_info_t, next) {
        if (pst_gpio_irq->st_gpioinfo.port == gpio->port) {
            slist_del(&pst_gpio_irq->next, &gpio_irq_info);
        }
    }
    krhino_spin_unlock_irq_restore(&gpio_lock, flags);
    return ret;
}

int32_t amp_hal_gpio_clear_irq(gpio_dev_t *gpio)
{
    return hal_gpio_clear_irq(gpio);
}

int32_t amp_hal_gpio_finalize(gpio_dev_t *gpio)
{
    return hal_gpio_finalize(gpio);
}

