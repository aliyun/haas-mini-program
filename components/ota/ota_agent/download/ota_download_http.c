/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ota_log.h"
#include "ota_hal_os.h"
#include "ota_import.h"
#include "ota_hal_trans.h"

#include <http.h>
#include "http_parser.h"

static int ota_upgrading = 0;
static unsigned char dl_buf[OTA_DOWNLOAD_BLOCK_SIZE] = {0};
#if defined OTA_CONFIG_SECURE_DL_MODE
static const char *ca_cert = \
{   \
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n" \
    "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n" \
    "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n" \
    "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n" \
    "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n" \
    "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n" \
    "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n" \
    "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n" \
    "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n" \
    "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n" \
    "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n" \
    "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n" \
    "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n" \
    "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n" \
    "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n" \
    "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n" \
    "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n" \
    "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n" \
    "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n" \
    "-----END CERTIFICATE-----"
};
#endif

#if defined OTA_CONFIG_ITLS
static char pkps[128];
#endif

void ota_set_upgrade_status(char is_upgrade)
{
    ota_upgrading = is_upgrade;
}

int ota_get_upgrade_status()
{
    return ota_upgrading;
}

/**
 * ota_httpc_settings_init       init httpc settings
 *
 * @param[in] char               *host_name  host name
 * @param[in] httpc_connection_t *settings   httpc settings
 *
 * @return 0  success
 * @return -1 fail
 */
int ota_httpc_settings_init(char *host_name, httpc_connection_t *settings, ota_service_t *ctx)
{
    int fd = 0;
    if (host_name == NULL || settings == NULL) {
        return -1;
    }
#if defined OTA_CONFIG_ITLS
    if(ctx == NULL) {
        return -1;
    }
#endif
    fd = ota_hal_socket();
    if (fd < 0) {
        return -1;
    }
    memset(settings, 0, sizeof(httpc_connection_t));
    settings->socket = fd;
    settings->server_name = host_name;
#if defined OTA_CONFIG_ITLS
    OTA_LOG_I("init itls ota.\n");
    memset(pkps, 0x00, sizeof(pkps));
    strncpy(pkps, ctx->pk, strlen(ctx->pk));
    strncpy(pkps + strlen(ctx->pk) + 1, ctx->ps, strlen(ctx->ps));
    settings->ca_cert = pkps;
#elif defined OTA_CONFIG_SECURE_DL_MODE
    OTA_LOG_I("init https ota.\n");
    settings->ca_cert = ca_cert;
#else
    OTA_LOG_I("init http ota.\n");
#endif
    settings->req_buf = dl_buf;
    settings->req_buf_size = OTA_DOWNLOAD_BLOCK_SIZE;
    return 0;
}

/**
 * ota_httpc_settings_destory  destory httpc settings
 *
 * @param[in] httpc_connection_t *settings  httpc settings
 *
 * @return void
 */
void ota_httpc_settings_destory(httpc_connection_t *settings)
{
    if(settings->socket > 0) {
        ota_hal_close(settings->socket);
        settings->socket = 0;
    }
    settings->req_buf = NULL;
}

/**
 * ota_httpc_request_send     OTA send request to server
 *
 * @param[in] httpc_handle_t  httpc_handle  httpc handle
 * @param[in] char            *host_uri     host uri
 * @param[in] char            *hdr          httpc request content
 * @param[in] unsigned char   retry_tm      retry time when received fail
 *
 * @return  0  success.
 * @return -1  failed.
 */
int ota_httpc_request_send(httpc_handle_t httpc_handle, char *host_uri, char *hdr, unsigned char retry_tm)
{
    int ret = -1;
    int i = 0;
    if(httpc_handle == 0 || host_uri == NULL || hdr == NULL) {
        return ret;
    }
    for(i = 0; i < retry_tm; i++){
        ret = httpc_send_request(httpc_handle, HTTP_GET, host_uri, hdr, NULL, NULL, 0);
        if(ret >= 0) {
            break;
        }
        ota_msleep(1500);
        OTA_LOG_E("send cnt:%d ret:%d \n", i, ret);
    }
    return ret;
}

/**
 * ota_httpc_recv_data        OTA receive data from httpc
 *
 * @param[in] httpc_handle_t  httpc_handle  httpc handle
 * @param[in] unsigned char   *buf          receive data buffer
 * @param[in] http_rsp_info_t *rsp_info     httpc response infomation
 * @param[in] unsigned char   retry_tm      retry time when received fail
 *
 * @return  0  success.
 * @return -1  failed.
 */
