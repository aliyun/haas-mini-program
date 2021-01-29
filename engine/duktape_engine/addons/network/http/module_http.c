/*
 * Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "amp_platform.h"
#include "amp_system.h"
#include "amp_network.h"
#include "amp_defines.h"
#include "amp_network.h"
#include "amp_platform.h"
#include "amp_socket.h"
#include "amp_httpc.h"
#include "amp_task.h"
#include "amp_fs.h"
#include "be_inl.h"
#include "ota_socket.h"
#include "http.h"

#include "infra_list.h"

#define MOD_STR                     "HTTP"
#define HTTP_BUFF_SIZE              2048
#define HTTP_HEADER_SIZE            1024
#define HTTP_HEADER_COUNT           8
#define HTTP_SEND_RECV_TIMEOUT      10000
#define HTTP_REQUEST_TIMEOUT        30000
#define HTTP_DEFAULT_HEADER_NAME    "content-type"
#define HTTP_DEFAULT_HEADER_DATA    "application/json"

typedef struct
{
    char *name;
    char *data;
} http_header_t;

typedef struct
{
    char *url;
    char *filepath;
    int method;
    http_header_t http_header[HTTP_HEADER_COUNT];
    uint32_t timeout;
    char *buffer;
    int js_cb_ref;
} http_param_t;

typedef struct
{
    char *url;
    char name[128];
    char path[128];
    int method;
    http_header_t http_header[HTTP_HEADER_COUNT];
    uint32_t timeout;
    int error;
    int js_cb_ref;
    int header_index;
    int index;
} http_download_param_t;

typedef struct {
    int js_cb_ref;
    int error;
} http_download_resp_t;

typedef struct
{
    int port;
    char hostname[128];
    char filepath[128];
} http_uri_t;

uint8_t req_buf[HTTP_BUFF_SIZE];
uint8_t rsp_buf[HTTP_BUFF_SIZE];
httpc_handle_t httpc_handle = 0;
static int http_header_index = 0;

typedef struct {
    http_download_param_t *dload_param;
    dlist_t node;
} http_dload_node_t;

static dlist_t g_dload_list = LIST_HEAD_INIT(g_dload_list);

static int dload_list_count = 0;

static char *strncasestr(const char *str, const char *key)
{
    int len;

    if (!str || !key)
        return NULL;

    len = strlen(key);
    if (len == 0)
        return NULL;

    while (*str) {
        if (!strncasecmp(str, key, len))
            return str;
        ++str;
    }
    return NULL;
}

static void parse_url(const char *url, char *uri)
{
    char url_dup[1024] = {0};

    if (url == NULL) {
        amp_warn(MOD_STR, "url is null");
        return;
    }
    memcpy(url_dup, url, strlen(url));
    char *start = NULL;
    char *p_slash = NULL;

    #if CONFIG_HTTP_SECURE
    const char *protocol = "https";
    #else
    const char *protocol = "http";
    #endif

    if (strncmp(url_dup, protocol, strlen(protocol)) == 0)
    {
        start = url_dup + strlen(protocol) + 3;
        p_slash = strchr(start, '/');
        if (p_slash != NULL)
        {
            memcpy(uri, p_slash, strlen(p_slash));
            *p_slash = '\0';
        }
        else
        {
            memcpy(uri, '/', 1);
        }
    }
}

static int http_request_and_recv(char *url, int method, char *hdr, uint32_t timeout, char *http_buffer)
{
    int ret = -1;
    char uri[HTTP_HEADER_SIZE] = {0};
    http_rsp_info_t rsp_info;
    
    parse_url(url, uri);
    ret = httpc_send_request(httpc_handle, method, uri, hdr, NULL, NULL, 0);
    if (ret != HTTP_SUCCESS)
    {
        amp_warn(MOD_STR, "http request fail");
        return ret;
    }

    ret = httpc_recv_response(httpc_handle, rsp_buf, HTTP_BUFF_SIZE, &rsp_info, timeout);
    if (ret < 0)
    {
        amp_warn(MOD_STR, "http response fail");
        return ret;
    }

    memcpy(http_buffer, rsp_info.body_start, rsp_info.rsp_len);
    ret = 0;

    return ret;
}


static void http_gethost_info(char *src, char **web, char **file, int *port)
{
    char *pa;
    char *pb;
    int isHttps = 0;

    if (!src || strlen(src) == 0) {
        amp_warn(MOD_STR, "http_gethost_info parms error!");
        return;
    }

    amp_warn(MOD_STR, "src = %s %d", src, strlen(src));

    *port = 0;
    if (!(*src)) {
        return;
    }

    pa = src;
    if (!strncmp(pa, "https://", strlen("https://"))) {
        pa      = src + strlen("https://");
        isHttps = 1;
    }

    if (!isHttps) {
        if (!strncmp(pa, "http://", strlen("http://"))) {
            pa = src + strlen("http://");
        }
    }

    *web = pa;
    pb   = strchr(pa, '/');
    if (pb) {
        *pb = 0;
        pb += 1;
        if (*pb) {
            *file                   = pb;
            *((*file) + strlen(pb)) = 0;
        }
    } else {
        (*web)[strlen(pa)] = 0;
    }

    pa = strchr(*web, ':');
    if (pa) {
        *pa   = 0;
        *port = atoi(pa + 1);
    } else {
        /* TODO: support https:443
        if (isHttps) {
            *port = 80;
        } else {
            *port = 80;
        } */
        *port = 80;
    }
}

