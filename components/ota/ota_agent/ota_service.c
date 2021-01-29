/*
 *Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <string.h>
#include "ota_log.h"
#include "ota_import.h"
#include "ota_hal_os.h"
#include "ota_hal.h"

#ifdef AOS_COMP_PWRMGMT
#include "aos/pwrmgmt.h"
#endif

#ifdef OTA_CONFIG_UAGENT
static int ota_inform(void *pctx);
#endif /* OTA_CONFIG_UAGENT */

#if !defined (OTA_LINUX) && defined OTA_CONFIG_SECURE_DL_MODE
static wdg_dev_t ota_wdg = {0};
static aos_timer_t  ota_mon_tmr = {0};
static void ota_monitor_task(void *arg1, void* arg2)
{
    hal_wdg_reload(&ota_wdg);
}
#endif

int ota_service_submodule_start(ota_service_t *ctx)
{
    /* OTA download init */
    int ret = 0;
    char *version = NULL;
    ota_boot_param_t ota_param = {0};
    char file_name[64] = {0};
    ret = ota_read_parameter(&ota_param);
    if(ret < 0) {
        goto SUBDEV_EXIT;
    }
    if(ctx == NULL) {
        goto SUBDEV_EXIT;
    }
    ret = ota_download_init(ctx);
    if (ret < 0) {
        goto SUBDEV_EXIT;
    }
    ret = ota_get_storefile_subdev_name(file_name); 
    if(ret < 0){
        OTA_LOG_E("get store file path failed\n");
        goto SUBDEV_EXIT;
    }
    ret = ota_download_store_fs_start(ctx, ota_param.url, file_name);
    if (ret < 0) {
        goto SUBDEV_EXIT;
    }
    OTA_LOG_I("verify subdev ota fs file\n");
    ret = ota_verify_fsfile(&ota_param, file_name);
    if(ret < 0) {
        goto SUBDEV_EXIT;
    }
    OTA_LOG_I("upgrade sub module\n");
    ret = ota_subdev_upgrade_start((void*)ctx, ota_param.len, ota_param.ver);
    if(ret < 0) {
        goto SUBDEV_EXIT;
    }
    OTA_LOG_I("report submode version\n");
    version = (char *)ota_hal_version(ctx->dev_type, ctx->module_name);
    if (version == NULL) {
        ret = OTA_MCU_VERSION_FAIL;
        goto SUBDEV_EXIT;
    }
    ret = ota_transport_inform_subver(ctx, version);
    if (ret < 0) {
        goto SUBDEV_EXIT;
    }

SUBDEV_EXIT:
    OTA_LOG_I("Subdev download complete ret = %d\n", ret);
    if (ret < 0) {
        ota_param.upg_status = ret;
        if (ctx != NULL) {
            ota_transport_status(ctx, ret);
        }
    }
    ret = ota_clear();
    if(ret < 0) {
        ota_transport_status(ctx, ret);
    }
    ota_reboot_module(ctx->module_name);
    ota_thread_destroy(NULL);
}

/**
 * ota_service_maindev_fs_start  OTA service start: download and upgrade
 *
 * @param[in] void
 *
 * @return int
 */
