#include "hw_prot.h"
#include "main.h"

#define PROT_FUIB_STX 0x02
#define PROT_FUIB_ETX 0x03
#define PROT_FUIB_EOT 0x04
#define PROT_FUIB_ACK 0x06
#define PROT_FUIB_NAK 0x15
#define PROT_FUIB_FS 0x1c

#define PROT_FUIB_RESP_TIMEOUT 1000
#define PROT_FUIB_BAUDRATE B9600

int prot_fuib_recv_cmd(char in_cmd);
int prot_fuib_send_cmd(char in_cmd, char *in_data);

uint8_t prot_fuib_LRC(uint8_t *cmd_str) {
    uint8_t *c = cmd_str;
    uint8_t lrc = 0x00;
    if (PROT_FUIB_STX != *c++) return(0);
    while (PROT_FUIB_ETX != *c) lrc ^= *c++;
    lrc ^= *c++;
    return(lrc);
}

int prot_fuib_setup_port(hw_socket_t *in_soc) {
    int rc=0;

    in_soc->keys.c_iflag = 0;
    in_soc->keys.c_oflag = 0;
    in_soc->keys.c_cflag = 0;
    in_soc->keys.c_lflag = 0;

//    in_soc->keys.c_cflag |= (CLOCAL|CREAD);
    in_soc->keys.c_cflag |= (CLOCAL);

    in_soc->keys.c_cflag |= PARENB;
    in_soc->keys.c_cflag &= ~PARODD;
    in_soc->keys.c_cflag &= ~CSTOPB;
    in_soc->keys.c_cflag &= ~CSIZE;
    in_soc->keys.c_cflag |= CS7;

//    in_soc->keys.c_iflag |= (INPCK|ISTRIP);
    in_soc->keys.c_iflag |= (INPCK);

    in_soc->keys.c_iflag &= ~( IXANY | IXON | IXOFF );
    in_soc->keys.c_cflag &= ~CRTSCTS;

    add_log(__func__, LOG_DEBUG,"open port=\"%s\" protcol=FUIB", in_soc->device);
    in_soc->timeout = PROT_FUIB_RESP_TIMEOUT;

    if ((rc=hw_setup_port(in_soc, O_RDWR | O_NOCTTY | O_NDELAY, PROT_FUIB_BAUDRATE))) return(rc);
    if ( strncmp(in_soc->device, "/dev/ttyUSB", 11) != 0 ) {
	if ((rc=setIoctlParam(in_soc, TIOCM_RTS, 1))) return(rc);
	if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 1))) return(rc);
    }

    return(0);
}

int prot_fuib_reset_port(hw_socket_t *in_soc) {
    int rc=0;
    if ( strncmp(in_soc->device, "/dev/ttyUSB", 11) != 0 ) {
	if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 0))) return(rc);
	if ((rc=setIoctlParam(in_soc, TIOCM_RTS, 0))) return(rc);
    }
    return(hw_close_device(in_soc));
}

int prot_fuib_recv_buf(hw_socket_t *in_soc, uint16_t in_len) {
    struct pollfd ufd;
    int res=0;
    int red_len=0;
    int err_no=0;

//    int r=0;

    if (!in_soc->sock) return(78);

    in_soc->lastop=time(NULL);
    memset(in_soc->buf, 0, HW_MAX_BUFF_SIZE);
    in_soc->buf_len=0;

    if (!in_len) in_len = HW_MAX_BUFF_SIZE;

    while(red_len < in_len) {
	ufd.fd = in_soc->sock;
	ufd.events = POLLIN;
	res = poll(&ufd, 1, ((red_len) ? 10 : in_soc->timeout));
	if (res < 0) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "HW: Read Poll[%d]: %s", err_no, strerror(err_no));
	    return(83);
	} else if (res == 0) {
	    if (red_len) break;
	    sprintf(in_soc->str_error, "HW: Read Poll Timeout: %d ms.", in_soc->timeout);
	    return(73);
	}

//	ioctl(in_soc->sock, FIONREAD, &r);
//	add_log(__func__, LOG_DEBUG, "ioctl(FIONREAD) = %u", r);

	if ((res = read(in_soc->sock, in_soc->buf+red_len, in_len-red_len)) == -1) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "Read[%d]: %s", err_no, strerror(err_no));
	    return(92);
	}
	red_len+= res;
    }

    in_soc->buf_len = red_len;

    return(0);
}