#define HTTP_HEADER                      \
    "GET /%s HTTP/1.1\r\nAccept:*/*\r\n" \
    "User-Agent: Mozilla/5.0\r\n"        \
    "Cache-Control: no-cache\r\n"        \
    "Connection: close\r\n"              \
    "Host:%s:%d\r\n\r\n"

static int http_request_and_save2(char *url, int method, char *hdr, uint32_t timeout, char *filepath)
{
    int ret             = 0;
    int sockfd          = 0;
    int port            = 0;
    int nbytes          = 0;
    int send            = 0;
    int totalsend       = 0;
    uint32_t breakpoint = 0;
    int size            = 0;
    int header_found    = 0;
    char *pos           = 0;
    int file_size       = 0;
    char *host_file     = NULL;
    char *host_addr     = NULL;
    void *fd;
    char *ptr;
    uint8_t *pdata;
    int count = 0;
    int content_len = 0;
    int data_len;
    int download_len = 0;

    fd = amp_fopen(filepath, "w+");
    if (!fd) {
        amp_error(MOD_STR, "open %s fail", filepath);
        return -1;
    }

    http_gethost_info(url, &host_addr, &host_file, &port);

    sockfd = ota_socket_connect(port, host_addr);

    memset(rsp_buf, 0, HTTP_BUFF_SIZE);
    sprintf(rsp_buf, HTTP_HEADER, host_file, host_addr, port);

    send      = 0;
    totalsend = 0;
    nbytes    = strlen(rsp_buf);

    while (totalsend < nbytes) {
        send = ota_socket_send(sockfd, rsp_buf + totalsend,
                               nbytes - totalsend);
        if (send == -1) {
            amp_warn(MOD_STR, "send error!%s", strerror(errno));
            amp_fclose(fd);
            ota_socket_close(sockfd);
            return -2;
        }
        totalsend += send;
    }

    while (1) {
        if (count++ == 0) {
            ret = ota_socket_recv(sockfd, rsp_buf, HTTP_BUFF_SIZE);
            ptr = strncasestr(rsp_buf, "Content-length: ");
            if (ptr) {
                ptr += strlen("Content-length: ");
                content_len = atoi(ptr);
            }
            ptr = strstr(rsp_buf, "\r\n\r\n");
            if (ptr) {
                ptr += strlen("\r\n\r\n");
                data_len = ret - (ptr - (char *)rsp_buf);
                pdata = ptr;
            } else {
                data_len = ret;
                pdata = rsp_buf;
            }
        } else {
            if (content_len - download_len >= HTTP_BUFF_SIZE)
                ret = ota_socket_recv(sockfd, rsp_buf, HTTP_BUFF_SIZE);
            else
                ret = ota_socket_recv(sockfd, rsp_buf, content_len - download_len);
            data_len = ret;
            pdata = rsp_buf;
        }
        if (ret < 0)
        {
            amp_warn(MOD_STR, "http response fail");
            amp_fclose(fd);
            ota_socket_close(sockfd);
            return -2;
        }

        amp_info(MOD_STR, "write %s %d", filepath, data_len);
        if (data_len > 0) {
            ret = amp_fwrite(pdata, 1, data_len, fd);
            if (ret < 0) {
                amp_error(MOD_STR, "write %s fail", filepath);
                amp_fclose(fd);
                amp_remove(filepath);
                ota_socket_close(sockfd);
                return -1;
            }
            download_len += data_len;
        }

        if (download_len >= content_len) {
            amp_info(MOD_STR, "content recv done, total %d", download_len);
            break;
        }
    }

    amp_fsync(fd);
    amp_fclose(fd);
    ota_socket_close(sockfd);
    return 0;
}


