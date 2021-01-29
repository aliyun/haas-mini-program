/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef _AMP_NETWORK_H_
#define _AMP_NETWORK_H_

typedef enum {
	AMP_NETWORK_WIFI,
	AMP_NETWORK_CELLULAR,
	AMP_NETWORK_ETHERNET,
	AMP_NETWORK_UNKNOW,
}AMP_NETWORK_TYPE_E;

typedef struct amp_sim_info {
    char imsi[32];
    char imei[32];
    char iccid[32];
} amp_sim_info_t;

typedef struct amp_locator_info {
    char mcc[4];
    char mnc[4];
    int cellid;
    int lac;
    int signal;
} amp_locator_info_t;

typedef struct amp_wifi_info {
    char *ssid;
    char mac[6];
    char ip[4];
    int rssi;
} amp_wifi_info_t;

/**
 * @brief       wifi init.
 *
 * @return      0: success, -1: failed
 */
int amp_wifi_init();

/**
 * @brief       wifi connect.
 *
 * @return      0: success, -1: failed
 */
int amp_wifi_connect(const char *ssid, const char *passwd);

/**
 * @brief       wifi disconnect.
 *
 * @return      0: success, -1: failed
 */
int amp_wifi_disconnect();

/**
 * @brief       get wifi info.
 *
 * @return      0: success, -1: failed
 */
int amp_get_wifi_info(amp_wifi_info_t *wifi_info);

/**
 * @brief       get sim info.
 *
 * @return      0: success, -1: failed
 */
int amp_get_sim_info(amp_sim_info_t *sim_info);

/**
 * @brief       get locator info.
 *
 * @return      0: success, -1: failed
 */
int amp_get_locator_info(amp_locator_info_t *locator_info);

/**
 * @brief       get neighbor locator info.
 *
 * @return      0: success, -1: failed
 */
int amp_get_neighbor_locator_info(void (*cb)(amp_locator_info_t*, int));

/**
 * @brief       get network status.
 *
 * @return      0: success, -1: failed
 */
int amp_get_network_status(void);

/**
 * @brief       get network type.
 *
 * @return      0: success, -1: failed
 */
AMP_NETWORK_TYPE_E amp_get_network_type();

#endif /* _AMP_NETWORK_H_ */