int ota_httpc_recv_data(httpc_handle_t httpc_handle, unsigned char *buf, http_rsp_info_t *rsp_info, unsigned char retry_tm)
{
   int ret = -1;
   int i = 0;
   if(httpc_handle == 0 || buf == NULL || rsp_info == NULL) {
        return ret;
    }
    for(i = 0; i < retry_tm; i++) {
        ret = httpc_recv_response(httpc_handle, buf, OTA_DOWNLOAD_BLOCK_SIZE, rsp_info, OTA_DOWNLOAD_TIMEOUT);
        if ((ret == HTTP_ERECV) || (ret == HTTP_ETIMEOUT)) {
            ota_msleep(1500);
            OTA_LOG_E("recv retry:%d ret:%d \n", i, ret);
            continue;
        }
        break;
    }
    return ret;
}

/**
 * ota_download_parse_host_uri  OTA parse host url
 *
 * @param[in] char *url           pasre url to host name & uri.
 *
 * @return host_name              host name from download url.
 * @return host_uri               host uri from download url.
 */
int ota_download_parse_host_uri(char *url, char *name, char *uri)
{
    int ret = 0;
    char *pa = url;
    char *pb = NULL;
    unsigned int len = 0;
    if ((url == NULL) || (strlen(url) == 0) || (name == NULL) || (uri == NULL)) {
        OTA_LOG_E("url parms error!");
        return OTA_DOWNLOAD_INIT_FAIL;
    }
    if (!(*url)) {
        return OTA_DOWNLOAD_INIT_FAIL;
    }
    if (!strncmp(pa, "https://", 8)) {
        pa = url + 8;
    }
    if (!strncmp(pa, "http://", 7)) {
        pa = url + 7;
    }
    pb = strchr(pa, '/');
    if (pb) {
        len = pb - url;
        strncpy(name, url, len);
#ifndef OTA_CONFIG_SECURE_DL_MODE
        if(name[4] == 's') {
            int i = 0;
            for(i = 4; i < len; i++) {
                name[i] = name[i + 1];
            }
            name[i] = 0;
        }
#endif
        pb += 1;
        strncpy(uri, pb, strlen(pb));
    }
    OTA_LOG_I("parse host name:%s  uri:%s \n", name, uri);
    return ret;
}

/**
 * ota_download_image_header  OTA download image header
 *
 * @param[in] char *url         download url
 * @param[in] unsigned int size image size
 * @param[out] void *pctx       image information with image header
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_DOWNLOAD_INIT_FAIL  OTA download init failed.
 * @return OTA_DOWNLOAD_CON_FAIL   OTA download connect failed.
 * @return OTA_DOWNLOAD_REQ_FAIL   OTA download request failed.
 * @return OTA_DOWNLOAD_RECV_FAIL  OTA download receive failed.
 */