static void http_recv_notify(void *pdata)
{
    http_param_t *msg = (http_param_t *)pdata;
    duk_context *ctx = be_get_context();
    be_push_ref(ctx, msg->js_cb_ref);
    duk_push_string(ctx, msg->buffer);
    if (duk_pcall(ctx, 1) != DUK_EXEC_SUCCESS) {
        amp_console("%s", duk_safe_to_stacktrace(ctx, -1));
    }
    duk_pop(ctx);
    be_unref(ctx, msg->js_cb_ref);
    amp_free(msg->buffer);
    amp_free(msg);

    duk_gc(ctx, 0);
}

static void http_recv_and_save_notify(void *pdata)
{
    http_download_resp_t *resp = (http_download_resp_t *)pdata;
    duk_context *ctx = be_get_context();
    be_push_ref(ctx, resp->js_cb_ref);
    duk_push_int(ctx, resp->error);
    if (duk_pcall(ctx, 1) != DUK_EXEC_SUCCESS) {
        amp_console("%s", duk_safe_to_stacktrace(ctx, -1));
    }
    duk_pop(ctx);
    be_unref(ctx, resp->js_cb_ref);
    amp_free(resp);

    duk_gc(ctx, 0);
}

/* create task for http request */
static void *task_http_request_fun(void *arg)
{
    int ret = -1;
    int socketid = -1;
    int i;
    char *url = NULL;
    uint32_t timeout = 0;
    int http_method = 0;
    char *http_buf = NULL;
    char hdr[HTTP_HEADER_SIZE] = {0};
    httpc_connection_t settings;
    http_param_t *msg = (http_param_t *)arg;

    url = msg->url;
    timeout = msg->timeout;
    http_buf = msg->buffer;
    
    amp_msleep(50); /* need do things after state changed in main task */

    http_method = msg->method;

    /* inti http client */
    http_client_initialize();
    /* create socket */
    socketid = httpc_wrapper_socket_create();

    settings.socket = socketid;
    settings.server_name = url;
    settings.req_buf = req_buf;
    settings.req_buf_size = HTTP_BUFF_SIZE;

    httpc_handle = httpc_init(&settings);

    // if (httpc_handle == 0)
    // {
    //     amp_warn(MOD_STR, "http session init fail");
    //     return ret;
    // }

    for (i = 0; i < http_header_index; i++) {
        ret = httpc_construct_header(hdr, HTTP_HEADER_SIZE, msg->http_header[i].name, msg->http_header[i].data);
        if (ret < 0) {
            amp_warn(MOD_STR, "http construct header fail");
            goto out;
        }
    }
    http_header_index = 0;
    /* Blocking send & recv */
    ret = http_request_and_recv(url, http_method, hdr, timeout, http_buf);
    if (ret != 0)
    {
        memset(http_buf, 0, HTTP_BUFF_SIZE);
        sprintf(http_buf, "%s", "ERROR");
    }

    httpc_wrapper_socket_close(socketid);

    httpc_deinit(httpc_handle);
    
    httpc_handle = 0;

    amp_task_schedule_call(http_recv_notify, msg);
    return NULL;

out:
    httpc_wrapper_socket_close(socketid);
    httpc_deinit(httpc_handle);
    httpc_handle = 0;
    if (msg) {
        amp_free(msg->buffer);
        amp_free(msg);
    }

    amp_thread_delete(NULL);

    return NULL;
}

