/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "infra_config.h"

#ifdef INFRA_HTTPC

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "infra_state.h"
#include "infra_compat.h"
#include "infra_types.h"
#include "infra_defs.h"
#include "infra_httpc.h"
#include "infra_net.h"
#include "infra_timer.h"
#include "wrappers.h"

#ifdef INFRA_LOG
    #include "infra_log.h"
    #define httpc_err(...)   log_err("httpc", __VA_ARGS__)
    #define httpc_info(...)  log_info("httpc", __VA_ARGS__)
    #define httpc_debug(...) log_debug("httpc", __VA_ARGS__)
#else
    #define httpc_err(...)
    #define httpc_info(...)
    #define httpc_debug(...)
#endif

#define HTTPCLIENT_MIN(x,y) (((x)<(y))?(x):(y))
#define HTTPCLIENT_MAX(x,y) (((x)>(y))?(x):(y))


#define HTTPCLIENT_READ_BUF_SIZE     (1024)          /* read payload */
#define HTTPCLIENT_RAED_HEAD_SIZE (32)            /* read header */
#define HTTPCLIENT_SEND_BUF_SIZE  (1024)          /* send */

#define HTTPCLIENT_MAX_URL_LEN   (256)

#define HTTP_RETRIEVE_MORE_DATA   (1)            /**< More data needs to be retrieved. */

#if defined(MBEDTLS_DEBUG_C)
    #define DEBUG_LEVEL 2
#endif
#define HTTPCLIENT_CHUNK_SIZE (1024)

static int _utils_parse_url(const char *url, char *host, char *path);
static int _http_recv(httpclient_t *client, char *buf, int max_len, int *p_read_len,
                      uint32_t timeout);
static int _http_get_response_body(httpclient_t *client, char *data, int len, uint32_t timeout,
                                   httpclient_data_t *client_data);
static int _http_parse_response_header(httpclient_t *client, char *data, int len, uint32_t timeout,
                                       httpclient_data_t *client_data);

static int _utils_parse_url(const char *url, char *host,
                            char *path)
{
    char *host_ptr = (char *) strstr(url, "://");
    uint32_t host_len = 0;
    uint32_t path_len;
    /* char *port_ptr; */
    char *path_ptr;
    char *fragment_ptr;

    if (host_ptr == NULL) {
        return STATE_USER_INPUT_HTTP_URL; /* URL is invalid */
    }
    host_ptr += 3;

    path_ptr = strchr(host_ptr, '/');
    if (NULL == path_ptr) {
        return STATE_USER_INPUT_HTTP_PATH;
    }

    if (host_len == 0) {
        host_len = path_ptr - host_ptr;
    }

    memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';
    fragment_ptr = strchr(host_ptr, '#');
    if (fragment_ptr != NULL) {
        path_len = fragment_ptr - path_ptr;
    } else {
        path_len = strlen(path_ptr);
    }

    memcpy(path, path_ptr, path_len);
    path[path_len] = '\0';

    return STATE_SUCCESS;
}

static int _utils_fill_tx_buffer(httpclient_t *client, char *send_buf, int *send_idx, char *buf,
                                 uint32_t len) /* 0 on success, err code on failure */
{
    int ret;
    int cp_len;
    int idx = *send_idx;

    if (len == 0) {
        len = strlen(buf);
    }
    do {
        if ((HTTPCLIENT_SEND_BUF_SIZE - idx) >= len) {
            cp_len = len;
        } else {
            cp_len = HTTPCLIENT_SEND_BUF_SIZE - idx;
        }

        memcpy(send_buf + idx, buf, cp_len);
        idx += cp_len;
        len -= cp_len;

        if (idx == HTTPCLIENT_SEND_BUF_SIZE) {
            ret = client->net.write(&client->net, send_buf, HTTPCLIENT_SEND_BUF_SIZE, 5000);
            if (ret) {
                return (ret);
            }
        }
    } while (len);

    *send_idx = idx;
    return STATE_SUCCESS;
}

