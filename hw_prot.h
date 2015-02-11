/* 
 * File:   hw_prot.h
 * Author: muzzy
 *
 * Created on May 18, 2009, 11:19 AM
 */

#ifndef _HW_PROT_H
#define	_HW_PROT_H

#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"

#define HW_MAX_BUFF_SIZE 1024

#define HW_SOCKET_IO_TIMEOUT 10000

#define HW_STATE_IS_READY       0x0001
#define HW_STATE_IS_DONE        0x0002
#define HW_STATE_IS_WAIT        0x0004
#define HW_STATE_IS_WORK        0x0008
#define HW_STATE_IS_PRN         0x0010
#define HW_STATE_IS_SOFTBLOCK   0x0020
#define HW_STATE_CLEAN      0x0000

typedef struct cmd_lst_s {
    struct cmd_lst_s *next;
    char *str;
    time_t ctime;
} cmd_lst_t;

typedef struct opt_lst_s {
    struct opt_lst_s *next;
    char opt_id;
    char *opt_name;
    char **val;
    int is_ro;
} opt_lst_t;

typedef struct hw_socket_s {
    int sock;

    char *device;
    struct termios keys;
    struct termios old_keys;

    uint32_t timeout;
    time_t lastop;

    uint8_t buf[HW_MAX_BUFF_SIZE];
    uint16_t buf_len;
    char str_error[MAX_STRING_LEN];

    char *software;

    cmd_lst_t *cmd_lst;
//    opt_lst_t *opt_lst;

    uint32_t state;
} hw_socket_t;

extern hw_socket_t *hw_fd;
extern char *pr_device;
extern uint8_t hw_use_crc;

int setIoctlParam(hw_socket_t *in_soc, int param, unsigned short level);
int getIoctlParam(hw_socket_t *in_soc, int param);
unsigned int getBaudRateConst(speed_t baudrate);
int hw_sock_new(hw_socket_t **in_soc);
int hw_init_device(hw_socket_t *in_soc, speed_t baud_rate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits, uint8_t full_modem, uint8_t vmin, uint8_t vtime);
int hw_setup_port(hw_socket_t *in_soc, int flags, speed_t baud_rate);
int hw_close_device(hw_socket_t *in_soc);
int hw_send_buf(hw_socket_t *in_soc);
int hw_detect_device(char *arg1, char *arg2);

// ==== hw_maria.c
#define HW_MARIA301_DEFAULT_BAUDRATE B115200

int hw_maria301_connect(hw_socket_t *in_soc);
int hw_maria301_disconnect(hw_socket_t *in_soc);
int hw_maria301_recv_buf(hw_socket_t *in_soc);
int hw_maria301_send_buf(hw_socket_t *in_soc);
int hw_maria301_recv_cmd();
int hw_maria301_send_cmd(char *in_cmd);
//int hw_maria301_check(char *device);
int hw_maria301_check(char *device, uint8_t reset);

// ==== hw_datecs.c

#define HW_DATECS_BYTE_NAK 0x15
#define HW_DATECS_BYTE_SYN 0x16
#define HW_DATECS_RESP_TIMEOUT 600
#define HW_DATECS_DEFAULT_BAUDRATE B115200

int hw_datecs_connect(hw_socket_t *in_soc, speed_t baudrate);
int hw_datecs_disconnect(hw_socket_t *in_soc);
int hw_datecs_recv_buf(hw_socket_t *in_soc);
int hw_datecs_send_buf(hw_socket_t *in_soc);
int hw_datecs_recv_cmd();
int hw_datecs_send_cmd(char *in_cmd);
int hw_datecs_check(char *device, uint8_t full_chk);
int net_datecs_send_cmd(char *in_cmd);

extern uint8_t cmd_seq;

// ==== hw_prot_privat.c
int prot_privat_check(char *device);

// ==== hw_prot_fuib.c
int prot_fuib_check(char *device);

#endif	/* _HW_PROT_H */
