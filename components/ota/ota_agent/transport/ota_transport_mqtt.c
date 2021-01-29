/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <string.h>
#include <cJSON.h>
#include <stdlib.h>
#include "ota_log.h"
#include "ota_import.h"
#include "ota_hal_os.h"
#include "ota_hal_trans.h"
#include "ota_agent.h"

/**
 * ota_sevice_parse_msg  OTA parse download info from cloud
 *
 * @param[in]  ota_service_t *ctx  service manager.
 *             char *json          transport message from Cloud.
 *
 * @return OTA_TRANSPORT_FAIL     transport errno.
 */
int ota_sevice_parse_msg(ota_service_t *ctx, const char *json)
{
    int ret            = 0;
    cJSON *root        = NULL;
    ota_boot_param_t ota_param = {0};
    root = cJSON_Parse(json);
    if (NULL == root || NULL == ctx) {
        ret = OTA_TRANSPORT_PAR_FAIL;
        goto EXIT;
    } else {
        ctx->module_ota = 0;
        /* recover the process type */
        cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
        if (NULL == cmd) {
            ctx->ota_process = OTA_PROCESS_NORMAL;
        }
        cJSON *message = cJSON_GetObjectItem(root, "message");
        if (NULL == message) {
            ret = OTA_MESSAGE_PAR_FAIL;
            goto EXIT;
        }
        if ((strncmp(message->valuestring, "success", strlen("success")))) {
            ret = OTA_SUCCESS_PAR_FAIL;
            goto EXIT;
        }
        cJSON *json_obj = cJSON_GetObjectItem(root, "data");
        if (NULL == json_obj) {
            ret = OTA_DATA_PAR_FAIL;
            goto EXIT;
        }
        cJSON *url = cJSON_GetObjectItem(json_obj, "url");
        if (NULL == url) {
            ret = OTA_URL_PAR_FAIL;
            goto EXIT;
        }
        cJSON *submodule = cJSON_GetObjectItem(json_obj, "module");
        if(NULL != submodule) {
            ctx->module_ota = 1;
            strncpy(ctx->module_name, submodule->valuestring, sizeof(ctx->module_name) - 1);
            OTA_LOG_I("submode = %s\r\n", submodule->valuestring);
        }
        cJSON *version = cJSON_GetObjectItem(json_obj, "version");
        if (NULL == version) {
            ret = OTA_VER_PAR_FAIL;
            goto EXIT;
        } else {
            if (OTA_PROCESS_UAGENT_OTA == ctx->ota_process) {
                /* suppose version parser fail */
                ret = OTA_TRANSPORT_VER_FAIL;
                if (NULL != version && cJSON_IsArray(version) && cJSON_IsArray(version)) {
                    cJSON *p = version->child;
                    while (NULL != p && cJSON_IsObject(p)) {
                        cJSON *type = cJSON_GetObjectItem(p, "type");
                        cJSON *version = cJSON_GetObjectItem(p, "version");
                        if (NULL != type && cJSON_IsNumber(type) && NULL != version && cJSON_IsString(version)) {
                            OTA_LOG_I("version type %d version %s", type->valueint, version->valuestring);
                            if (OTA_UPGRADE_SOC == type->valueint) {
                                int is_ota = strncmp(version->valuestring, ota_hal_version(ctx->dev_type, ctx->dn), strlen(version->valuestring));
                                if (is_ota > 0) {
                                    ret = 0;
                                }
                            }
                        } else {
                            OTA_LOG_W("version missing param, try next item");
                        }
                        p = p->next;
                    }
                }
                if (OTA_TRANSPORT_VER_FAIL == ret) { /* still keep fail */
                    goto EXIT;
                }
            } else {
                int is_ota = 0;
                if(ctx->module_ota == 0) {
                    is_ota = strncmp(version->valuestring, ota_hal_version(ctx->dev_type, NULL), strlen(version->valuestring));
                }
                else {
                    is_ota = strncmp(version->valuestring, ota_hal_version(ctx->dev_type, (char*)ctx->module_name), strlen(version->valuestring));
                }
                if (is_ota <= 0) {
                    ret = OTA_TRANSPORT_VER_FAIL;
                    goto EXIT;
                }
                strncpy(ota_param.ver, version->valuestring, strlen(version->valuestring));
                ota_param.ver[strlen(version->valuestring)] = '\0';
            }
        }

        strncpy(ota_param.url, url->valuestring, OTA_URL_LEN - 1);
        cJSON *signMethod = cJSON_GetObjectItem(json_obj, "signMethod");
        if (signMethod != NULL) {
            memset(ota_param.hash, 0x00, OTA_HASH_LEN);
            ret = ota_to_capital(signMethod->valuestring, strlen(signMethod->valuestring));
            if (ret != 0) {
                ret = OTA_VER_PAR_FAIL;
                goto EXIT;
            }
            if (0 == strncmp(signMethod->valuestring, "MD5", strlen("MD5"))) {
                cJSON *md5 = cJSON_GetObjectItem(json_obj, "sign");
                if (NULL == md5) {
                    ret = OTA_MD5_PAR_FAIL;
                    goto EXIT;
                }
                ota_param.hash_type = OTA_MD5;
                strncpy(ota_param.hash, md5->valuestring, OTA_HASH_LEN - 1);
                ret = ota_to_capital(ota_param.hash, strlen(ota_param.hash));
                if (ret != 0) {
                    ret = OTA_VER_PAR_FAIL;
                    goto EXIT;
                }
             }  else if (0 == strncmp(signMethod->valuestring, "SHA256", strlen("SHA256"))) {
                cJSON *sha256 = cJSON_GetObjectItem(json_obj, "sign");
                if (NULL == sha256) {
                    ret = OTA_SHA256_PAR_FAIL;
                    goto EXIT;
                }
                ota_param.hash_type = OTA_SHA256;
                strncpy(ota_param.hash, sha256->valuestring, OTA_HASH_LEN - 1);
                ret = ota_to_capital(ota_param.hash, strlen(ota_param.hash));
                if (ret != 0) {
                    ret = OTA_VER_PAR_FAIL;
                    goto EXIT;
                }
            } else {
                ret = OTA_HASH_PAR_FAIL;
                goto EXIT;
            }
        } else { /* old protocol*/
            memset(ota_param.hash, 0x00, OTA_HASH_LEN);
            cJSON *md5 = cJSON_GetObjectItem(json_obj, "md5");
            if (NULL == md5) {
                ret = OTA_MD5_PAR_FAIL;
                goto EXIT;
            }
            ota_param.hash_type = OTA_MD5;
            strncpy(ota_param.hash, md5->valuestring, OTA_HASH_LEN - 1);
            ret = ota_to_capital(ota_param.hash, strlen(ota_param.hash));
            if (ret != 0) {
                ret = OTA_VER_PAR_FAIL;
                goto EXIT;
            }
        }
        cJSON *size = cJSON_GetObjectItem(json_obj, "size");
        if (NULL == size) {
            ret = OTA_SIZE_PAR_FAIL;
            goto EXIT;
        }
        ota_param.len = size->valueint;
        cJSON *digestSign = cJSON_GetObjectItem(json_obj, "digestsign");
        if (digestSign != NULL) {
            unsigned int sign_len = OTA_SIGN_LEN;
            memset(ota_param.sign, 0x00, OTA_SIGN_LEN);
            if (ota_base64_decode((const unsigned char *)digestSign->valuestring, strlen(digestSign->valuestring), (unsigned char*)ota_param.sign, &sign_len) != 0) {
                ret = OTA_SIGN_PAR_FAIL;
                goto EXIT;
            }
        }
    }

EXIT:
    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    OTA_LOG_I("Parse ota version:%s url:%s ret:%d \n", ota_param.ver, ota_param.url, ret);
    if (ret == OTA_TRANSPORT_VER_FAIL) {
        OTA_LOG_E("ota version is too old, discard it.");
#ifdef OTA_CONFIG_UAGENT
        if (OTA_PROCESS_UAGENT_OTA == ctx->ota_process) {
            ota_update_process("OTA transport verion is too old", -1);
        } else
#endif /* OTA_CONFIG_UAGENT */
        {
            ota_transport_status(ctx, ret);
        }
    } else if (ret < 0) {
        ret = OTA_TRANSPORT_PAR_FAIL;
#ifdef OTA_CONFIG_UAGENT
        if (OTA_PROCESS_UAGENT_OTA == ctx->ota_process) {
            ota_update_process("ota upgrade parse failed", -1);
        } else
#endif /* OTA_CONFIG_UAGENT */
        {
            ota_transport_status(ctx, ret);
        }
    } else {
        ota_transport_status(ctx, 1);
        ota_param.upg_flag = 0x00;
        if(ctx->dev_type == 0) {
            ota_param.upg_flag = OTA_UPGRADE_ALL;
        }
        ota_param.upg_status = OTA_TRANSPORT;
        ret = ota_update_parameter(&ota_param);
        if (ret != 0) {
            OTA_LOG_I("ota param err.\n");
            ota_transport_status(ctx, OTA_TRANSPORT_PAR_FAIL);
        }
        if(ctx->module_ota == 0) {
            if(ctx->on_upgrade != NULL) {
                ctx->on_upgrade(ctx, ota_param.ver, ota_param.url);
            }
        }
        else {
            if(ctx->on_module_upgrade != NULL) {
                ctx->on_module_upgrade(ctx, ota_param.ver, ota_param.url);
            }
        }
    }
    return ret;
}