/* create task for http download */
static int task_http_download_fun(void *arg)
{
    int ret = -1;
    int socketid = -1;
    int i;
    char *url = NULL;
    char *ptr;
    uint32_t timeout = 0;
    int http_method = 0;
    char hdr[HTTP_HEADER_SIZE] = {0};
    httpc_connection_t settings;
    http_download_param_t *msg = (http_download_param_t *)arg;
    void *fd;

    url = msg->url;
    timeout = msg->timeout;

    amp_msleep(50); /* need do things after state changed in main task */

    http_method = msg->method;

    /* inti http client */
    http_client_initialize();
    /* create socket */
    socketid = httpc_wrapper_socket_create();

    memset(&settings, 0, sizeof(settings));
    settings.socket = socketid;
    settings.server_name = url;
    settings.req_buf = req_buf;
    settings.req_buf_size = HTTP_BUFF_SIZE;

    httpc_handle = httpc_init(&settings);

    for (i = 0; i < msg->header_index; i++) {
        ret = httpc_construct_header(hdr, HTTP_HEADER_SIZE, msg->http_header[i].name, msg->http_header[i].data);
        if (ret < 0) {
            amp_warn(MOD_STR, "http construct header fail");
            goto out;
        }
    }
    msg->header_index = 0;
    /* Blocking send & recv */
    ret = http_request_and_save2(url, http_method, hdr, timeout, msg->path);
    if (ret != 0)
    {
        if (ret == -2)
            msg->error = 2;
        else
            msg->error = 1;
    }
    else
    {
        msg->error = 0;
    }

    httpc_wrapper_socket_close(socketid);

    httpc_deinit(httpc_handle);

    httpc_handle = 0;

    if (msg->error) {
        //amp_task_schedule_call(http_recv_and_save_notify, msg);
        return -1;
    }
    return 0;

out:
    httpc_wrapper_socket_close(socketid);
    httpc_deinit(httpc_handle);
    httpc_handle = 0;
    return -1;
}

