/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_ota.h"
#include "amp_system.h"
#include "ota_agent.h"

int interval_sys_upgrade_start(void *ctx)
{
    return ota_service_start((ota_service_t*)ctx);
}