int ota_service_maindev_fs_start(ota_service_t *ctx)
{
    int ret = 0;
    ota_boot_param_t ota_param = {0};
    char file_name[64] = {0};
    ret = ota_read_parameter(&ota_param);
    if(ret < 0) {
        goto MAINDEV_FS_EXIT;
    }
    OTA_LOG_I("download start upg_flag:0x%x \n", ota_param.upg_flag);
    /* OTA download init */
    ret = ota_download_init(ctx);
    if (ret < 0) {
        goto MAINDEV_FS_EXIT;
    }
    ret = ota_get_storefile_maindev_name(file_name); 
    if(ret < 0){
        OTA_LOG_E("get store file path failed\n");
        goto MAINDEV_FS_EXIT;
    }
    OTA_LOG_I("file_name = %s\rn", file_name);
    ret = ota_download_store_fs_start(ctx, ota_param.url, file_name);
    if (ret < 0) {
        goto MAINDEV_FS_EXIT;
    }
    OTA_LOG_I("verify maindev ota fs file\n");
    ret = ota_verify_fsfile(&ota_param, file_name);
    if(ret < 0) {
        goto MAINDEV_FS_EXIT;
    }
MAINDEV_FS_EXIT:
    OTA_LOG_I("Download complete, rebooting ret:%d.\n", ret);
    if (ret < 0) {
        ota_param.upg_status = ret;
        if (ctx != NULL) {
            ota_transport_status(ctx, ret);
        }
        ret = ota_clear();
        if(ret < 0) {
            ota_transport_status(ctx, ret);
        }
    } else {
        ota_param.crc = 0;
        ret = ota_hal_boot(&ota_param);
        if(ret < 0) {
            ota_transport_status(ctx, ret);
        }
    }
    if ((ctx != NULL) && (ctx->on_boot != NULL)) {
        ctx->on_boot(&ota_param);
    } else {
        if(ota_param.upg_flag != OTA_UPGRADE_DIFF) {
            ret = ota_hal_reboot_bank();
        }
        ota_reboot();
    }
    ota_thread_destroy(NULL);
    return ret;
}

/**
 * ota_service_start  OTA service start: download and upgrade
 *
 * @param[in] void
 *
 * @return int
 */
