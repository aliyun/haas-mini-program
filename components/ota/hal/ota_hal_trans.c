/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include "ota_log.h"
//#include <sys/socket.h>
#include "amp_platform.h"
#include "ota_hal_trans.h"

#ifndef OTA_SIGNAL_CHANNEL
#define OTA_SIGNAL_CHANNEL 1
#endif
#include "infra/infra_defs.h"
#include "mqtt/mqtt_api.h"
#include "wrappers.h"
#include "amp_socket.h"

int close(int fd);
int ota_hal_socket(void)
{
    return amp_socket_open(AF_INET, SOCK_STREAM, 0);
}

int ota_hal_close(int fd)
{
    return amp_socket_close(fd);
}

/*MQTT API*/
int ota_hal_mqtt_publish(void *mqtt_client, char *topic, int qos, void *data, int len)
{
    return IOT_MQTT_Publish_Simple(mqtt_client, topic, qos, data, len);
}

int ota_hal_mqtt_subscribe(void *mqtt_client, char *topic, void *cb, void *ctx)
{
    return IOT_MQTT_Subscribe_Sync(mqtt_client, topic, 0, cb, ctx, 1000);
}

int ota_hal_mqtt_deinit(void)
{
    return IOT_MQTT_Destroy(NULL);
}

int ota_hal_mqtt_init(void)
{
    return (IOT_MQTT_Construct(NULL) == NULL) ? -1 : 0;
}
