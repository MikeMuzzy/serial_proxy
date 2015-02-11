#include "hw_prot.h"
#include "net_prot.h"
#include "main.h"

uint8_t cmd_seq = 0x20;

#ifndef SOH
#define SOH 0x01
#define ENQ 0x05
#endif

char *hw_datecs_count_bcc(uint8_t *cmd_str) {
    uint8_t *c  = cmd_str;
    char ret[4];
    int csum = 0;
    if (SOH != *c++) return(0);
    while(ENQ != *c) csum += *c++;
    csum += *c++;
    ret[3] = csum%16+0x30;
    csum = csum/16;
    ret[2] = csum%16+0x30;
    csum = csum/16;
    ret[1] = csum%16+0x30;
    csum = csum/16;
    ret[0] = csum%16+0x30;
    csum = csum/16;
    return(xstrcpy(ret));
}

int hw_datecs_connect(hw_socket_t *in_soc, speed_t baudrate) {
    int rc=0;
    char strtmp[MAX_STRING_LEN];

    add_log(__func__, LOG_DEBUG,"open datecs port=\"%s\"", in_soc->device);
    in_soc->timeout = HW_DATECS_RESP_TIMEOUT;
    cmd_seq = 0x20;
    if (!in_soc->sock) {
	if ((rc=hw_init_device(in_soc, baudrate, '8', 'N', '1', 0, 3, 7))) return(rc);
    }
    sprintf(strtmp,"%c000000;",0x28);
    if ((rc=hw_datecs_send_cmd(strtmp))) return(rc);
    if ((rc=hw_datecs_recv_cmd())) return(rc);
    add_log(__func__, LOG_DEBUG,"datecs port opened");
    return(0);
}

int hw_datecs_disconnect(hw_socket_t *in_soc) {
    return(hw_close_device(&(*in_soc)));
}

int hw_datecs_recv_buf(hw_socket_t *in_soc) {
    struct pollfd ufd;
    int res=0;
    int err_no=0;
    uint16_t red_len=0;
    uint16_t in_len=1;

    if (!in_soc->sock) return(41);

    memset(in_soc->buf, 0, HW_MAX_BUFF_SIZE);
    in_soc->buf_len=0;

    while(red_len<in_len) {
        ufd.fd=in_soc->sock;
        ufd.events=POLLIN;
        if((res=poll(&ufd, 1, in_soc->timeout)) < 0) {
            err_no=errno;
            sprintf(in_soc->str_error, "Read Poll[%d]: %s", err_no, strerror(err_no));
            return(46);
        }
        if (!res) {
            err_no=errno;
            sprintf(in_soc->str_error, "HW: Read Poll Timeout: %d ms.", in_soc->timeout);
            return(47);
        }

        if((res=read(in_soc->sock, in_soc->buf+red_len, in_len-red_len)) == -1) {
            err_no=errno;
            sprintf(in_soc->str_error, "HW: Read[%d]: %s", err_no, strerror(err_no));
            return(42);
        }

	if (ufd.revents & (POLLERR | POLLHUP)) {
            err_no=errno;
            sprintf(in_soc->str_error, "HW: device has been disconnected");
            return(47);
	}

        red_len+=res;
        if (red_len == 1) {
            if (in_soc->buf[0] == 0x01) {
                in_len = 2;
            } else if (in_soc->buf[0] == HW_DATECS_BYTE_NAK) {
                return(43); /*** cmd error, need retry ***/
            } else if (in_soc->buf[0] == HW_DATECS_BYTE_SYN) {
                red_len = 0;
                continue;
            }
        } else if(red_len == 2) {
            in_len = (in_soc->buf[1] - 0x20) + 6; /*** + 1(SOH) + 4(BCC) + 1(ETX) ***/
            in_soc->buf_len=in_len;
            if(in_len < sizeof(in_len)) return(44);
            if(in_len > HW_MAX_BUFF_SIZE) return(45);
        }
    }
    in_soc->lastop=time(NULL);
    add_log(__func__, LOG_DEBUG,"got red_len=%d, in_len=%d",red_len,in_len);
    return(0);
}

int hw_datecs_recv_cmd() {
    int x=0;
    int rc=0;
    char strtmp[MAX_STRING_LEN];

    if ((rc=hw_datecs_recv_buf(hw_fd))) return(rc);
    if (hw_fd->buf[0] != 0x01) return(48);
    sprintf(strtmp,"hex:");
    for(x = 0; x < hw_fd->buf_len;x++) {
	sprintf(strtmp, "%s [%02x]",strtmp, hw_fd->buf[x]);
    }
    add_log(__func__, LOG_DEBUG,"r< (%d) \"%s\"",hw_fd->buf_len, strtmp);
    return(0);
}