/**
 * ota_mqtt_publish message to Cloud
 *
 * @param[in] mqtt_client      mqtt client ptr
 * @param[in] name             topic name
 * @param[in] msg              message content
 * @param[in] pk               product key
 * @param[in] dn               device name
 *
 * @return OTA_SUCCESS         OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old. 
 */
static int ota_mqtt_publish(void *mqtt_client, const char *name, char *msg, char *pk, char *dn)
{
    int ret = 0;
    char topic[OTA_MSG_LEN] = {0};
    if (name == NULL || msg == NULL || pk == NULL || dn == NULL) {
        return OTA_TRANSPORT_INT_FAIL;
    }

    ret = ota_snprintf(topic, OTA_MSG_LEN-1, "/ota/device/%s/%s/%s", name, pk, dn);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    OTA_LOG_I("Public topic:%s msg:%s", topic, msg);
    ret = ota_hal_mqtt_publish(mqtt_client, topic, 1, (void *)msg, strlen(msg) + 1);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    return ret;
}

/**
 * ota_mqtt_sub_cb  upgrade callback
 *
 * @param[in] pctx      ota context
 * @param[in] pclient   mqtt pclient
 * @param[in] msg       mqtt message 
 *
 * @return void
 */
static void ota_mqtt_sub_cb(void *pctx, void *pclient, void *msg)
{
    char *payload = NULL;
    if (msg == NULL) {
        return;
    }
    ota_service_t *ctx = (ota_service_t *)pctx;
    ota_mqtt_msg_t *mqtt_msg = (ota_mqtt_msg_t *)msg;
    switch (mqtt_msg->event) {
        case OTA_MQTT_EVENT_PUB_RECEIVED:
            payload = (char *)mqtt_msg->topic->payload;
            break;
        default:
            return;
    }
    if(ctx == NULL) {
        OTA_LOG_E("mqtt ctx err.");
        return;
    }
    if(payload != NULL) {
        OTA_LOG_I("mqtt cb evt:%d %s", mqtt_msg->event, payload);
        ota_sevice_parse_msg(ctx, payload);
    }
}