static duk_ret_t native_http_request(duk_context *ctx)
{
    http_param_t *http_param = NULL;
    char *http_buffer = NULL;
    const char *method = NULL;
    int http_method = 0;
    int timeout = 0;
    char localip[32];
    int i;
    void *http_task;
    int http_task_stack_used;
    amp_os_thread_param_t http_task_params = {0};

    if (!duk_is_object(ctx, 0))
    {
        amp_warn(MOD_STR, "invalid parameter\n");
        goto done;
    }

    if (amp_get_ip(localip) != 0)
    {
        amp_warn(MOD_STR, "network not ready\r\n");
        goto done;
    }

    http_param = (http_param_t *)amp_malloc(sizeof(http_param_t));
    if (!http_param)
    {
        amp_warn(MOD_STR, "allocate memory failed\n");
        goto done;
    }

    http_buffer = amp_malloc(HTTP_BUFF_SIZE + 1);
    if (!http_buffer)
    {
        amp_warn(MOD_STR, "allocate memory failed\n");
        goto done;
    }
    memset(http_buffer, 0, HTTP_BUFF_SIZE + 1);
    http_param->buffer = http_buffer;

    /* get http request url */
    duk_get_prop_string(ctx, 0, "url");
    if (!duk_is_string(ctx, -1)) {
        amp_debug(MOD_STR, "request url is invalid");
        goto done;
    }
    http_param->url = duk_get_string(ctx, 1);
    duk_pop(ctx);

    /* get http request filepath */
    if (duk_get_prop_string(ctx, 0, "filepath")) {
        if (!duk_is_string(ctx, -1)) {
            amp_debug(MOD_STR, "request url is invalid");
            goto done;
        }
        http_param->filepath = duk_get_string(ctx, 1);
        duk_pop(ctx);
    } else {
        http_param->filepath = NULL;
    }

    /* get http request method */
    if (duk_get_prop_string(ctx, 0, "method")) {
        if (duk_is_string(ctx, -1)) {
            method = duk_get_string(ctx, -1);
            if(strcmp(method, "GET") == 0) {
                http_method = HTTP_GET; /* GET */
            }
            else if(strcmp(method, "POST") == 0) {
                http_method = HTTP_POST; /* POST */
            }
            else if(strcmp(method, "PUT") == 0) {
                http_method = HTTP_PUT; /* PUT */
            }
            else {
                http_method = HTTP_GET;
            }

            http_param->method = http_method;
        }
    } else {
        http_param->method = HTTP_GET;
    }
    duk_pop(ctx);

    /* get http request timeout */
    if (duk_get_prop_string(ctx, 0, "timeout")) {
        if (duk_is_number(ctx, -1)) {
            timeout = duk_get_number(ctx, -1);
            http_param->timeout = timeout;
        } else {
            http_param->timeout = HTTP_REQUEST_TIMEOUT;
        }
    } else {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }

    if (http_param->timeout <= 0) {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }
    duk_pop(ctx);

    /* get http request headers */
    if (duk_get_prop_string(ctx, 0, "headers")) {
        duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1))
        {
            // amp_debug(MOD_STR, "key=%s, value=%s ", duk_to_string(ctx, -2), duk_to_string(ctx, -1));
            if (!duk_is_string(ctx, -2) || !duk_is_string(ctx, -1)) {
                amp_debug(MOD_STR, "get header failed, index is: %d", http_header_index);
                break;
            }
            http_param->http_header[http_header_index].name = duk_to_string(ctx, -2);
            http_param->http_header[http_header_index].data = duk_to_string(ctx, -1);
            http_header_index++;
            duk_pop_2(ctx);
        }
    } else {
        http_param->http_header[0].name = HTTP_DEFAULT_HEADER_NAME;
        http_param->http_header[0].data = HTTP_DEFAULT_HEADER_DATA;
        http_header_index++;
    }

    amp_debug(MOD_STR, "url: %s", http_param->url);
    amp_debug(MOD_STR, "method: %d", http_param->method);
    amp_debug(MOD_STR, "timeout: %d", http_param->timeout);
    for (i = 0; i < http_header_index; i++) {
        amp_debug(MOD_STR, "headers: %s:%s", http_param->http_header[i].name, http_param->http_header[i].data);
    }

    /* callback */
    if (duk_get_prop_string(ctx, 0, "success")) {
        duk_dup(ctx, -1);
        http_param->js_cb_ref = be_ref(ctx);
    } else {
        http_param->js_cb_ref = -1;
    }


    http_task_params.name = "amp http task";
    http_task_params.priority = ADDON_TSK_PRIORRITY;
    http_task_params.stack_size = 1024 * 4;
    amp_thread_create(&http_task, task_http_request_fun, http_param, &http_task_params, &http_task_stack_used);

    duk_push_int(ctx, 0);
    return 1;

done:
    if (http_buffer)
        amp_free(http_buffer);
    if (http_param)
        amp_free(http_param);

    duk_push_int(ctx, -1);
    return 1;
}

