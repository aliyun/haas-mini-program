/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef AMP_PM_H
#define AMP_PM_H


typedef enum {
	AMP_CHARGER_STAT_SHUTDOWN = 0,
	AMP_CHARGER_STAT_CHECK,
	AMP_CHARGER_STAT_TRICKLE,
	AMP_CHARGER_STAT_PRE,
	AMP_CHARGER_STAT_CC,
	AMP_CHARGER_STAT_CV,
	AMP_CHARGER_STAT_TERMINAL,
	AMP_CHARGER_STAT_FAULT
} amp_charger_state_t;

/**
 * System enter sleep
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_system_sleep(void);

/**
 * Enable system autosleep interface
 *
 * @param[in]  mode  1 - autosleep enable, 0 - autosleep disable
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_system_autosleep(int mode);

/**
 * Accquire wakelock
 *
 * @param[in]  wakelock  wakelock instance
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_wakelock_lock(void *wakelock);

/**
 * Release wakelock
 *
 * @param[in]  wakelock  wakelock instance
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_wakelock_unlock(void *wakelock);

/**
 * Accquire wakelock within given time
 *
 * @param[in]  wakelock  wakelock instance
 * @param[in]  msec  wakelock keep time in ms
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_wakelock_timedlock(void *wakelock, unsigned int msec);

/**
 * Create wakelock
 *
 * @param[in]  name  wakelock name
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
void *amp_wakelock_create(const char *name);

/**
 * Destroy wakelock
 *
 * @param[in]  wakelock  wakelock instance
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
void amp_wakelock_release(void *wakelock);

/**
 * Register power key state notifier
 *
 * @param[in]  cb  power key notifier callback (argment: 1 - key down, 0 - key up)
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_pwrkey_notify_register(void (*cb)(int));

/**
 * Device power down
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_power_down(void);

/**
 * Device power reset
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_power_reset(void);

/**
 * Get battery connection state
 *
 * @param[in]  state (1 - connected, 0 - disconnected)
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_battery_connect_state_get(int *state);

/**
 * Get battery connection state
 *
 * @param[in] store voltage in mV
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_battery_voltage_get(int *voltage);

/**
 * Get battery level
 *
 * @param[in] store battery level (0 - 100)
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_battery_level_get(int *level);

/**
 * Get battery temperature
 *
 * @param[in]  store temperature
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_battery_temperature_get(int *temperature);

/**
 * Get charger connection state
 *
 * @param[in] store connection state (1 - connected, 0 - disconnected)
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_charger_connect_state_get(int *state);

/**
 * Get charger state
 *
 * @param[in] store charger state
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_charger_state_get(amp_charger_state_t *state);

/**
 * Get charger current
 *
 * @param[in] store charger current in mA
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_charger_current_get(int *current);

/**
 * Set charger switch (1 - ON, 0 - OFF)
 *
 * @param[in] charger switch onoff
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_charger_switch_set(int enable);


#endif /* AMP_PM_H */
