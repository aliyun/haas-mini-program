/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "infra_compat.h"

#define NETWORK_ADDR_LEN    (16)

intptr_t amp_udp_socket_create()
{
    int                     rc = -1;
    long                    socket_id = -1;

    socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_id < 0) {
        printf("create socket error");
        return rc;
    }

    return socket_id;
}

intptr_t amp_udp_socket_bind(intptr_t p_socket, unsigned short port)
{
    struct sockaddr_in addr;
    int opt_val = 1;

    memset(&addr, 0, sizeof(struct sockaddr_in));

    if (0 != setsockopt(p_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val))) {
        printf("setsockopt(SO_REUSEADDR) falied\n");
        close(p_socket);
        return -1;
    }

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (-1 == bind(p_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
        printf("bind(%d) falied\n", (int)p_socket);
        close(p_socket);
        return -1;
    }
    printf("success to establish udp, fd = %d", (int)p_socket);

    return (intptr_t)p_socket;
}

intptr_t amp_udp_create(char *host, unsigned short port)
{


    int                     rc = -1;
    long                    socket_id = -1;
    char                    port_ptr[6] = {0};
    struct addrinfo         hints;
    char                    addr[NETWORK_ADDR_LEN] = {0};
    struct addrinfo        *res, *ainfo;
    struct sockaddr_in     *sa = NULL;

    if (NULL == host) {
        return (-1);
    }

    sprintf(port_ptr, "%u", port);
    memset((char *)&hints, 0x00, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_UDP;

    rc = getaddrinfo(host, port_ptr, &hints, &res);
    if (0 != rc) {
        printf("getaddrinfo error");
        return (-1);
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
        if (AF_INET == ainfo->ai_family) {
            sa = (struct sockaddr_in *)ainfo->ai_addr;
            inet_ntop(AF_INET, &sa->sin_addr, addr, NETWORK_ADDR_LEN);
            fprintf(stderr, "The host IP %s, port is %d\r\n", addr, ntohs(sa->sin_port));

            socket_id = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
            if (socket_id < 0) {
                printf("create socket error");
                continue;
            }
            if (0 == connect(socket_id, ainfo->ai_addr, ainfo->ai_addrlen)) {
                break;
            }

            close(socket_id);
        }
    }
    freeaddrinfo(res);

    return socket_id;

}

int amp_udp_write(intptr_t p_socket,
                  const unsigned char *p_data,
                  unsigned int datalen)
{
    int             rc = -1;
    long            socket_id = -1;

    socket_id = (long)p_socket;
    rc = send(socket_id, (char *)p_data, (int)datalen, 0);
    if (-1 == rc) {
        return -1;
    }

    return rc;
}

int amp_udp_read_timeout(intptr_t p_socket,
                        unsigned char *p_data,
                        unsigned int datalen,
                        unsigned int timeout)
{
    int                 ret;
    struct timeval      tv;
    fd_set              read_fds;
    long                socket_id = -1;

    if (0 == p_socket || NULL == p_data) {
        return -1;
    }
    socket_id = (long)p_socket;

    if (socket_id < 0) {
        return -1;
    }

    FD_ZERO(&read_fds);
    FD_SET(socket_id, &read_fds);

    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ret = select(socket_id + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);

    /* Zero fds ready means we timed out */
    if (ret == 0) {
        return -2;    /* receive timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want read */
        }

        return -4; /* receive failed */
    }

    /* This call will not block */
    return read(p_socket, p_data, datalen);
}

intptr_t amp_udp_create_without_connect(const char *host, unsigned short port)
{
    struct sockaddr_in addr;
    long sockfd;
    int opt_val = 1;
    struct hostent *hp;
    struct in_addr in;
    uint32_t ip;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("socket");
        return -1;
    }
    if (0 == port) {
        return (intptr_t)sockfd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));

    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val))) {
        printf("setsockopt(SO_REUSEADDR) falied\n");
        close(sockfd);
        return -1;
    }

    if (NULL == host) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_aton(host, &in)) {
            ip = *(uint32_t *)&in;
        } else {
            hp = gethostbyname(host);
            if (!hp) {
                printf("can't resolve the host address - '%s'\n", host);
                close(sockfd);
                return -1;
            }
            ip = *(uint32_t *)(hp->h_addr);
        }
        addr.sin_addr.s_addr = ip;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (-1 == bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
        printf("bind(%d) falied\n", (int)sockfd);
        close(sockfd);
        return -1;
    }
    printf("success to establish udp, fd = %d", (int)sockfd);

    return (intptr_t)sockfd;
}