static int _http_send_header(httpclient_t *client, const char *host, const char *path, int method,
                             httpclient_data_t *client_data)
{
    int len;
    char send_buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    char buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    char *meth = (method == HTTPCLIENT_GET) ? "GET" : (method == HTTPCLIENT_POST) ? "POST" :
                 (method == HTTPCLIENT_PUT) ? "PUT" : (method == HTTPCLIENT_DELETE) ? "DELETE" :
                 (method == HTTPCLIENT_HEAD) ? "HEAD" : "";
    int ret;

    /* Send request */
    memset(send_buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
    len = 0; /* Reset send buffer */

    amp_snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); /* Write request */

    ret = _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));
    if (ret) {
        /* httpc_err("Could not write request"); */
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }

    /* Add user header information */
    if (client->header) {
        _utils_fill_tx_buffer(client, send_buf, &len, (char *) client->header, strlen(client->header));
    }

    if (client_data->post_buf != NULL) {
        amp_snprintf(buf, sizeof(buf), "Content-Length: %d\r\n", client_data->post_buf_len);
        _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));

        if (client_data->post_content_type != NULL) {
            amp_snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", client_data->post_content_type);
            _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));
        }
    }

    /* Close headers */
    _utils_fill_tx_buffer(client, send_buf, &len, "\r\n", 0);

#ifdef INFRA_LOG
    log_multi_line(LOG_DEBUG_LEVEL, "REQUEST", "%s", send_buf, ">");
#endif

    /* ret = httpclient_tcp_send_all(client->net.handle, send_buf, len); */
    ret = client->net.write(&client->net, send_buf, len, 5000);
    if (ret <= 0) {
        httpc_err("ret = client->net.write() = %d", ret);
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }

    return STATE_SUCCESS;
}

int _http_send_userdata(httpclient_t *client, httpclient_data_t *client_data)
{
    int ret = 0;

    if (client_data->post_buf && client_data->post_buf_len) {
        /* ret = httpclient_tcp_send_all(client->handle, (char *)client_data->post_buf, client_data->post_buf_len); */
        ret = client->net.write(&client->net, (char *)client_data->post_buf, client_data->post_buf_len, 5000);
        httpc_debug("client_data->post_buf: %s, ret is %d", client_data->post_buf, ret);
        if (ret <= 0) {
            return STATE_SYS_DEPEND_NWK_CLOSE; /* Connection was closed by server */
        }
    }

    return STATE_SUCCESS;
}

/* 0 on success, err code on failure */
static int _http_recv(httpclient_t *client, char *buf, int max_len, int *p_read_len,
                      uint32_t timeout_ms)
{
    int ret = 0;
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    *p_read_len = 0;

    ret = client->net.read(&client->net, buf, max_len, iotx_time_left(&timer));
    /* httpc_debug("Recv: | %s", buf); */
    httpc_info("ret of _http_recv is %d", ret);

    if (ret > 0) {
        *p_read_len = ret;
        return STATE_SUCCESS;
    } else if (ret == 0) {
        /* timeout */
        return STATE_SYS_DEPEND_NWK_TIMEOUT;
    } else {
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }
}

#define MIN_TIMEOUT (100)
#define MAX_RETRY_COUNT (600)


static int _utils_check_deadloop(int len, iotx_time_t *timer, int ret, unsigned int *dead_loop_count,
                                 unsigned int *extend_count)
{
    /* if timeout reduce to zero, it will be translated into NULL for select function in TLS lib */
    /* it would lead to indenfinite behavior, so we avoid it */
    if (iotx_time_left(timer) < MIN_TIMEOUT) {
        (*extend_count)++;
        utils_time_countdown_ms(timer, MIN_TIMEOUT);
    }

    /* if it falls into deadloop before reconnected to internet, we just quit*/
    if ((0 == len) && (0 == iotx_time_left(timer)) && (STATE_SUCCESS > ret)) {
        (*dead_loop_count)++;
        if (*dead_loop_count > MAX_RETRY_COUNT) {
            httpc_err("deadloop detected, exit");
            return STATE_SYS_DEPEND_NWK_CLOSE;
        }
    } else {
        *dead_loop_count = 0;
    }

    /*if the internet connection is fixed during the loop, the download stream might be disconnected. we have to quit */
    if ((0 == len) && (*extend_count > 2 * MAX_RETRY_COUNT) && (STATE_SUCCESS > ret)) {
        httpc_err("extend timer for too many times, exit");
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }
    return STATE_SUCCESS;
}

