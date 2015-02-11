/* 
 * File:   net_prot.h
 * Author: muzzy
 *
 * Created on May 15, 2009, 5:23 PM
 */

#ifndef _NET_PROT_H
#define	_NET_PROT_H

#include "net_connect.h"

#define NET_PROTO_T 0
#define NET_PROTO_M 1
#define NET_PROTO_V2 2
#define NET_PROTO_D 3

#define NET_PRT_SEND_CMD 0x01
#define NET_PRT_RECV_OK 0x02

int net_wait_session(char *ip, char *port);
int net_send_cmd(char *in_cmd);
int net_v2_send_cmd(uint8_t in_cmd, char *cmd_str);

#endif	/* _NET_PROT_H */
