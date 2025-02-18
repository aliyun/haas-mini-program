/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include "amp_hal_pwm.h"

int32_t amp_hal_pwm_init(pwm_dev_t *pwm)
{
    return hal_pwm_init(pwm);
}

int32_t amp_hal_pwm_start(pwm_dev_t *pwm)
{
    return hal_pwm_start(pwm);
}

int32_t amp_hal_pwm_stop(pwm_dev_t *pwm)
{
    return hal_pwm_stop(pwm);
}

int32_t amp_hal_pwm_para_chg(pwm_dev_t *pwm, pwm_config_t para)
{
    return hal_pwm_para_chg(pwm, para);
}

int32_t amp_hal_pwm_finalize(pwm_dev_t *pwm)
{
    return hal_pwm_finalize(pwm);
}