int prot_fuib_recv_cmd(char in_cmd) {
    int x=0;
    int rc=0;
    int in_len;
    char strtmp[MAX_STRING_LEN];

    in_len = (in_cmd != PROT_FUIB_STX) ? 1 : 0;

    if ((rc=prot_fuib_recv_buf(hw_fd,in_len))) return(rc);
    sprintf(strtmp,"hex:");
    for(x = 0; x < hw_fd->buf_len;x++) {
	sprintf(strtmp, "%s [%02x]",strtmp, hw_fd->buf[x]);
    }
    add_log(__func__, LOG_DEBUG,"r< (%d) \"%s\"",hw_fd->buf_len, strtmp);

    if (strncmp((char *)hw_fd->buf+1,"GNS11", 5) == 0) {
	memcpy(strtmp, hw_fd->buf+7, 10);
	strtmp[10] = '\0';
	hw_fd->software = xstrcpy(strtmp);
    }

    if (in_cmd) {
	if (hw_fd->buf[0] != in_cmd) return(53);
	if (hw_fd->buf[0] == PROT_FUIB_STX) {
	    prot_fuib_send_cmd(PROT_FUIB_ACK, NULL);
	}
	return(0);
    } else {
	return(0);
    }
}

int prot_fuib_send_buf(hw_socket_t *in_soc) {
    int x=0;
    char strtmp[MAX_STRING_LEN];
    sprintf(strtmp, "hex:");
    for(x = 0; x < in_soc->buf_len;x++) {
        sprintf(strtmp, "%s [%02x]",strtmp, in_soc->buf[x]);
    }
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",in_soc->buf_len, strtmp);
    return(hw_send_buf(&(*in_soc)));
}

int prot_fuib_send_cmd(char in_cmd, char *in_data) {
    uint16_t data_len=0;
    uint8_t rc=0;

    if(!in_cmd) return(52);

    memset(hw_fd->buf, 0, NET_MAX_BUFF_SIZE);
    hw_fd->buf_len = 0;
    hw_fd->buf[hw_fd->buf_len++] = in_cmd;
    if ((in_cmd == PROT_FUIB_STX) && (in_data) && (in_data[0])) {
	data_len = strlen(in_data);
	memcpy(hw_fd->buf+hw_fd->buf_len, in_data, data_len);
	hw_fd->buf_len+= data_len;
	hw_fd->buf[hw_fd->buf_len++] = PROT_FUIB_ETX;
	hw_fd->buf[hw_fd->buf_len++] = prot_fuib_LRC(hw_fd->buf);
    }
    hw_fd->buf[hw_fd->buf_len] = '\0';
    if ((rc = prot_fuib_send_buf(hw_fd))) return(rc);
    if (in_cmd == PROT_FUIB_STX) {
	return(prot_fuib_recv_cmd(PROT_FUIB_ACK));
    }
    return(0);
}

int prot_fuib_dialog(hw_socket_t *in_soc) {
    int rc=0;
    char strtmp[MAX_STRING_LEN];

    sprintf(strtmp, "GNS10.%c", PROT_FUIB_FS);
    if ((rc = prot_fuib_send_cmd(PROT_FUIB_STX, strtmp))) return(rc);

    if ((rc = prot_fuib_recv_cmd(PROT_FUIB_STX))) return(rc);
    if (strncmp(in_soc->software, "FUIB", 4) != 0) return(25);
    if ((rc = prot_fuib_send_cmd(PROT_FUIB_EOT,NULL))) return(rc);
    add_log(__func__, LOG_DEBUG,"[detect] Protocol FUIB detected on port \"%s\", SoftwareName \"%s\"",in_soc->device, in_soc->software);
    return(0);
}

int prot_fuib_check(char *device) {
    int rc=0;

    add_log(__func__, LOG_DEBUG,"[detect] check device \"%s\", protcol=FUIB",device);

    if ((rc = hw_sock_new(&hw_fd))) return(rc);

    hw_fd->device=xstrcpy(device);
    if ((rc = prot_fuib_setup_port(hw_fd))) return(rc);

    rc = prot_fuib_dialog(hw_fd);

    prot_fuib_reset_port(hw_fd);
    return(rc);
}