static int _utils_fill_rx_buf(int *recv_count, int len_to_write_to_respons_buf, httpclient_data_t *client_data,
                              char *data)
{
    int count = *recv_count;
    if (count + len_to_write_to_respons_buf < client_data->response_buf_len - 1) {
        memcpy(client_data->response_buf + count, data, len_to_write_to_respons_buf);
        count += len_to_write_to_respons_buf;
        client_data->response_buf[count] = '\0';
        client_data->retrieve_len -= len_to_write_to_respons_buf;
        *recv_count = count;
        return STATE_SUCCESS;
    } else {
        memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
        client_data->response_buf[client_data->response_buf_len - 1] = '\0';
        client_data->retrieve_len -= (client_data->response_buf_len - 1 - count);
        return HTTP_RETRIEVE_MORE_DATA;
    }
}

static int _http_get_response_body(httpclient_t *client, char *data, int data_len_actually_received,
                                   uint32_t timeout_ms, httpclient_data_t *client_data)
{
    int written_response_buf_len = 0;
    int len_to_write_to_respons_buf = 0;
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    /* Receive data */
    /* httpc_debug("Current data: %s", data); */

    client_data->is_more = IOT_TRUE;

    /* the header is not received finished */
    if (client_data->response_content_len == -1 && client_data->is_chunked == IOT_FALSE) {
        /* can not enter this if */
        /* TODO check the way to go into this branch */
        httpc_err("header is not received yet");
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }

    while (1) {
        unsigned int dead_loop_count = 0;
        unsigned int extend_count = 0;
        do {
            int res;
            /* move previous fetched data into response_buf */
            len_to_write_to_respons_buf = HTTPCLIENT_MIN(data_len_actually_received, client_data->retrieve_len);
            res = _utils_fill_rx_buf(&written_response_buf_len, len_to_write_to_respons_buf, client_data, data);
            if (HTTP_RETRIEVE_MORE_DATA == res) {
                return HTTP_RETRIEVE_MORE_DATA;
            }

            /* get data from internet and put into "data" buf temporary */
            if (client_data->retrieve_len) {
                int ret;
                int max_len_to_receive = HTTPCLIENT_MIN(HTTPCLIENT_CHUNK_SIZE - 1,
                                                        client_data->response_buf_len - 1 - written_response_buf_len);
                max_len_to_receive = HTTPCLIENT_MIN(max_len_to_receive, client_data->retrieve_len);

                ret = _http_recv(client, data, max_len_to_receive, &data_len_actually_received, iotx_time_left(&timer));
                if (ret != STATE_SUCCESS) {
                    return ret;
                }
                httpc_debug("Total- remaind Payload: %d Bytes; currently Read: %d Bytes", client_data->retrieve_len,
                            data_len_actually_received);

                /* TODO  add deadloop processing*/
                ret = _utils_check_deadloop(data_len_actually_received, &timer, ret, &dead_loop_count,
                                            &extend_count);
                if (ret != STATE_SUCCESS) {
                    return ret;
                }
            }
        } while (client_data->retrieve_len);
        client_data->is_more = IOT_FALSE;
        break;
    }

    return STATE_SUCCESS;
}

