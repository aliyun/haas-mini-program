/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_network.h"

int amp_get_sim_info(amp_sim_info_t *sim_info)
{
    int ret = -1;

    char imsi[24] = {0};
    char imei[32] = {0};
    char iccid[24] = {0};

    memset(sim_info, 0x0, sizeof(amp_sim_info_t));

    memcpy(sim_info->imsi, imsi, 24);
    memcpy(sim_info->imei, imei, 32);
    memcpy(sim_info->iccid, iccid, 24);

    return 0;
}

int amp_get_locator_info(amp_locator_info_t *locator_info)
{
    int ret = -1;

    char mcc[4] = {0};
    char mnc[4] = {0};

    memset(locator_info, 0x0, sizeof(amp_locator_info_t));

    memcpy(locator_info->mcc, mcc, 4);
    memcpy(locator_info->mnc, mnc, 4);
    locator_info->cellid = 0;
    locator_info->lac = 0;

    return 0;
}

int amp_get_neighbor_locator_info(void (*cb)(amp_locator_info_t*, int))
{
    // cb(NULL, 0);
    return 0;
}