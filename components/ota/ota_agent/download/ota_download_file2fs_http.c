/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ota_log.h"
#include "ota_hal_os.h"
#include "ota_import.h"
#include "ota_hal_trans.h"
#include <http.h>

int ota_get_upgrade_status(void);
void ota_set_upgrade_status(char is_upgrade);

/**
 * ota_download_store_fs_start    OTA download file start and store in fs
 *
 * @param[in] ota_service_t* ctx  device information
 * @param[in] char *url           download url
 * @param[in] char *store_path    store file path and name eg:/root/test.bin
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_DOWNLOAD_INIT_FAIL  OTA download init failed.
 * @return OTA_DOWNLOAD_CON_FAIL   OTA download connect failed.
 * @return OTA_DOWNLOAD_REQ_FAIL   OTA download request failed.
 * @return OTA_DOWNLOAD_RECV_FAIL  OTA download receive failed.
 */
int ota_download_store_fs_start(ota_service_t *ctx, char *url, char *store_path)
{
    int j = 0;
    void *fptr = NULL;
    char *hdr = NULL;
    int  ret = OTA_DOWNLOAD_INIT_FAIL;
    unsigned int offset = 0;
    unsigned int off_size = 0;
    http_rsp_info_t rsp_info = {0};
    char *content = NULL;
    char host_name[128] = {0};
    char host_uri[256] = {0};
    int ota_rx_size = 0;
    int ota_file_size = 0;
    unsigned char ota_header_found = false;
    httpc_connection_t settings = {0};
    httpc_handle_t httpc_handle = 0;
    int percent                 = 0;
    int divisor                 = 5;
    if((ctx == NULL) || (store_path == NULL)) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    if(ota_get_upgrade_status() == 1) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    hdr = ota_malloc(OTA_HTTP_HEAD_LEN);
    if(hdr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    memset(hdr, 0x00, OTA_HTTP_HEAD_LEN); 
    fptr = ota_fopen(store_path, "wb");
    if(fptr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        OTA_LOG_E("open %s failed\n", store_path);
        goto EXIT;
    }
    ota_download_parse_host_uri(url, host_name, host_uri);
    for(j = OTA_DOWNLOAD_RETRY_CNT; (j > 0) && (ret < 0); j--) {
       ret = ota_httpc_settings_init(host_name, &settings, ctx);
       if(ret < 0) {
           ret = OTA_DOWNLOAD_INIT_FAIL;
           goto EXIT;
       }
       httpc_handle = httpc_init(&settings);
       if (httpc_handle == 0) {
           ret = OTA_DOWNLOAD_INIT_FAIL;
           goto EXIT;
       }
       if (j >= OTA_DOWNLOAD_RETRY_CNT) {
           ret = httpc_construct_header(hdr, OTA_HTTP_HEAD_LEN, "Accept", "*/*");
           if (ret < 0) {
                ret = OTA_DOWNLOAD_CON_FAIL;
                goto EXIT;
           }
       } else {
           ota_msleep(6000);
           char resume[64] = {0};
           OTA_LOG_E("reconnect retry:%d ret:%d rx_size:%d \n", j, ret, off_size);
           ota_snprintf(resume, 64, "bytes=%d-", off_size);
           ret = httpc_construct_header(hdr, OTA_HTTP_HEAD_LEN, "Range", resume);
           if (ret < 0) {
                ret = OTA_DOWNLOAD_CON_FAIL;
                goto EXIT;
           }
       }
       ret = ota_httpc_request_send(httpc_handle, host_uri, hdr, OTA_DOWNLOAD_RETRY_CNT);
       if (ret < 0) {
           ret = OTA_DOWNLOAD_REQ_FAIL;
           goto EXIT;
       }
       ota_set_upgrade_status(1);
       if(settings.req_buf == NULL) {
           ret = OTA_TRANSPORT_PAR_FAIL;
           goto EXIT;
       }
       memset(settings.req_buf, 0, settings.req_buf_size);
       OTA_LOG_E("ota download begin....\n");
       while (ota_file_size == 0 || ota_rx_size < ota_file_size) {
           ret = ota_httpc_recv_data(httpc_handle, settings.req_buf, &rsp_info, OTA_DOWNLOAD_RETRY_CNT);
           if (ota_get_upgrade_status() == 0) {
                OTA_LOG_E("download stop.\n");
                ret = OTA_DOWNLOAD_RECV_FAIL;
                break;
           } else if (ret < 0) {
                ret = OTA_DOWNLOAD_RECV_FAIL;
                break;
           } else {
                if (rsp_info.body_present || rsp_info.message_complete) {
                    if (ota_header_found == false) {
                        if (ota_file_size == 0) {
                             content = strstr((const char *)settings.req_buf, "Content-Length");
                             OTA_LOG_I("recvd head = %s\n", content);
                             if (content) {
                                 ret = sscanf(content, "%*[^ ]%d", &ota_file_size);
                                 if (ret < 0) {
                                     OTA_LOG_I("header fail\n");
                                     break;
                                 }
                                 ota_header_found = true;
                                 OTA_LOG_I("header file size %d\r\n", ota_file_size);
                             } else {
                                 continue;
                             }
                         }
                         content = strstr((const char *)settings.req_buf, "\r\n\r\n");
                         if (content) {
                             content += 4;
                             ota_rx_size = rsp_info.rsp_len - ((unsigned char *)content - settings.req_buf);
                             if((ctx != NULL)&&(ctx->on_data != NULL)) {
                                 //ret = ctx->on_data(content, ota_rx_size);
                                
                             } else {
                                 ret = ota_fwrite(content, ota_rx_size, 1, fptr);
                             }
                             if (ret < 0) {
                                 ret = OTA_UPGRADE_WRITE_FAIL;
                                 goto EXIT;
                             }
                         }
                    } else {
                         if((ctx != NULL)&&(ctx->on_data != NULL)) {
                             //ret = ctx->on_data((char *)settings.req_buf, rsp_info.rsp_len);
                         } else {
                            ret = ota_fwrite((char *)settings.req_buf, rsp_info.rsp_len, 1, fptr);
                         }
                         if (ret < 0) {
                             ret = OTA_UPGRADE_WRITE_FAIL;
                             goto EXIT;
                         }
                         ota_rx_size += rsp_info.rsp_len;
                    }
                    ota_msleep(5);
                    off_size = ota_rx_size;
                    if(ota_file_size) {
                         percent = ((long)(ota_rx_size >> 6) * 100) / (long)(ota_file_size >> 6);
                         if(percent / divisor) {
                             divisor += 5;
                             if((ctx != NULL)&&(ctx->on_percent != NULL)) {
                                 ctx->on_percent(percent);
                             } else {
#if !defined BOARD_ESP8266
                                 if (ctx != NULL) {
#ifdef OTA_CONFIG_UAGENT
                                     OTA_LOG_I("download process %d", ctx->ota_process);
                                     if (OTA_PROCESS_UAGENT_OTA == ctx->ota_process) {
                                         ota_update_process(NULL, percent);
                                     } else
#endif /* OTA_CONFIG_UAGENT */
                                     {
                                         ota_transport_status(ctx, percent);
                                     }
                                }else{
                                    OTA_LOG_W("in fs download ctx NULL");
                                }
#endif /* !defined BOARD_ESP8266 */
                             }
                             OTA_LOG_I(" in fs recv data(%d/%d) off:%d \r\n", ota_rx_size, ota_file_size, off_size);
                         }
                     }
                 }
             }
         }

EXIT:
        ota_set_upgrade_status(0);
        if(fptr != NULL) {
            (void)ota_fclose(fptr);
        }
        ota_httpc_settings_destory(&settings);
        if(httpc_handle > 0) {
            httpc_deinit(httpc_handle);
            httpc_handle = 0;
        }
        httpc_handle = 0;
        ota_header_found = false;
        ota_file_size = 0;
        ota_rx_size = 0;
    }
    if(hdr != NULL) {
        ota_free(hdr);
        hdr = NULL;
    }
    OTA_LOG_E("in fs download complete:%d \n", ret);
    return ret;
}