static int _http_parse_response_header(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
                                       httpclient_data_t *client_data)
{
    int crlf_pos;
    iotx_time_t timer;
    char *tmp_ptr, *ptr_body_end;
    int new_trf_len, ret;
    char *crlf_ptr;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    client_data->response_content_len = -1;

    /* http client response */
    /* <status-line> HTTP/1.1 200 OK(CRLF)

       <headers> ...(CRLF)

       <blank line> (CRLF)

      [<response-body>] */
    crlf_ptr = strstr(data, "\r\n");
    if (crlf_ptr == NULL) {
        httpc_err("\r\n not found");
        return ERROR_HTTP_UNRESOLVED_DNS;
    }

    crlf_pos = crlf_ptr - data;
    data[crlf_pos] = '\0';
    client->response_code = atoi(data + 9);
    httpc_debug("Reading headers: %s", data);
    memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
    len -= (crlf_pos + 2);       /* remove status_line length */
    client_data->is_chunked = IOT_FALSE;

    /*If not ending of response body*/
    /* try to read more header again until find response head ending "\r\n\r\n" */
    while (NULL == (ptr_body_end = strstr(data, "\r\n\r\n"))) {
        /* try to read more header */
        int max_remain_len = HTTPCLIENT_READ_BUF_SIZE - len -1;
        if (max_remain_len <= 0) {
            httpc_debug("buffer exceeded max\n");
            return ERROR_HTTP_PARSE;
        }
        max_remain_len = max_remain_len > HTTPCLIENT_RAED_HEAD_SIZE ? HTTPCLIENT_RAED_HEAD_SIZE : max_remain_len;

        ret = _http_recv(client, data + len, max_remain_len, &new_trf_len, iotx_time_left(&timer));
        if (ret != STATE_SUCCESS) {
            return ret;
        }
        len += new_trf_len;
        data[len] = '\0';
    }

    /* parse response_content_len */
    if (NULL != (tmp_ptr = strstr(data, "Content-Length"))) {
        client_data->response_content_len = atoi(tmp_ptr + strlen("Content-Length: "));
        client_data->retrieve_len = client_data->response_content_len;
    } else {
        return STATE_HTTP_RESP_MISSING_CONTENT_LEN;
    }

    /* remove header length */
    /* len is Had read body's length */
    /* if client_data->response_content_len != 0, it is know response length */
    /* the remain length is client_data->response_content_len - len */
    len = len - (ptr_body_end + 4 - data);
    memmove(data, ptr_body_end + 4, len + 1);
    client_data->response_received_len += len;
    return _http_get_response_body(client, data, len, iotx_time_left(&timer), client_data);
}

int httpclient_connect(httpclient_t *client)
{
    int retry_max = 3;
    int retry_cnt = 1;
    int retry_interval = 1000;
    int rc = -1;

    do {
        client->net.handle = 0;
        httpc_debug("calling TCP or TLS connect HAL for [%d/%d] iteration", retry_cnt, retry_max);

        rc = client->net.connect(&client->net);
        if (0 != rc) {
            client->net.disconnect(&client->net);
            httpc_err("TCP or TLS connect failed, rc = %d", rc);
            amp_msleep(retry_interval);
            continue;
        } else {
            httpc_debug("rc = client->net.connect() = %d, success @ [%d/%d] iteration", rc, retry_cnt, retry_max);
            break;
        }
    } while (++retry_cnt <= retry_max);

    return STATE_SUCCESS;
}

int _http_send_request(httpclient_t *client, const char *host, const char *path, HTTPCLIENT_REQUEST_TYPE method,
                       httpclient_data_t *client_data)
{
    int ret = STATE_SUCCESS;

    if (0 == client->net.handle) {
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }

    ret = _http_send_header(client, host, path, method, client_data);
    if (ret != 0) {
        return ret;
    }

    if (method == HTTPCLIENT_POST || method == HTTPCLIENT_PUT) {
        ret = _http_send_userdata(client, client_data);
        if (ret < 0) {
            ret = ret;
        }
    }

    return ret;
}

int httpclient_recv_response(httpclient_t *client, uint32_t timeout_ms, httpclient_data_t *client_data)
{
    int reclen = 0, ret = STATE_SUCCESS;
    char buf[HTTPCLIENT_READ_BUF_SIZE] = { 0 };
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    if (0 == client->net.handle) {
        httpc_err("not connection have been established");
        return STATE_SYS_DEPEND_NWK_CLOSE;
    }

    if (client_data->is_more) {
        client_data->response_buf[0] = '\0';
        ret = _http_get_response_body(client, buf, reclen, iotx_time_left(&timer), client_data);
    } else {
        client_data->is_more = 1;
        /* try to read header */
        ret = _http_recv(client, buf, HTTPCLIENT_RAED_HEAD_SIZE, &reclen, iotx_time_left(&timer));
        if (ret != STATE_SUCCESS) {
            return ret;
        }

        buf[reclen] = '\0';

        if (reclen) {
#ifdef INFRA_LOG
            log_multi_line(LOG_DEBUG_LEVEL, "RESPONSE", "%s", buf, "<");
#endif
            ret = _http_parse_response_header(client, buf, reclen, iotx_time_left(&timer), client_data);
        }
    }

    return ret;
}