int amp_udp_close_without_connect(intptr_t sockfd)
{
    return close((int)sockfd);
}

int amp_udp_joinmulticast(intptr_t sockfd,
                          char *p_group)
{
    int err = -1;
    int socket_id = -1;
    int loop = 0;
    struct ip_mreq mreq;

    if (NULL == p_group) {
        return -1;
    }

    /*set loopback*/
    socket_id = (int)sockfd;
    err = setsockopt(socket_id, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    if (err < 0) {
        printf("setsockopt");
        return err;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(p_group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); /*default networt interface*/

    /*join to the multicast group*/
    err = setsockopt(socket_id, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0) {
        printf("setsockopt");
        return err;
    }

    return 0;
}

int amp_udp_recvfrom(intptr_t sockfd,
                     NetworkAddr *p_remote,
                     unsigned char *p_data,
                     unsigned int datalen,
                     unsigned int timeout_ms)
{
    int ret;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    fd_set read_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    ret = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* receive timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want read */
        }
        return -4; /* receive failed */
    }

    ret = recvfrom(sockfd, p_data, datalen, 0, (struct sockaddr *)&addr, &addr_len);
    if (ret > 0) {
        if (NULL != p_remote) {
            p_remote->port = ntohs(addr.sin_port);

            strcpy((char *)p_remote->addr, inet_ntoa(addr.sin_addr));
        }

        return ret;
    }

    return -1;
}

int amp_udp_sendto(intptr_t sockfd,
                   const NetworkAddr *p_remote,
                   const unsigned char *p_data,
                   unsigned int datalen,
                   unsigned int timeout_ms)
{
    int ret;
    uint32_t ip;
    struct in_addr in;
    struct hostent *hp;
    struct sockaddr_in addr;
    fd_set write_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (inet_aton((char *)p_remote->addr, &in)) {
        ip = *(uint32_t *)&in;
    } else {
        hp = gethostbyname((char *)p_remote->addr);
        if (!hp) {
            printf("can't resolute the host address \n");
            return -1;
        }
        ip = *(uint32_t *)(hp->h_addr);
    }

    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    ret = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
    if (ret == 0) {
        return 0;    /* write timeout */
    }

    if (ret < 0) {
        if (errno == EINTR) {
            return -3;    /* want write */
        }
        return -4; /* write failed */
    }

    addr.sin_addr.s_addr = ip;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(p_remote->port);

    ret = sendto(sockfd, p_data, datalen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    if (ret < 0) {
        printf("sendto");
    }

    return (ret) > 0 ? ret : -1;
}

#define ROUTER_INFO_PATH        "/proc/net/route"
#define ROUTER_RECORD_SIZE      256

char *_get_default_routing_ifname(char *ifname, int ifname_size)
{
    FILE *fp = NULL;
    char line[ROUTER_RECORD_SIZE] = {0};
    char iface[IFNAMSIZ] = {0};
    char *result = NULL;
    unsigned int destination, gateway, flags, mask;
    unsigned int refCnt, use, metric, mtu, window, irtt;
    char *buff = NULL;

    fp = fopen(ROUTER_INFO_PATH, "r");
    if (fp == NULL) {
        perror("fopen");
        return result;
    }

    buff = fgets(line, sizeof(line), fp);
    if (buff == NULL) {
        perror("fgets");
        goto out;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (11 !=
            sscanf(line, "%s %08x %08x %x %d %d %d %08x %d %d %d",
                   iface, &destination, &gateway, &flags, &refCnt, &use,
                   &metric, &mask, &mtu, &window, &irtt)) {
            perror("sscanf");
            continue;
        }

        /*default route */
        if ((destination == 0) && (mask == 0)) {
            strncpy(ifname, iface, ifname_size - 1);
            result = ifname;
            break;
        }
    }

out:
    if (fp) {
        fclose(fp);
    }

    return result;
}

uint32_t HAL_Wifi_Get_IP(char ip_str[NETWORK_ADDR_LEN], const char *ifname)
{
    struct ifreq ifreq;
    int sock = -1;
    char ifname_buff[IFNAMSIZ] = {0};

    if ((NULL == ifname || strlen(ifname) == 0) &&
        NULL == (ifname = _get_default_routing_ifname(ifname_buff, sizeof(ifname_buff)))) {
        perror("get default routeing ifname");
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    ifreq.ifr_addr.sa_family = AF_INET;
    strncpy(ifreq.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFADDR, &ifreq) < 0) {
        close(sock);
        perror("ioctl");
        return -1;
    }

    close(sock);

    strncpy(ip_str,
            inet_ntoa(((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr),
            NETWORK_ADDR_LEN);

    return ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
}




