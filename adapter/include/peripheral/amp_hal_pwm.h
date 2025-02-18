/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef AMP_HAL_PWM_H
#define AMP_HAL_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    float    duty_cycle; /* the pwm duty_cycle */
    uint32_t freq;       /* the pwm freq */
} pwm_config_t;

typedef struct {
    uint8_t       port;   /* pwm port */
    pwm_config_t  config; /* spi config */
    void         *priv;   /* priv data */
} pwm_dev_t;

/**
 * Initialises a PWM pin
 *
 * @param[in]  pwm  the PWM device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t amp_hal_pwm_init(pwm_dev_t *pwm);

/**
 * Starts Pulse-Width Modulation signal output on a PWM pin
 *
 * @param[in]  pwm  the PWM device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t amp_hal_pwm_start(pwm_dev_t *pwm);

/**
 * Stops output on a PWM pin
 *
 * @param[in]  pwm  the PWM device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t amp_hal_pwm_stop(pwm_dev_t *pwm);

/**
 * change the para of pwm
 *
 * @param[in]  pwm   the PWM device
 * @param[in]  para  the para of pwm
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t amp_hal_pwm_para_chg(pwm_dev_t *pwm, pwm_config_t para);

/**
 * De-initialises an PWM interface, Turns off an PWM hardware interface
 *
 * @param[in]  pwm  the interface which should be de-initialised
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t amp_hal_pwm_finalize(pwm_dev_t *pwm);

#ifdef __cplusplus
}
#endif

#endif /* AMP_HAL_PWM_H */

