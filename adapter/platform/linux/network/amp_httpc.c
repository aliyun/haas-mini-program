/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_network.h"
#include "amp_platform.h"

struct hostent *amp_httpc_get_host_by_name(const char *name)
{
    return gethostbyname(name);
}

int32_t amp_httpc_socket_connect(uintptr_t fd, const struct sockaddr *name, socklen_t namelen)
{
    return connect(fd, name, namelen);
}