/**
 * ota_transport_inform  OTA inform version to cloud.
 *
 * @param[in] ota_service_t *ctx ctx  ota service context
 * @param[in]          char *ver      version string
 *
 * @return OTA_SUCCESS         OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old. 
 */
int ota_transport_inform(ota_service_t *ctx, char* ver)
{
    int  ret              = 0;
    char msg[OTA_MSG_LEN] = {0};
    ret = ota_snprintf(msg, OTA_MSG_LEN - 1, "{\"id\":%d,\"params\":{\"version\":\"%s\"}}", 0, ver);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    ret = ota_mqtt_publish(ctx->mqtt_client, "inform", msg, ctx->pk, ctx->dn);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    return ret;
}

int ota_transport_inform_subver(ota_service_t *ctx, char* ver)
{
    int  ret              = 0;
    char msg[OTA_MSG_LEN] = {0};
    ret = ota_snprintf(msg, OTA_MSG_LEN - 1, "{\"id\":%d,\"params\":{\"version\":\"%s\",\"module\":\"%s\"}}", 0, ver, ctx->module_name);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    ret = ota_mqtt_publish(ctx->mqtt_client, "inform", msg, ctx->pk, ctx->dn);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    return ret;
}

/**
 * ota_transport_upgrade  OTA subcribe message from Cloud.
 *
 * @param[in] ota_service_t *ctx ctx  ota service context
 *
 * @return OTA_SUCCESS         OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old. 
 */
int ota_transport_upgrade(ota_service_t *ctx)
{
    int  ret                = OTA_TRANSPORT_INT_FAIL;
    char topic[OTA_MSG_LEN] = {0};

    if(ctx == NULL) {
        return ret;
    }
    ret = ota_snprintf(topic, OTA_MSG_LEN - 1, "/ota/device/%s/%s/%s", "upgrade", ctx->pk, ctx->dn);
    if(ret < 0) {
        ret = OTA_TRANSPORT_INT_FAIL;
    }
    ret = ota_hal_mqtt_subscribe(ctx->mqtt_client, topic, ota_mqtt_sub_cb, ctx);
    return ret;
}

/**
 * ota_transport_status  OTA report status to Cloud
 *
 * @param[in] ota_service_t *ctx ctx  ota service context
 * @param[in] status    [1-100] percent, [<0] error no.
 *
 * @return OTA_SUCCESS         OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old. 
 */
