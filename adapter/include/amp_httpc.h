/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef _AMP_HTTPC_H_
#define _AMP_HTTPC_H_

#include "amp_platform.h"

/**
 * @brief amp_httpc_get_host_by_name() get host by name.
 *
 * @param[in] name   the name of the host.
 *
 * @return  On success, return struct of hostent.
 *          On error
 */
struct hostent *amp_httpc_get_host_by_name(const char *name);

/**
 * @brief amp_httpc_socket_connect() http socket connect.
 *
 * @param[in] fd        the file descriptor.
 * @param[in] name      the name of sock addr.
 * @param[in] namelen   name length.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_httpc_socket_connect(uintptr_t fd, const struct sockaddr *name, socklen_t namelen);

#endif /* _AMP_HTTPC_H_ */