int ota_service_start(ota_service_t *ctx)
{
    int ret = 0;
    ota_boot_param_t ota_param = {0};
    ret = ota_read_parameter(&ota_param);
    if(ret < 0) {
        goto EXIT;
    }
    OTA_LOG_I("download start upg_flag:0x%x \n", ota_param.upg_flag);
#if !defined (OTA_LINUX) && defined OTA_CONFIG_SECURE_DL_MODE
    ota_wdg.config.timeout = 180000;
    hal_wdg_init(&ota_wdg);
    aos_timer_new(&ota_mon_tmr, ota_monitor_task, NULL, 3000, 1);
    if (ota_is_download_mode() == 0) {
        ret = OTA_INIT_FAIL;
        goto EXIT;
    }
#endif
#ifdef AOS_COMP_PWRMGMT
    aos_pwrmgmt_lowpower_suspend(PWRMGMT_OTA);
#endif
#if defined BOARD_ESP8266 && !defined OTA_CONFIG_SECURE_DL_MODE
    aos_task_delete("linkkit");
    ota_msleep(200);
#endif
    /* OTA download init */
    ret = ota_download_init(ctx);
    if (ret < 0) {
        goto EXIT;
    }
    /* parse image header */
    ctx->header.image_info = ota_malloc(sizeof(ota_image_info_t));
    if (ctx->header.image_info == NULL) {
        OTA_LOG_E("mem err.\n");
        ret = OTA_INIT_FAIL;
        goto EXIT;
    }
#ifdef OTA_CONFIG_LOCAL_RSA
    ctx->header.sign_info = ota_malloc(sizeof(ota_sign_info_t));
    if(ctx->header.sign_info == NULL) {
        OTA_LOG_E("mem err.\n");
        ret = OTA_INIT_FAIL;
        goto EXIT;
    }
#endif
    if (ota_param.upg_flag != OTA_UPGRADE_DIFF) {
        ret = ota_download_image_header(ctx, ota_param.url, ota_param.len);
        OTA_LOG_I("image header magic:0x%x ret:%d \n", ctx->header.image_info->image_magic, ret);
        if(ret < 0) {
            goto EXIT;
        }
        ota_param.upg_magic = ctx->header.image_info->image_magic;
#ifdef OTA_CONFIG_LOCAL_RSA
        OTA_LOG_I("image sign magic:0x%x\n", ctx->header.sign_info->encrypto_magic);
        if(ctx->header.sign_info->encrypto_magic == 0xAABBCCDD) {
            char tmp_buf[65];
            int result = 0;
            memset(tmp_buf, 0x00, sizeof(tmp_buf));
            memcpy(ota_param.sign, ctx->header.sign_info->signature, sizeof(ota_param.sign));
            ota_param.hash_type = OTA_SHA256;
            result = ota_hex2str(tmp_buf, (const unsigned char*)ctx->header.sign_info->hash, sizeof(tmp_buf), sizeof(ctx->header.sign_info->hash));
            if(result >= 0) {
                memcpy(ota_param.hash, tmp_buf, sizeof(tmp_buf));
            }
            else {
                OTA_LOG_E("sign info cpy err.");
                ret = OTA_DOWNLOAD_HEAD_FAIL;
                goto EXIT;
            }
        }
        else {
            OTA_LOG_E("sign info err.");
            ret = OTA_DOWNLOAD_HEAD_FAIL;
            goto EXIT;
        }
#endif
    }
    /* init ota partition */
    ret = ota_int(&ota_param);
    if (ret < 0) {
        ret = OTA_INIT_FAIL;
        goto EXIT;
    }
    /* download start */
    ret = ota_download_start(ctx, ota_param.url);
    if (ret < 0) {
        goto EXIT;
    }
    /* verify image */
    ret = ota_verify(&ota_param);
    if (ret < 0) {
        goto EXIT;
    }
    /* MCU firmware */
    if(ctx->header.image_info->image_magic == OTA_BIN_MAGIC_MCU) {
        OTA_LOG_I("Download complete, start MCU OTA.\n");
        ret = ota_mcu_upgrade_start(ota_param.len, ota_param.ver, ota_param.hash);
    }
EXIT:
    OTA_LOG_E("Download complete, rebooting ret:%d.\n", ret);
    if (ret < 0) {
        ota_param.upg_status = ret;
#if !defined BOARD_ESP8266 && !defined OTA_CONFIG_SECURE_DL_MODE
        if (ctx != NULL) {
#ifdef OTA_CONFIG_UAGENT
            if (OTA_PROCESS_UAGENT_OTA == ctx->ota_process) {
                ota_update_process("general reason", -1);
            } else
#endif /* OTA_CONFIG_UAGENT */
            {
                ota_transport_status(ctx, ret);
            }
        }
#endif /* !defined BOARD_ESP8266 && !defined OTA_CONFIG_SECURE_DL_MODE */
        ret = ota_clear();
        if(ret < 0) {
            ota_transport_status(ctx, ret);
        }
    } else {
        ota_param.crc = 0;
        ret = ota_hal_boot(&ota_param);
        if(ret < 0) {
            ota_transport_status(ctx, ret);
        }
    }
    if(ctx->header.image_info != NULL) {
        ota_free(ctx->header.image_info);
        ctx->header.image_info = NULL;
    }
    if(ctx->header.sign_info != NULL) {
        ota_free(ctx->header.sign_info);
        ctx->header.sign_info = NULL;
    }
#ifdef AOS_COMP_PWRMGMT
    aos_pwrmgmt_lowpower_resume(PWRMGMT_OTA);
#endif
    if ((ctx != NULL) && (ctx->on_boot != NULL)) {
        ctx->on_boot(&ota_param);
    } else {
        if(ota_param.upg_flag != OTA_UPGRADE_DIFF) {
            ret = ota_hal_reboot_bank();
        }
        ota_reboot();
    }
    ota_thread_destroy(NULL);
    return ret;
}