void httpclient_close(httpclient_t *client)
{
    if (client->net.handle > 0) {
        client->net.disconnect(&client->net);
    }
    client->net.handle = 0;
    httpc_info("client disconnected");
}

static int _http_send(httpclient_t *client, const char *url, int port, const char *ca_crt,
                      HTTPCLIENT_REQUEST_TYPE method, httpclient_data_t *client_data)
{
    int ret;
    char host[HTTPCLIENT_MAX_URL_LEN] = { 0 };
    char path[HTTPCLIENT_MAX_URL_LEN] = { 0 };

    /* First we need to parse the url (http[s]://host[:port][/[path]]) */
    ret = _utils_parse_url(url, host, path);
    if (ret != STATE_SUCCESS) {
        return ret;
    }

    if (0 == client->net.handle) {
        /* Establish connection if no. */
        ret = iotx_net_init_internal(&client->net, host, port, ca_crt);
        if (0 != ret) {
            return ret;
        }

        ret = httpclient_connect(client);
        if (0 != ret) {
            httpclient_close(client);
            return ret;
        }

        ret = _http_send_request(client, host, path, method, client_data);
        if (ret != STATE_SUCCESS) {
            httpc_err("_http_send_request is error, ret = %d", ret);
            return ret;
        }
    }
    return STATE_SUCCESS;
}

int httpclient_common(httpclient_t *client, const char *url, int port, const char *ca_crt,
                      HTTPCLIENT_REQUEST_TYPE method, uint32_t timeout_ms, httpclient_data_t *client_data)
{
    iotx_time_t timer;
    int ret = _http_send(client, url, port, ca_crt, method, client_data);
    if (STATE_SUCCESS != ret) {
        httpclient_close(client);
        return ret;
    }

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    if ((NULL != client_data->response_buf)
        && (0 != client_data->response_buf_len)) {
        ret = httpclient_recv_response(client, iotx_time_left(&timer), client_data);
        if (ret < STATE_SUCCESS) {
            httpc_err("httpclient_recv_response is error,ret = %d", ret);
            httpclient_close(client);
            return ret;
        }
    }

    if (! client_data->is_more) {
        /* Close the HTTP if no more data. */
        httpc_info("close http channel");
        httpclient_close(client);
    }

    ret = 0;
    return ret;
}

int iotx_post(httpclient_t *client,
              const char *url,
              int port,
              const char *ca_crt,
              httpclient_data_t *client_data)
{
    return _http_send(client, url, port, ca_crt, HTTPCLIENT_POST, client_data);
}

/**********************************************************************************/

typedef struct {
    httpclient_t client;
    httpclient_data_t http_client_data;
    int receive_maxlen;
    int already_received;
    char *url;
    int port;
    iotx_http_method_t method;
    char *header;
    char *cert;
    int timeout;
    recvcallback recv_cb;
    void *recv_ctx;
} wrapper_http_handle_t;

static int _replase_str(char **ptr, void *data)
{
    if (*ptr != NULL) {
        amp_free(*ptr);
    }
    *ptr = amp_malloc(strlen(data) + 1);
    if (*ptr == NULL) {
        return STATE_SYS_DEPEND_MALLOC;
    }
    memset(*ptr, 0, strlen(data) + 1);
    memcpy(*ptr, data, strlen(data));

    return STATE_SUCCESS;
}

void *wrapper_http_init(void)
{
    wrapper_http_handle_t *handle = amp_malloc(sizeof(wrapper_http_handle_t));
    if (handle == NULL) {
        iotx_state_event(ITE_STATE_USER_INPUT, STATE_SYS_DEPEND_MALLOC, "malloc failed");
        return NULL;
    }
    memset(handle, 0, sizeof(wrapper_http_handle_t));

    return handle;
}