int hw_datecs_send_buf(hw_socket_t *in_soc) {
    int x=0;
    char strtmp[MAX_STRING_LEN];
    sprintf(strtmp, "hex:");
    for(x = 0; x < in_soc->buf_len;x++) {
	sprintf(strtmp, "%s [%02x]",strtmp, in_soc->buf[x]);
    }
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",in_soc->buf_len, strtmp);
    return(hw_send_buf(&(*in_soc)));
}

int hw_datecs_send_cmd(char *in_cmd) {
    int x=0;
    char strtmp[MAX_STRING_LEN];

    if(!in_cmd || !in_cmd[0]) return(52);

    for(x=0;x<strlen(in_cmd);x++)
        if ( (in_cmd[x] == '\r') || (in_cmd[x] == '\n') ) in_cmd[x]='\0';

    hw_fd->buf_len=0;
    memset(hw_fd->buf, 0, NET_MAX_BUFF_SIZE);
    memset(strtmp, 0, MAX_STRING_LEN);
    hw_fd->buf[hw_fd->buf_len++] = 0x01;
    hw_fd->buf[hw_fd->buf_len++] = 0x20 + strlen(in_cmd) + 3;
    if (cmd_seq >= 0x7F || cmd_seq < 0x20) cmd_seq = 0x20;
    hw_fd->buf[hw_fd->buf_len++] = ++cmd_seq;
    hw_fd->buf_len+= sprintf((char*)(hw_fd->buf+3), "%s", in_cmd);
    hw_fd->buf[hw_fd->buf_len++] = 0x05;
    //hw_fd->buf_len+= sprintf((char*)(hw_fd->buf+hw_fd->buf_len), "%s", hw_datecs_count_bcc(hw_fd->buf));
    memcpy(hw_fd->buf+hw_fd->buf_len, hw_datecs_count_bcc(hw_fd->buf), 4);
    hw_fd->buf_len+=4;
    hw_fd->buf[hw_fd->buf_len++] = 0x03;
    hw_fd->buf[hw_fd->buf_len] = '\0';
    return(hw_datecs_send_buf(hw_fd));
}

int net_datecs_send_cmd(char *in_cmd) {
    int x=0;
    char strtmp[MAX_STRING_LEN];
    uint8_t cmd_str[HW_MAX_BUFF_SIZE];
    uint16_t cmd_len = 0;

    if(!in_cmd || !in_cmd[0]) return(52);

    for(x=0;x<strlen(in_cmd);x++)
        if ( (in_cmd[x] == '\r') || (in_cmd[x] == '\n') ) in_cmd[x]='\0';

    cmd_len=0;
    memset(cmd_str, 0, NET_MAX_BUFF_SIZE);
    cmd_str[cmd_len++] = 0x01;
    cmd_str[cmd_len++] = 0x20 + strlen(in_cmd) + 3;
    if (cmd_seq > 0x7F || cmd_seq < 0x20) cmd_seq = 0x20;
    cmd_str[cmd_len++] = cmd_seq++;
    cmd_len+= sprintf((char*)(cmd_str+3), "%s", in_cmd);
    cmd_str[cmd_len++] = 0x05;
    memcpy(cmd_str+cmd_len, hw_datecs_count_bcc(cmd_str), 4);
    cmd_len+=4;
    cmd_str[cmd_len++] = 0x03;
    cmd_str[cmd_len] = '\0';
    sprintf(strtmp, "hex:");
    for (x = 0; x < cmd_len; x++) {
	sprintf(strtmp, "%s [%02x]", strtmp, cmd_str[x]);
    }
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",cmd_len, strtmp);
    return(net_v2_send_cmd(NET_PRT_SEND_CMD,(char *)cmd_str));
}

int hw_datecs_check(char *device, uint8_t full_chk) {
    int i=0;
    int rc=0;
    speed_t baudrate[] = {B115200, B9600, B57600, B38400, B19200};

    add_log(__func__, LOG_DEBUG,"[detect] check datecs device \"%s\"",device);
    if ((rc = hw_sock_new(&hw_fd))) return(rc);
    hw_fd->device=xstrcpy(device);
    full_chk = (full_chk) ? (sizeof(baudrate)/sizeof(speed_t)) : 2;
    for (i = 0; i < full_chk; i++) {
	if ((rc = hw_datecs_connect(hw_fd,baudrate[i]))&&(rc!=47)&&(rc!=48)) return(rc);
	if (!rc) {
	    add_log(__func__, LOG_DEBUG,"[detect] Detected datecs on port \"%s\", connected on baudrate \"%lu\"", device, getBaudRateConst(baudrate[i]));
	    hw_datecs_disconnect(hw_fd);
	    return(0);
	}
    }
    hw_datecs_disconnect(hw_fd);
    return(13);
}
