/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include "amp_hal_wdg.h"

int32_t amp_hal_wdg_init(wdg_dev_t *wdg)
{
    return hal_wdg_init(wdg);
}

void amp_hal_wdg_reload(wdg_dev_t *wdg)
{
    hal_wdg_reload(wdg);
}

int32_t amp_hal_wdg_finalize(wdg_dev_t *wdg)
{
    return hal_wdg_finalize(wdg);
}