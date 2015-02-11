#include "hw_prot.h"
#include "main.h"

#define PROT_PRIVAT_BYTE_STX 0x02
#define PROT_PRIVAT_BYTE_ACK 0x06
#define PROT_PRIVAT_RESP_TIMEOUT 2000
#define PROT_PRIVAT_BAUDRATE B115200

int prot_privat_setup_port(hw_socket_t *in_soc) {
    int rc=0;

    in_soc->keys.c_iflag = 0;
    in_soc->keys.c_oflag = 0;
    in_soc->keys.c_cflag = 0;
    in_soc->keys.c_lflag = 0;

    in_soc->keys.c_cflag |= (CLOCAL);

    /*** 8N1 ***/
    in_soc->keys.c_cflag &= ~PARENB;
    in_soc->keys.c_cflag &= ~CSTOPB;
    in_soc->keys.c_cflag &= ~CSIZE;
    in_soc->keys.c_cflag |= CS8;

    in_soc->keys.c_iflag |= (INPCK);

    in_soc->keys.c_iflag &= ~( IXANY | IXON | IXOFF );
    in_soc->keys.c_cflag &= ~CRTSCTS;

    add_log(__func__, LOG_DEBUG,"open port=\"%s\" protcol=PRIVAT", in_soc->device);
    in_soc->timeout = PROT_PRIVAT_RESP_TIMEOUT;

    if ((rc=hw_setup_port(in_soc, O_RDWR | O_NOCTTY | O_NDELAY, PROT_PRIVAT_BAUDRATE))) return(rc);

    if ( strncmp(in_soc->device, "/dev/ttyUSB", 11) != 0 ) {
        if ((rc=setIoctlParam(in_soc, TIOCM_RTS, 1))) return(rc);
        if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 1))) return(rc);
    }

    return(0);

}

int prot_privat_reset_port(hw_socket_t *in_soc) {
    int rc=0;
    if ( strncmp(in_soc->device, "/dev/ttyUSB", 11) != 0 ) {
        if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 0))) return(rc);
        if ((rc=setIoctlParam(in_soc, TIOCM_RTS, 0))) return(rc);
    }
    return(hw_close_device(in_soc));
}

int prot_privat_send_buf(hw_socket_t *in_soc) {
    int x=0;
    char strtmp[MAX_STRING_LEN];
    sprintf(strtmp, "hex:");
    for(x = 0; x < in_soc->buf_len;x++) {
        sprintf(strtmp, "%s [%02x]",strtmp, in_soc->buf[x]);
    }
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",in_soc->buf_len, strtmp);
    return(hw_send_buf(&(*in_soc)));
}

int prot_privat_recv_buf(hw_socket_t *in_soc, int in_len) {
    struct pollfd ufd;
    int res=0;
    int red_len=0;
    //int in_len=0;
    int err_no=0;
    char strtmp[MAX_STRING_LEN];

    if (!in_soc->sock) return(78);

    in_soc->lastop=time(NULL);
    memset(in_soc->buf, 0, HW_MAX_BUFF_SIZE);
    in_soc->buf_len=0;

    while(red_len<in_len) {
        ufd.fd=in_soc->sock;
        ufd.events=POLLIN;
        res=poll(&ufd, 1, in_soc->timeout);
        if (res < 0) {
            err_no=errno;
            sprintf(in_soc->str_error, "HW: Read Poll[%d]: %s", err_no, strerror(err_no));
            return(83);
        } else if (res == 0) {
            err_no=errno;
            sprintf(in_soc->str_error, "HW: Read Poll Timeout: %d ms.", in_soc->timeout);
            return(73);
        }

        if ((res=read(in_soc->sock, in_soc->buf+red_len, in_len-red_len)) == -1) {
            err_no=errno;
            sprintf(in_soc->str_error, "Read[%d]: %s", err_no, strerror(err_no));
            return(92);
        }

        red_len+=res;
        if (red_len == 3) {
            in_len+= (in_soc->buf[1] << 8) + in_soc->buf[2];
            in_soc->buf_len = in_len+1;
            add_log(__func__, LOG_DEBUG,"r< expected data len = %u", in_len);
            if (in_len > HW_MAX_BUFF_SIZE) return(35);
        }
    }
    add_log(__func__, LOG_DEBUG, "r< red_len=%d, in_len=%d", red_len, in_len);

    sprintf(strtmp,"hex:");
    for(res = 0; res < red_len; res++) sprintf(strtmp, "%s [%02x]",strtmp, in_soc->buf[res]);
    add_log(__func__, LOG_DEBUG,"r< (%d) \"%s\"",red_len, strtmp);

    return(0);
}

int prot_privat_dialog(hw_socket_t *in_soc) {
    int rc=0;
    int is_found=0;
    int i=0;
    char strtmp[MAX_STRING_LEN] = {
		0x02, 0x00, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 
		0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 
		0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x04, 0x00, 
		0x00, 0x00, 0x20, 0x35
	};

    memcpy(hw_fd->buf, strtmp, 28);
    hw_fd->buf_len = 28;
    if ((rc=prot_privat_send_buf(hw_fd))) return(rc);

    if ((rc=prot_privat_recv_buf(hw_fd,1))) return(rc);
    if (hw_fd->buf[0] != PROT_PRIVAT_BYTE_ACK) return(33);

    if ((rc=prot_privat_recv_buf(hw_fd,3))) return(rc);

    for(i = 3; i < hw_fd->buf_len-12; i++) {
	if (hw_fd->buf[i] == 0x32) {
	    add_log(__func__, LOG_DEBUG,"[detect] found 0x32 sequence at %d byte", i);
	    if (memcmp("TE7E", hw_fd->buf+i+8, 4) == 0) {
		is_found = i+8;
		add_log(__func__, LOG_DEBUG,"[detect] found TE7E sequence at %d byte", is_found);
		sprintf(strtmp, "TE7E");
		hw_fd->software = xstrcpy(strtmp);
	    }
	}
    }

    hw_fd->buf[0] = PROT_PRIVAT_BYTE_ACK;
    hw_fd->buf_len = 1;
    if ((rc=prot_privat_send_buf(hw_fd))) return(rc);

    if (!is_found) return(34);

    add_log(__func__, LOG_DEBUG, "[detect] Protocol privat detected on port \"%s\", SoftwareName \"%s\"", in_soc->device, in_soc->software);

    return(0);
}

int prot_privat_check(char *device) {
    int rc = 0;

    add_log(__func__, LOG_DEBUG, "[detect] check device \"%s\", protocol=PRIVAT",device);
    if ((rc = hw_sock_new(&hw_fd))) return(rc);
    hw_fd->device=xstrcpy(device);
    if ((rc = prot_privat_setup_port(hw_fd))) return(rc);

    rc = prot_privat_dialog(hw_fd);

    prot_privat_reset_port(hw_fd);

    if (rc == 33) sleep(1);

    return(rc);
}