#ifdef OTA_CONFIG_UAGENT
static int on_ota_handler(void *p, const unsigned short len, void *str)
{
    int rc = -1;
    ota_service_t *ctx = (ota_service_t *)p;
    if (NULL != str && 0 != len && NULL != ctx) {
        cJSON *root = NULL;
        OTA_LOG_I("Handle ota payload %s", str);
        root = cJSON_Parse(str);
        if (NULL != root) {
            cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
            if (NULL != cmd) {
                if (cJSON_IsNumber(cmd)) {
                    if (OTA_REQ_VERSION == cmd->valueint) {
                        rc = 0;
                        if (0 != ota_inform((void*)ctx)) {
                            OTA_LOG_E("ota version report via uagent fail");
                        }
                    } else if (0 != (OTA_UPGRADE_SOC & cmd->valueint)) {
                        rc = 0;
                        ctx->ota_process = OTA_PROCESS_UAGENT_OTA;
                        ota_sevice_parse_msg(ctx, str);
                    } else {
                        OTA_LOG_W("unsupport command %d", cmd->valueint);
                    }
                }
            }
            cJSON_Delete(root);
        }
        if (0 != rc) {
            ota_update_process("upgrade command not support", -1);
        }
    }
    return rc;
}

static int ota_inform(ota_service_t *ctx)
{
    char info_msg[OTA_MSG_LEN] = { 0 };
    if(ctx == NULL) {
        return -1;
    }
    const int len = ota_snprintf(info_msg, sizeof(info_msg), "{\"version\":[ {\"type\":%d, \"version\":\"%s\"}]}",
        OTA_UPGRADE_SOC, ota_hal_version(ctx->dev_type, ctx->dn));
    OTA_LOG_I("version report %s", ota_hal_version(ctx->dev_type, ctx->dn));
    return uagent_send(UAGENT_MOD_OTA, VERSION_INFORM, len < OTA_MSG_LEN ? len : OTA_MSG_LEN,
        info_msg, send_policy_object);
}

int ota_update_process(const char *error_description, const int step)
{
    int rc = -1;
    char msg[OTA_MSG_LEN] = { 0 };

    if (NULL != error_description) {
        rc = ota_snprintf(msg, sizeof(msg), "{\"step\": %d,\"desc\":\"%s\"}", step, error_description);
    } else {
        rc = ota_snprintf(msg, sizeof(msg), "{\"step\": %d,\"desc\":\"%s\"}", step, "");
    }
    if (rc > 0) {
        rc = uagent_send(UAGENT_MOD_OTA, OTA_INFO, rc < OTA_MSG_LEN ? rc : OTA_MSG_LEN, msg, send_policy_object);
    }
    return rc;
}

static uagent_func_node_t ota_uagent_funclist[] =
{
    {OTA_INFO,        "otainfo", NULL,           NULL, &ota_uagent_funclist[1]},
    {VERSION_INFORM,  "inform",  NULL,           NULL, &ota_uagent_funclist[2]},
    {OTA_UPGRADE_CMD, "otacmd",  on_ota_handler, NULL, NULL}
};

static mod_func_t ulog_uagent_func =
{
    { UAGENT_MOD_OTA, 3, "OTA", OTA_VERSION },
    ota_uagent_funclist,
};
#endif /* OTA_CONFIG_UAGENT */

/**
 * ota_register_cb  OTA register callback
 *
 * @param[in] int id, void* cb
 *
 * @return OTA_SUCCESS         OTA register callback success.
 * @return OTA_CUSTOM_CALLBAK_FAIL OTA register callback fail.
 */
int ota_register_cb(ota_service_t *ctx, int id, void* cb)
{
    if(ctx == NULL) {
        return OTA_CUSTOM_CALLBAK_FAIL;
    }
    switch (id) {
        case OTA_CB_ID_UPGRADE:
           ctx->on_upgrade = cb;
        break;
        case OTA_CB_ID_MODULE_UPGRADE:
           ctx->on_module_upgrade = cb;
        break;
        case OTA_CB_ID_DATA:
           ctx->on_data = cb;
        break;
        case OTA_CB_ID_PERCENT:
           ctx->on_percent = cb;
        break;
        case OTA_CB_ID_BOOT:
           ctx->on_boot = cb;
        break;
        default:
           OTA_LOG_E("Err CB ID.");
           return OTA_CUSTOM_CALLBAK_FAIL;
        break;
    }
    return 0;
}

