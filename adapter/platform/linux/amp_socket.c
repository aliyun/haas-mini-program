/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"
#include "amp_socket.h"

int amp_socket_errno(void)
{
	return errno;
}

int amp_socket_open(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int amp_socket_send(int sockfd, const void *data, size_t size, int flags)
{
	return send(sockfd, data, size, flags);
}

int amp_socket_recv(int sockfd, void *mem, size_t len, int flags)
{
    return recv(sockfd, mem, len, flags);
}

int amp_socket_write(int sockfd, const void *data, size_t size)
{
	return write(sockfd, data, size);
}

int amp_socket_read(int sockfd, void *data, size_t len)
{
	return read(sockfd, data, len);
}

int amp_socket_sendto(int sockfd, const void *data, size_t size, int flags,
	const struct sockaddr *to, socklen_t tolen)
{
	return sendto(sockfd, data, size, flags, to, tolen);
}

int amp_socket_recvfrom(int sockfd, void *data, size_t len, int flags,
	struct sockaddr *from, socklen_t *fromlen)
{
	return recvfrom(sockfd, data, len, flags, from, fromlen);
}

int amp_socket_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	return setsockopt(sockfd, level, optname, optval, optlen);
}

int amp_socket_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
	return getsockopt(sockfd, level, optname, optval, optlen);
}

int amp_socket_connect(int sockfd, const struct sockaddr *name, socklen_t namelen)
{
	return connect(sockfd, name, namelen);
}

int amp_socket_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	return bind(sockfd, addr, addrlen);
}

int amp_socket_listen(int sockfd, int backlog)
{
	return listen(sockfd, backlog);
}

int amp_socket_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept(sockfd, addr, addrlen);
}

int amp_socket_select(int maxfdp1, fd_set *readset, fd_set *writeset,
	fd_set *exceptset, struct timeval *timeout)
{
	return select(maxfdp1, readset, writeset, exceptset, timeout);
}

int amp_socket_close(int sockfd)
{
	return close(sockfd);
}

int amp_socket_shutdown(int sockfd, int how)
{
	return shutdown(sockfd, how);
}