int ota_download_image_header(ota_service_t *ctx, char *url, unsigned int size)
{
    int  ret = OTA_DOWNLOAD_INIT_FAIL;
    char *hdr = NULL;
    char host_name[128] = {0};
    char host_uri[256] = {0};
    unsigned int off_size = 0;
    http_rsp_info_t rsp_info = {0};
    char *content = NULL;
    char resume[64] = {0};
    int retry_tm = 0;
    int ota_rx_size = 0;
    int want_download_size = 0;
    unsigned char ota_header_found = false;
    httpc_connection_t settings = {0};
    httpc_handle_t httpc_handle = 0;
#ifdef OTA_CONFIG_LOCAL_RSA
    char *sign_info_ptr         = NULL;
#endif
    char *image_info_ptr        = NULL;
    if((ctx == NULL) || (url == NULL) || (ota_get_upgrade_status() == 1)) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
#ifdef OTA_CONFIG_LOCAL_RSA
    sign_info_ptr = (char *)ctx->header.sign_info;
    if(sign_info_ptr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
#endif
    image_info_ptr = (char *)ctx->header.image_info;
    if(image_info_ptr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    hdr = ota_malloc(OTA_HTTP_HEAD_LEN);
    if(hdr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    memset(hdr, 0, OTA_HTTP_HEAD_LEN);
    ota_download_parse_host_uri(url, host_name, host_uri);
#ifdef OTA_CONFIG_LOCAL_RSA
    off_size = size - sizeof(ota_image_info_t) - sizeof(ota_sign_info_t);
#else
    off_size = size - sizeof(ota_image_info_t);
#endif
    for(retry_tm = OTA_DOWNLOAD_RETRY_CNT; (retry_tm > 0) && (ret < 0); retry_tm--) {
       if(retry_tm < OTA_DOWNLOAD_RETRY_CNT) {
           OTA_LOG_I("retry count.\n");
           ota_msleep(6000);
       }
       ret = ota_httpc_settings_init(host_name, &settings, ctx);
       if(ret < 0) {
           ret = OTA_DOWNLOAD_INIT_FAIL;
           goto OVER;
       }
       httpc_handle = httpc_init(&settings);
       if (httpc_handle == 0) {
           ret = OTA_DOWNLOAD_INIT_FAIL;
           goto OVER;
       }
       OTA_LOG_I("retry:%d ret:%d rx_size:%d \n", retry_tm, ret, off_size);
       ota_snprintf(resume, 64, "bytes=%d-", off_size);
       ret = httpc_construct_header(hdr, OTA_HTTP_HEAD_LEN, "Range", resume);
       if (ret < 0) {
            ret = OTA_DOWNLOAD_CON_FAIL;
            goto OVER;
       }
       if(settings.req_buf == NULL) {
           ret = OTA_TRANSPORT_PAR_FAIL;
           goto OVER;
       }
       memset(settings.req_buf, 0, OTA_DOWNLOAD_BLOCK_SIZE);
       ret = ota_httpc_request_send(httpc_handle, host_uri, hdr, OTA_DOWNLOAD_RETRY_CNT);
       if (ret < 0) {
           ret = OTA_DOWNLOAD_REQ_FAIL;
           goto OVER;
       }
       ota_set_upgrade_status(1);
       while (want_download_size == 0 || ota_rx_size < want_download_size) {
           ret = ota_httpc_recv_data(httpc_handle, settings.req_buf, &rsp_info, OTA_DOWNLOAD_RETRY_CNT);
           if (ret < 0) {
                ret = OTA_DOWNLOAD_RECV_FAIL;
                break;
           }
           else {
                if (rsp_info.body_present || rsp_info.message_complete) {
                    int tmp_size = rsp_info.rsp_len;
                    content = (char*)settings.req_buf;
                    if (ota_header_found == false) {
                        ota_header_found = true;
                        content = strstr((const char *)settings.req_buf, "Content-Length");
                        if (content) {
                            ret = sscanf(content, "%*[^ ]%d", &want_download_size);
                            if (ret >= 0) {
                               OTA_LOG_I("header file size %d\r\n", want_download_size);
                               content = strstr((const char *)settings.req_buf, "\r\n\r\n");
                               if (content) {
                                   content += 4;
                                   tmp_size = rsp_info.rsp_len - ((unsigned char *)content - settings.req_buf);
                                } else {
                                   break;
                                }
                            } else {
                               break;
                            }
                        } else {
                            ota_header_found = false;
                            continue;
                        }
                    }
                    if(ota_rx_size + tmp_size <= off_size) {
#ifdef OTA_CONFIG_LOCAL_RSA
                        if(ota_rx_size  + tmp_size <= sizeof(ota_sign_info_t)) {
                            memcpy(sign_info_ptr, content, tmp_size);
                            sign_info_ptr += tmp_size;
                            ota_rx_size += tmp_size;
                        }
                        else {
                            if(ota_rx_size <= sizeof(ota_sign_info_t)) {
                                unsigned int remain_len = sizeof(ota_sign_info_t) - ota_rx_size;
                                memcpy(sign_info_ptr, content, remain_len);
                                sign_info_ptr += remain_len;
                                content += remain_len;
                                memcpy(image_info_ptr, content, tmp_size - remain_len);
                                image_info_ptr += tmp_size - remain_len;
                                ota_rx_size += tmp_size;
                            }
                            else {
                                memcpy(image_info_ptr, content, tmp_size);
                                image_info_ptr += tmp_size;
                                ota_rx_size += tmp_size;
                            }
                        }
#else
                        memcpy(image_info_ptr, content, tmp_size);
                        image_info_ptr += tmp_size;
                        ota_rx_size += tmp_size;
#endif
                    }
                    else {
                        OTA_LOG_E("image head buf full");
                        ret = OTA_DOWNLOAD_RECV_FAIL;
                        retry_tm = 0;
                        break;
                    }
                 }
             }
         }
OVER:
         ota_set_upgrade_status(0);
         ota_httpc_settings_destory(&settings);
         if(httpc_handle > 0) {
             httpc_deinit(httpc_handle);
             httpc_handle = 0;
         }
         ota_header_found = false;
         want_download_size = 0;
         ota_rx_size = 0;
    }
    if(hdr != NULL) {
        ota_free(hdr);
        hdr = NULL;
    }
    OTA_LOG_E("parse image info:%d \n", ret);
    return ret;
}
/**
 * ota_download_start    OTA download start
 *
 * @param[in] ota_service_t* ctx  device information
 * @param[in] char *url   download url
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_DOWNLOAD_INIT_FAIL  OTA download init failed.
 * @return OTA_DOWNLOAD_CON_FAIL   OTA download connect failed.
 * @return OTA_DOWNLOAD_REQ_FAIL   OTA download request failed.
 * @return OTA_DOWNLOAD_RECV_FAIL  OTA download receive failed.
 */
int ota_download_start(ota_service_t* ctx, char *url)
{
    char *hdr = NULL;
    int  ret = OTA_DOWNLOAD_INIT_FAIL;
    unsigned int offset = 0;
    unsigned int off_size = 0;
    http_rsp_info_t rsp_info = {0};
    char *content = NULL;
    char host_name[128] = {0};
    char host_uri[256] = {0};
    int j = 0;
    int ota_rx_size = 0;
    int ota_file_size = 0;
    unsigned char ota_header_found = false;
    httpc_connection_t settings = {0};
    httpc_handle_t httpc_handle = 0;
    int percent                 = 0;
    int divisor                 = 5;
    if(ctx == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    if(ota_get_upgrade_status() == 1) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    hdr = ota_malloc(OTA_HTTP_HEAD_LEN);
    memset(hdr, 0x00, OTA_HTTP_HEAD_LEN);
    if(hdr == NULL) {
        ret = OTA_DOWNLOAD_INIT_FAIL;
        return ret;
    }
    memset(hdr, 0, OTA_HTTP_HEAD_LEN);
    ota_download_parse_host_uri(url, host_name, host_uri);
    for(j = OTA_DOWNLOAD_RETRY_CNT; (j > 0)&&(ret < 0); j--) {
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
                                 ret = ctx->on_data(content, ota_rx_size);
                             } else {
                                 ret = ota_write(&offset, content, ota_rx_size);
                             }
                             if (ret < 0) {
                                 ret = OTA_UPGRADE_WRITE_FAIL;
                                 goto EXIT;
                             }
                         }
                    } else {
                         if((ctx != NULL)&&(ctx->on_data != NULL)) {
                             ret = ctx->on_data((char *)settings.req_buf, rsp_info.rsp_len);
                         } else {
                             ret = ota_write(&offset, (char *)settings.req_buf, rsp_info.rsp_len);
                         }
                         if (ret < 0) {
                             ret = OTA_UPGRADE_WRITE_FAIL;
                             goto EXIT;
                         }
                         ota_rx_size += rsp_info.rsp_len;
                    }
                    ota_msleep(5);
                    off_size = ota_rx_size;
                    //OTA_LOG_E("ota (%d/%d) recv len:%d off:%d \r\n", ota_rx_size, ota_file_size, rsp_info.rsp_len, off_size);
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
                                    OTA_LOG_W("download ctx NULL");
                                }
#endif /* !defined BOARD_ESP8266 */
                             }
                             OTA_LOG_I("ota recv data(%d/%d) off:%d \r\n", ota_rx_size, ota_file_size, off_size);
                         }
                     }
                 }
             }
         }