/**
 * ota_service_init  OTA service init
 *
 * @param[in] ota_service_t *ctx OTA service context
 *
 * @return OTA_SUCCESS         OTA service init success.
 * @return OTA_INIT_FAIL       OTA service init fail.
 */
int ota_service_init(ota_service_t *ctx)
{
    int ret = 0;
    char *version = NULL;

    if(ctx == NULL) {
        ret = OTA_INIT_FAIL;
        goto EXIT;
    }
    /* inform version to cloud */
    version = (char *)ota_hal_version(ctx->dev_type, NULL);
    if (version == NULL) {
        ret = OTA_INIT_FAIL;
        goto EXIT;
    } 
#if defined OTA_CONFIG_BLE
    extern on_ota_bool_pfunc ota_ble_status_get_cb;
    extern on_ota_int_pfunc ota_ble_user_store_data_process_cb;
    ota_ble_status_get_cb = ctx->on_ota_status_cb;
    ota_ble_user_store_data_process_cb = ctx->on_ota_user_store_data_handle;
    ctx->on_message =  ota_ble_transport_msg;
    OTA_LOG_I("BLE OTA init success.\r\n");
    return ret;
#else
    /* transport init */
    ret = ota_transport_init(ctx);
    if (ret < 0) {
        goto EXIT;
    }
    ctx->ota_process = OTA_PROCESS_NORMAL;
#ifdef OTA_CONFIG_UAGENT
    OTA_LOG_I("register ota cb into uagent");
    uagent_func_node_t *p = ulog_uagent_func.header;
    while (NULL != p) {
        p->argu = (void*)ctx;
        if (0 != uagent_register(ulog_uagent_func.mod_info.mod, ulog_uagent_func.mod_info.name,
            ulog_uagent_func.mod_info.version, p->func, p->func_name,
            p->service, p->argu)) {
            OTA_LOG_E("register into uagent fail");
        }
        p = p->next;
    }
    if (0 != ota_inform(p->argu)) {
        OTA_LOG_E("uagent inform fail");
    }
#endif /* OTA_CONFIG_UAGENT */
    ret = ota_transport_inform(ctx, version);
    if (ret < 0) {
        goto EXIT;
    }
    if(strlen(ctx->module_name) != 0) {
        version = (char *)ota_hal_version(ctx->dev_type, ctx->module_name);
        ret = ota_transport_inform_subver(ctx, version);
    }
    /* subcribe upgrade */
    ret = ota_transport_upgrade(ctx);
    if (ret < 0) {
        goto EXIT;
    }
    /* rollback */
    if(ctx->dev_type == 0) {//master dev ota
        ret = ota_hal_rollback();
        if (ret < 0) {
            ret = OTA_INIT_FAIL;
            goto EXIT;
        }
    }
    OTA_LOG_I("ota init success, ret:%d", ret);
    return ret;
#endif
EXIT:
    OTA_LOG_E("ota init fail, ret:%d", ret);
    return ret;
}

/**
 * ota_service_deinit  OTA service deinit
 *
 * @param[in] ota_service_t *ctx OTA service context
 *
 * @return OTA_SUCCESS         OTA service deinit success.
 * @return OTA_INIT_FAIL       OTA service deinit fail.
 */
int ota_service_deinit(ota_service_t* ctx)
{
    int ret = 0;
    ret = ota_transport_deinit(ctx);
    if(ret < 0) {
        OTA_LOG_E("transport deinit failed.");
        return ret;
    }
    ret = ota_download_deinit(ctx);
    if(ret < 0) {
        OTA_LOG_E("download deinit failed.");
        return ret;
    }
    return ret;
}
