/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "amp_pm.h"


int amp_system_sleep(void)
{
    return -1;
}

int amp_system_autosleep(int mode)
{
    return -1;
}

int amp_wakelock_lock(void *wakelock)
{
    return -1;
}

int amp_wakelock_unlock(void *wakelock)
{
    return -1;
}

int amp_wakelock_timedlock(void *wakelock, unsigned int msec)
{
    return -1;
}

void *amp_wakelock_create(const char *name)
{
    return NULL;
}

void amp_wakelock_release(void *wakelock)
{
}

int amp_pwrkey_notify_register(void (*cb)(int))
{
    return -1;
}

int amp_power_down(void)
{
    return -1;
}

int amp_power_reset(void)
{
    return -1;
}

int amp_battery_connect_state_get(int *state)
{
    return -1;
}

int amp_battery_voltage_get(int *voltage)
{
    return -1;
}

int amp_battery_charge_voltage_get(int *voltage)
{
    return -1;
}

int amp_battery_level_get(int *level)
{
    return -1;
}

int amp_battery_temperature_get(int *temperature)
{
    return -1;
}

int amp_charger_connect_state_get(int *state)
{
    return -1;
}

int amp_charger_current_get(int *current)
{
    return -1;
}

int amp_charger_switch_set(int enable)
{
    return -1;
}

