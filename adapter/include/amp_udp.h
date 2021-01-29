/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef _AMP_UDP_H_
#define _AMP_UDP_H_

#define NETWORK_ADDR_LEN (16)

typedef struct _network_addr_t {
    unsigned char
    addr[NETWORK_ADDR_LEN];
    unsigned short  port;
} amp_networkAddr;

/**
 * This function will create a udp socket.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_socket_create();

/**
 * This function will bind a port for udp socket.
 *
 * @param[in]  p_socket  udp socket.
 * @param[in]  port      port.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_socket_bind(int p_socket, unsigned short port);

/**
 * This function will establish a udp connect.
 *
 * @param[in]  host  host name of udp socket connect
 * @param[in]  port  port.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_create(char *host, unsigned short port);

/**
 * This function will send data on udp socket.
 *
 * @param[in]  p_socket  udp socket.
 * @param[in]  p_data    data.
 * @param[in]  datalen   data length.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_write(int p_socket,
                  const unsigned char *p_data,
                  unsigned int datalen);

/**
 * This function will read data from a udp socket.
 *
 * @param[in]   p_socket  udp socket.
 * @param[out]  p_data    data buffer.
 * @param[in]   datalen   data length.
 * @param[in]   timeout   timeout.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_read_timeout(int p_socket,
                        unsigned char *p_data,
                        unsigned int datalen,
                        unsigned int timeout);

/**
 * This function will create a udp socket without connect.
 *
 * @param[in]  host  host name of udp socket to create.
 * @param[in]  port  port.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_create_without_connect(const char *host, unsigned short port);

/**
 * This function will close a udp socket without connect.
 *
 * @param[in]  sockfd  udp socket.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_close_without_connect(int sockfd);

/**
 * This function will establish a tcp socket.
 *
 * @param[in]  host  host name of tcp socket connect
 * @param[in]  port  port.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_joinmulticast(int sockfd,
                          char *p_group);

/**
 * This function will receive data from a udp socket.
 *
 * @param[in]   sockfd       udp socket.
 * @param[in]   p_remote     remote info.
 * @param[out]  p_data       data.
 * @param[in]   datalen      data length.
 * @param[in]   timeout_ms   timeout.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_recvfrom(int sockfd,
                     amp_networkAddr *p_remote,
                     unsigned char *p_data,
                     unsigned int datalen,
                     unsigned int timeout_ms);

/**
 * This function will send data from a udp socket.
 * 
 * @param[in]   sockfd       udp socket.
 * @param[in]   p_remote     remote info.
 * @param[in]   p_data       data.
 * @param[in]   datalen      data length.
 * @param[in]   timeout_ms   timeout.
 *
 * @return  0: success, otherwise: fail.
 */
int amp_udp_sendto(int sockfd,
                   const amp_networkAddr *p_remote,
                   const unsigned char *p_data,
                   unsigned int datalen,
                   unsigned int timeout_ms);

#endif /* _AMP_UDP_H_ */