EXIT:
        ota_set_upgrade_status(0);
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
    OTA_LOG_E("download complete:%d \n", ret);
    return ret;
}

/**
 * ota_download_init  OTA download init
 *
 * @param[in] ota_service_t* ctx  device information
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_DOWNLOAD_INIT_FAIL  OTA download init failed.
 * @return OTA_DOWNLOAD_CON_FAIL   OTA download connect failed.
 * @return OTA_DOWNLOAD_REQ_FAIL   OTA download request failed.
 * @return OTA_DOWNLOAD_RECV_FAIL  OTA download receive failed.
 */
int ota_download_init(ota_service_t* ctx)
{
    int ret = 0;
    static char is_http_inited = 0;
    if(is_http_inited == 0) {
        ret = http_client_initialize();
        if(ret < 0) {
            ret = OTA_DOWNLOAD_INIT_FAIL;
        }
        else {
            is_http_inited = 1;
        }
    }
    return ret;
}

/**
 * ota_download_deinit  OTA download deinit
 *
 * @param[in] ota_service_t* ctx  device information
 *
 * @return OTA_SUCCESS             OTA success.
 * @return OTA_DOWNLOAD_INIT_FAIL  OTA download init failed.
 * @return OTA_DOWNLOAD_CON_FAIL   OTA download connect failed.
 * @return OTA_DOWNLOAD_REQ_FAIL   OTA download request failed.
 * @return OTA_DOWNLOAD_RECV_FAIL  OTA download receive failed.
 */
int ota_download_deinit(ota_service_t* ctx)
{
    int ret = 0;
    ota_set_upgrade_status(0);
    return ret;
}
