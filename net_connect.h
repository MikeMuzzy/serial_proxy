/* 
 * File:   network.h
 * Author: muzzy
 *
 * Created on May 15, 2009, 3:48 PM
 */

#ifndef _NETWORK_H
#define	_NETWORK_H

#include <stdlib.h>
#include <stdint.h>
#include "util.h"

#include <time.h>
#include <errno.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#include "hw_prot.h"

#define NET_MAX_BUFF_SIZE 1024

#define NET_HDR_SIZE 4
#define NET_MAX_SIZE NET_MAX_BUFF_SIZE

#define NET_PROT_NUM 0x02

typedef struct net_socket_s {
    int out_sock;       // stdout
    int inp_sock;        // stdin
    int rm_sock;      // netinout

    cmd_lst_t *cmd_lst;

    int timeout;
    time_t lastop;

    uint32_t state;

    uint8_t proto;

    uint8_t buf[NET_MAX_BUFF_SIZE]; // Cmd buffer
    uint16_t buf_len;
    char str_error[MAX_STRING_LEN];

    char *sock_host; // IP address of local side
    char *peer_host; // IP address of remote side

    char *ip_addr;
    char *ip_port;
} net_socket_t;

extern net_socket_t *net_fd;

extern char *net_rem_host;
extern char *net_rem_port;

int net_sock_new(net_socket_t **in_soc, char *ip, char *port);
int net_make_connect(net_socket_t *in_soc);
int net_get_peer(net_socket_t *in_soc);
int net_recv_buf(net_socket_t *in_soc);
int net_send_buf(net_socket_t *in_soc);

int net_v2_recv_buf(net_socket_t *in_soc);
int net_v2_send_buf(net_socket_t *in_soc);

#endif	/* _NETWORK_H */