int wrapper_http_setopt(void *handle, iotx_http_option_t option, void *data)
{
    wrapper_http_handle_t *http_handle = (wrapper_http_handle_t *)handle;

    if (handle == NULL || data == NULL) {
        return STATE_USER_INPUT_INVALID;
    }

    switch (option) {
        case IOTX_HTTPOPT_URL: {
            _replase_str(&http_handle->url, data);
        }
        break;
        case IOTX_HTTPOPT_PORT: {
            if (*(int *)(data) < 0) {
                return STATE_USER_INPUT_HTTP_PORT;
            }
            http_handle->port = *(int *)(data);
        }
        break;
        case IOTX_HTTPOPT_METHOD: {
            http_handle->method = *(iotx_http_method_t *)data;
        }
        break;
        case IOTX_HTTPOPT_HEADER: {
            _replase_str(&http_handle->header, data);
        }
        break;
        case IOTX_HTTPOPT_CERT: {
            http_handle->cert = data;
        }
        break;
        case IOTX_HTTPOPT_TIMEOUT: {
            if (*(int *)(data) < 0) {
                return STATE_USER_INPUT_HTTP_TIMEOUT;
            }
            http_handle->timeout = *(int *)(data);
        }
        break;
        case IOTX_HTTPOPT_RECVCALLBACK: {
            http_handle->recv_cb = data;
        }
        break;
        case IOTX_HTTPOPT_RECVMAXLEN: {
            http_handle->receive_maxlen = *(int *)(data);
        }
        break;
        case IOTX_HTTPOPT_RECVCONTEXT: {
            http_handle->recv_ctx = (recvcallback)data;
        }
        break;
        default: {
            httpc_err("Unknown Option");
            return STATE_USER_INPUT_HTTP_OPTION;
        }
    }

    return STATE_SUCCESS;
}

int wrapper_http_perform(void *handle, void *data, int length)
{
    int res = 0, received_this_time = 0;
    iotx_time_t timer;
    wrapper_http_handle_t *http_handle = (wrapper_http_handle_t *)handle;
    char *response_payload = NULL;

    if (handle == NULL) {
        return STATE_USER_INPUT_INVALID;
    }

    if ((http_handle->method == IOTX_HTTP_POST) &&
        (data == NULL || length <= 0)) {
        return STATE_USER_INPUT_HTTP_POST_DATA;
    }

    if (http_handle->receive_maxlen <= 0) {
        return STATE_USER_INPUT_INVALID;
    }

    response_payload = amp_malloc(http_handle->receive_maxlen);
    if (response_payload == NULL) {
        return STATE_SYS_DEPEND_MALLOC;
    }
    memset(response_payload, 0, http_handle->receive_maxlen);

    http_handle->client.header = http_handle->header;
    http_handle->http_client_data.post_buf = data;
    http_handle->http_client_data.post_buf_len = length;
    http_handle->http_client_data.response_buf = response_payload;
    http_handle->http_client_data.response_buf_len = http_handle->receive_maxlen;


    res = _http_send(&http_handle->client, http_handle->url, http_handle->port,
                     http_handle->cert, (HTTPCLIENT_REQUEST_TYPE)http_handle->method, &http_handle->http_client_data);
    if (res < STATE_SUCCESS) {
        amp_free(response_payload);
        return res;
    }

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, http_handle->timeout);

    res = httpclient_recv_response(&http_handle->client, iotx_time_left(&timer), &http_handle->http_client_data);
    if (res < STATE_SUCCESS) {
        httpc_err("httpclient_recv_response is error,res = %d", res);
        amp_free(response_payload);
        return res;
    }

    received_this_time = http_handle->http_client_data.response_content_len - http_handle->http_client_data.retrieve_len -
                         http_handle->already_received;
    http_handle->already_received = http_handle->http_client_data.response_content_len -
                                    http_handle->http_client_data.retrieve_len;

    if (res >= STATE_SUCCESS) {
        if (http_handle->recv_cb) {
            http_handle->recv_cb(response_payload, received_this_time, http_handle->http_client_data.response_content_len,
                                 http_handle->recv_ctx);
        }
    }

    amp_free(response_payload);

    return received_this_time;
}

void wrapper_http_deinit(void **handle)
{
    wrapper_http_handle_t **http_handle = (wrapper_http_handle_t **)handle;

    if (handle == NULL || *handle == NULL) {
        return;
    }

    if ((*http_handle)->url) {
        amp_free((*http_handle)->url);
    }

    if ((*http_handle)->header) {
        amp_free((*http_handle)->header);
    }

    httpclient_close(&(*http_handle)->client);

    amp_free(*http_handle);
    *handle = NULL;
}

/**********************************************************************************/

#endif