static duk_ret_t native_http_download(duk_context *ctx)
{
    http_download_param_t *http_param = NULL;
    const char *method = NULL;
    char *filepath = NULL;
    int http_method = 0;
    int timeout = 0;
    char localip[32];
    int i;
    void *http_task;
    int http_task_stack_used;
    amp_os_thread_param_t http_task_params = {0};

    if (!duk_is_object(ctx, 0))
    {
        amp_warn(MOD_STR, "invalid parameter\n");
        goto done;
    }

    if (amp_get_ip(localip) != 0)
    {
        amp_warn(MOD_STR, "network not ready\r\n");
        goto done;
    }

    http_param = (http_param_t *)amp_malloc(sizeof(http_download_param_t));
    if (!http_param)
    {
        amp_warn(MOD_STR, "allocate memory failed\n");
        goto done;
    }

    /* get http request url */
    duk_get_prop_string(ctx, 0, "url");
    if (!duk_is_string(ctx, -1)) {
        amp_debug(MOD_STR, "request url is invalid");
        goto done;
    }
    http_param->url = duk_get_string(ctx, 1);
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, 0, "filepath")) {
        if (duk_is_string(ctx, -1)) {
            filepath = duk_get_string(ctx, -1);
            memset(http_param->path, 0, sizeof(http_param->path));
            #if 1
            amp_get_user_dir(http_param->path);
            snprintf(http_param->path + strlen(http_param->path),
                sizeof(http_param->path) - strlen(http_param->path), "%s", filepath);
            #else
            snprintf(http_param->path, sizeof(http_param->path), "%s", filepath);
            #endif
            amp_debug(MOD_STR, "filepath is %s", http_param->path);
        }
    } else {
        amp_error(MOD_STR, "file path not exist");
        goto done;
    }

    /* get http request method */
    if (duk_get_prop_string(ctx, 0, "method")) {
        if (duk_is_string(ctx, -1)) {
            method = duk_get_string(ctx, -1);
            if(strcmp(method, "GET") == 0) {
                http_method = HTTP_GET; /* GET */
            }
            else if(strcmp(method, "POST") == 0) {
                http_method = HTTP_POST; /* POST */
            }
            else if(strcmp(method, "PUT") == 0) {
                http_method = HTTP_PUT; /* PUT */
            }
            else {
                http_method = HTTP_GET;
            }

            http_param->method = http_method;
        }
    } else {
        http_param->method = HTTP_GET;
    }
    duk_pop(ctx);

    /* get http request timeout */
    if (duk_get_prop_string(ctx, 0, "timeout")) {
        if (duk_is_number(ctx, -1)) {
            timeout = duk_get_number(ctx, -1);
            http_param->timeout = timeout;
        } else {
            http_param->timeout = HTTP_REQUEST_TIMEOUT;
        }
    } else {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }

    if (http_param->timeout <= 0) {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }
    duk_pop(ctx);

    /* get http request headers */
    if (duk_get_prop_string(ctx, 0, "headers")) {
        duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1))
        {
            // amp_debug(MOD_STR, "key=%s, value=%s ", duk_to_string(ctx, -2), duk_to_string(ctx, -1));
            if (!duk_is_string(ctx, -2) || !duk_is_string(ctx, -1)) {
                amp_debug(MOD_STR, "get header failed, index is: %d", http_header_index);
                break;
            }
            http_param->http_header[http_header_index].name = duk_to_string(ctx, -2);
            http_param->http_header[http_header_index].data = duk_to_string(ctx, -1);
            http_header_index++;
            duk_pop_2(ctx);
        }
    } else {
        http_param->http_header[0].name = HTTP_DEFAULT_HEADER_NAME;
        http_param->http_header[0].data = HTTP_DEFAULT_HEADER_DATA;
        http_header_index++;
    }

    amp_debug(MOD_STR, "url: %s", http_param->url);
    amp_debug(MOD_STR, "method: %d", http_param->method);
    amp_debug(MOD_STR, "timeout: %d", http_param->timeout);
    for (i = 0; i < http_header_index; i++) {
        amp_debug(MOD_STR, "headers: %s:%s", http_param->http_header[i].name, http_param->http_header[i].data);
    }

    /* callback */
    if (duk_get_prop_string(ctx, 0, "success")) {
        duk_dup(ctx, -1);
        http_param->js_cb_ref = be_ref(ctx);
        duk_pop(ctx);
    } else {
        http_param->js_cb_ref = -1;
    }

    http_task_params.name = "amp http task";
    http_task_params.priority = ADDON_TSK_PRIORRITY;
    http_task_params.stack_size = 1024 * 10;
    amp_thread_create(&http_task, task_http_download_fun, http_param, &http_task_params, &http_task_stack_used);

    duk_push_int(ctx, 0);
    return 1;