int ota_transport_status(ota_service_t *ctx, int status)
{
    int  ret                = OTA_TRANSPORT_INT_FAIL;
    char msg[OTA_MSG_LEN]   = {0};
    char *err_str           = "";

    if(ctx == NULL) {
        return ret;
    }
    if (status < 0) {
        switch (status) {
            case OTA_INIT_FAIL:
                err_str = "OTA init failed";
                break;
            case OTA_TRANSPORT_INT_FAIL:
                err_str = "OTA transport init failed";
                break;
            case OTA_TRANSPORT_VER_FAIL:
                err_str = "OTA transport verion is too old";
                break;
            case OTA_TRANSPORT_PAR_FAIL:
                err_str = "OTA transport parse failed";
                break;
            case OTA_DOWNLOAD_INIT_FAIL:
                err_str = "OTA download init failed";
                break;
            case OTA_DOWNLOAD_HEAD_FAIL:
                err_str = "OTA download header failed";
                break;
            case OTA_DOWNLOAD_CON_FAIL:
                err_str = "OTA download connect failed";
                break;
            case OTA_DOWNLOAD_REQ_FAIL:
                err_str = "OTA download request failed";
                break;
            case OTA_DOWNLOAD_RECV_FAIL:
                err_str = "OTA download receive failed";
                break;
            case OTA_VERIFY_MD5_FAIL:
                err_str = "OTA verfiy MD5 failed";
                break;
            case OTA_VERIFY_SHA2_FAIL:
                err_str = "OTA verfiy SHA256 failed";
                break;
            case OTA_VERIFY_RSA_FAIL:
                err_str = "OTA verfiy RSA failed";
                break;
            case OTA_VERIFY_IMAGE_FAIL:
                err_str = "OTA verfiy image failed";
                break;
            case OTA_UPGRADE_WRITE_FAIL:
                err_str = "OTA upgrade write failed";
                break;
            case OTA_UPGRADE_FW_SIZE_FAIL:
                err_str = "OTA upgrade FW too big";
                break;
            case OTA_CUSTOM_CALLBAK_FAIL:
                err_str = "OTA register callback failed";
                break;
            case OTA_MCU_INIT_FAIL:
                err_str = "OTA MCU init failed";
                break;
            case OTA_MCU_VERSION_FAIL:
                err_str = "OTA MCU init failed";
                break;
            case OTA_MCU_NOT_READY:
                err_str = "OTA MCU not ready";
                break;
            case OTA_MCU_REBOOT_FAIL:
                err_str = "OTA MCU fail to reboot";
                break;
            case OTA_MCU_HEADER_FAIL:
                err_str = "OTA MCU header error";
                break;
            case OTA_MCU_UPGRADE_FAIL:
                err_str = "OTA MCU upgrade fail";
                break;
            case OTA_INVALID_PARAMETER:
                err_str = "OTA invalid parameter";
                break;
            default:
                err_str = "OTA undefined failed";
                break;
        }
    }
    if(ctx->module_ota == 0) {
        ret = ota_snprintf(msg, OTA_MSG_LEN - 1, "{\"id\":%d,\"params\":{\"step\": \"%d\",\"desc\":\"%s\"}}", 1, status, err_str);
    }
    else {
        ret = ota_snprintf(msg, OTA_MSG_LEN - 1, "{\"id\":%d,\"params\":{\"step\": \"%d\",\"desc\":\"%s\",\"module\":\"%s\"}}", 1, status, err_str, ctx->module_name);
    }
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    ret = ota_mqtt_publish(ctx->mqtt_client, "progress", msg, ctx->pk, ctx->dn);
    if (ret < 0) {
        return OTA_TRANSPORT_INT_FAIL;
    }
    return ret;
}

/**
 * ota_transport_init  OTA transport init
 *
 * @param[in] ota_service_t *ctx ctx  ota service context
 *
 * @return OTA_SUCCESS         OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old.
 */
int ota_transport_init(ota_service_t *ctx)
{
    int ret = -1;
    if(ctx != NULL) {
        if((ctx->dev_type == 0) && (ctx->mqtt_client == NULL)) {
            ret = ota_hal_mqtt_init();
            if(ret < 0){
                return OTA_TRANSPORT_INT_FAIL;
            }
        }
        else {
            ret = 0;
        }
    }
    return ret;
}

/**
 * ota_transport_init  OTA transport deinit
 *
 * @param[in] ota_service_t *ctx ctx  ota service context
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_TRANSPORT_INT_FAIL  OTA transport init fail.
 * @return OTA_TRANSPORT_PAR_FAIL  OTA transport parse fail.
 * @return OTA_TRANSPORT_VER_FAIL  OTA transport verion is too old.
 */
int ota_transport_deinit(ota_service_t *ctx)
{
    int ret = 0;
    ret = ota_hal_mqtt_deinit();
    if(ret < 0){
        ret = OTA_TRANSPORT_INT_FAIL;
    }
    return ret;
}