done:
    if (http_param)
        amp_free(http_param);

    duk_push_int(ctx, -1);
    return 1;
}

static void *task_http_download_loop(void *arg)
{
    http_download_resp_t *resp = (http_download_resp_t *)arg;
    http_dload_node_t *dload_node;
    dlist_t *temp;
    int ret;
    resp->error = 0;

    dlist_for_each_entry_safe(&g_dload_list, temp, dload_node, http_dload_node_t, node)
    {
        ret = task_http_download_fun(dload_node->dload_param);
        if (ret < 0) {
            resp->error = dload_node->dload_param->error;
            break;
        }
        dlist_del(&dload_node->node);
        amp_free(dload_node->dload_param);
        amp_free(dload_node);
    }

    if (ret < 0) {
        dlist_for_each_entry_safe(&g_dload_list, temp, dload_node, http_dload_node_t, node)
        {
            dlist_del(&dload_node->node);
            amp_free(dload_node->dload_param);
            amp_free(dload_node);
        }
    }

    dload_list_count = 0;
    amp_task_schedule_call(http_recv_and_save_notify, resp);
    amp_thread_delete(NULL);
    return NULL;
}

static duk_ret_t native_http_download_start(duk_context *ctx)
{
    void *http_task;
    int http_task_stack_used;
    amp_os_thread_param_t http_task_params = {0};
    http_download_resp_t *resp;

    resp = amp_malloc(sizeof(http_download_resp_t));
    if (!resp) {
        amp_error(MOD_STR, "alloc download resp fail");
        duk_push_int(ctx, 1);
        return -1;
    }

    /* callback */
    if (duk_get_prop_string(ctx, 0, "success")) {
        duk_dup(ctx, -1);
        resp->js_cb_ref = be_ref(ctx);
        duk_pop(ctx);
    } else {
        resp->js_cb_ref = -1;
    }

    http_task_params.name = "amp http task";
    http_task_params.priority = amp_get_default_task_priority();
    http_task_params.stack_size = 1024 * 10;
    amp_thread_create(&http_task, task_http_download_loop, resp, &http_task_params, &http_task_stack_used);

    duk_push_int(ctx, 0);
    return 1;
}

static duk_ret_t native_http_download_add(duk_context *ctx)
{
    http_download_param_t *http_param = NULL;
    const char *method = NULL;
    char *filepath = NULL;
    int http_method = 0;
    int timeout = 0;
    char localip[32];
    int i;
    void *http_task;
    int http_task_stack_used;
    amp_os_thread_param_t http_task_params = {0};

    if (!duk_is_object(ctx, 0))
    {
        amp_warn(MOD_STR, "invalid parameter\n");
        goto done;
    }

    if (amp_get_ip(localip) != 0)
    {
        amp_warn(MOD_STR, "network not ready\r\n");
        goto done;
    }

    http_param = (http_download_param_t *)amp_malloc(sizeof(http_download_param_t));
    if (!http_param)
    {
        amp_warn(MOD_STR, "allocate memory failed\n");
        goto done;
    }
    memset(http_param, 0, sizeof(http_download_param_t));

    /* get http request url */
    duk_get_prop_string(ctx, 0, "url");
    if (!duk_is_string(ctx, -1)) {
        amp_debug(MOD_STR, "request url is invalid");
        goto done;
    }
    http_param->url = duk_get_string(ctx, 1);
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, 0, "filepath")) {
        if (duk_is_string(ctx, -1)) {
            filepath = duk_get_string(ctx, -1);
            memset(http_param->path, 0, sizeof(http_param->path));
            #if 1
            amp_get_user_dir(http_param->path);
            snprintf(http_param->path + strlen(http_param->path),
                sizeof(http_param->path) - strlen(http_param->path), "%s", filepath);
            #else
            snprintf(http_param->path, sizeof(http_param->path), "%s", filepath);
            #endif
            amp_debug(MOD_STR, "filepath is %s", http_param->path);
        }
    } else {
        amp_error(MOD_STR, "file path not exist");
        goto done;
    }

    /* get http request method */
    if (duk_get_prop_string(ctx, 0, "method")) {
        if (duk_is_string(ctx, -1)) {
            method = duk_get_string(ctx, -1);
            if(strcmp(method, "GET") == 0) {
                http_method = HTTP_GET; /* GET */
            }
            else if(strcmp(method, "POST") == 0) {
                http_method = HTTP_POST; /* POST */
            }
            else if(strcmp(method, "PUT") == 0) {
                http_method = HTTP_PUT; /* PUT */
            }
            else {
                http_method = HTTP_GET;
            }

            http_param->method = http_method;
        }
    } else {
        http_param->method = HTTP_GET;
    }
    duk_pop(ctx);

    /* get http request timeout */
    if (duk_get_prop_string(ctx, 0, "timeout")) {
        if (duk_is_number(ctx, -1)) {
            timeout = duk_get_number(ctx, -1);
            http_param->timeout = timeout;
        } else {
            http_param->timeout = HTTP_REQUEST_TIMEOUT;
        }
    } else {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }

    if (http_param->timeout <= 0) {
        http_param->timeout = HTTP_REQUEST_TIMEOUT;
    }
    duk_pop(ctx);

    /* get http request headers */
    if (duk_get_prop_string(ctx, 0, "headers")) {
        duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1))
        {
            // amp_debug(MOD_STR, "key=%s, value=%s ", duk_to_string(ctx, -2), duk_to_string(ctx, -1));
            if (!duk_is_string(ctx, -2) || !duk_is_string(ctx, -1)) {
                amp_debug(MOD_STR, "get header failed, index is: %d", http_param->header_index);
                break;
            }
            http_param->http_header[http_param->header_index].name = duk_to_string(ctx, -2);
            http_param->http_header[http_param->header_index].data = duk_to_string(ctx, -1);
            http_param->header_index++;
            duk_pop_2(ctx);
        }
    } else {
        http_param->http_header[0].name = HTTP_DEFAULT_HEADER_NAME;
        http_param->http_header[0].data = HTTP_DEFAULT_HEADER_DATA;
        http_param->header_index++;
    }

    amp_debug(MOD_STR, "add download url: %s", http_param->url);
    for (i = 0; i < http_param->header_index; i++) {
        amp_debug(MOD_STR, "headers: %s:%s", http_param->http_header[i].name, http_param->http_header[i].data);
    }

    http_dload_node_t *dload_node = amp_malloc(sizeof(http_dload_node_t));
    memset(dload_node, 0, sizeof(http_dload_node_t));
    dload_node->dload_param = http_param;
    dlist_add_tail(&dload_node->node, &g_dload_list);
    http_param->index = ++dload_list_count;

    duk_push_int(ctx, 0);
    return 1;

done:
    if (http_param)
        amp_free(http_param);

    duk_push_int(ctx, -1);
    return 1;
}

void module_http_register(void)
{
    duk_context *ctx = be_get_context();
    duk_push_object(ctx);

    /* request */
    AMP_ADD_FUNCTION("request", native_http_request, 1);
    AMP_ADD_FUNCTION("download", native_http_download, 1);
    AMP_ADD_FUNCTION("startDownload", native_http_download_start, 1);
    AMP_ADD_FUNCTION("addDownload", native_http_download_add, 1);

    duk_put_prop_string(ctx, -2, "HTTP");
}
