#include "hw_prot.h"
#include "net_prot.h"
#include "main.h"

int hw_maria301_connect(hw_socket_t *in_soc) {
    int rc=0;
    int tries=0;
    time_t ctime;

    /* init mariya*/
    for(;;) {
        if (tries++>3) return(84);
        if ((rc=net_send_cmd("WAIT"))) return(rc);

        if (!in_soc->sock) {
	    if ((rc=hw_init_device(in_soc, HW_MARIA301_DEFAULT_BAUDRATE, '8', 'E', '2', 0, 255, 1))) return(rc);
	}

        /* set reciver readiness to 1 */
        if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 1))) return(rc);
        /* check app readiness */
        ctime=time(NULL);

	add_log(__func__, LOG_DEBUG,"begin sync with ekka, DTR=%d", (getIoctlParam(in_soc, TIOCM_DTR)) ? 1 : 0);
	while ((time(NULL)-ctime) <= 10) {
	    memset(in_soc->buf,0,sizeof(in_soc->buf));
	    in_soc->buf[0] = 'U';
	    in_soc->buf_len = 1;
	    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",in_soc->buf_len, in_soc->buf);
	    if ((rc=hw_maria301_send_buf(in_soc))) return(rc);
	    usleep(10000);
	    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",in_soc->buf_len, in_soc->buf);
	    if ((rc=hw_maria301_send_buf(in_soc))) return(rc);
	    in_soc->timeout = 1000;
	    if ((rc=hw_maria301_recv_cmd()) && (rc != 73)) return(rc);
	    in_soc->timeout = HW_SOCKET_IO_TIMEOUT;
	    if ( hw_fd->state & HW_STATE_IS_READY ) return(0);
	    if ((rc=net_send_cmd("WAIT"))) return(rc);
	}

	add_log(__func__, LOG_DEBUG,"sync with ekka false. Set DTR=0 and sleep(3).");
        /* can't sync with app, sleep & repeat */
        if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 0))) return(rc);
        sleep(3);
    }
    return(0);
}

int hw_maria301_disconnect(hw_socket_t *in_soc) {
    int rc=0;
    //if ((rc=setIoctlParam(in_soc, TIOCM_DTR, 0))) return(rc);
    rc=setIoctlParam(in_soc, TIOCM_DTR, 0);
    add_log(__func__, LOG_DEBUG ,"setIoctlParam(TIOCM_DTR)=%d", rc);
    return(hw_close_device(&(*in_soc)));
}

int hw_maria301_recv_buf(hw_socket_t *in_soc) {
    struct pollfd ufd;
    int res=0;
    uint8_t *strptr;
    int red_len=0;
    int err_no=0;
    int r=0;

    if (!in_soc->sock) return(78);

    in_soc->lastop=time(NULL);
    memset(in_soc->buf, 0, HW_MAX_BUFF_SIZE);
    in_soc->buf_len=0;

    ufd.fd=in_soc->sock;
    ufd.events=POLLIN;
    res=poll(&ufd, 1, in_soc->timeout);
    if (res<0) {
	err_no=errno;
	sprintf(in_soc->str_error, "HW: Read Poll[%d]: %s", err_no, strerror(err_no));
	return(83);
    }
    if (res==0) {
	err_no=errno;
        sprintf(in_soc->str_error, "HW: Read Poll Timeout: %d ms.", in_soc->timeout);
	return(73);
    }

    ioctl(in_soc->sock, FIONREAD, &r);
    add_log(__func__, LOG_DEBUG,"ioctl(FIONREAD) = %u", r);

    strptr = in_soc->buf;
    while ( (red_len = read(in_soc->sock, strptr, in_soc->buf + sizeof(in_soc->buf)-strptr-1)) > 0 ) {
        strptr+= red_len;
        in_soc->buf_len+=red_len;
        if(!(res=poll(&ufd, 1, 10))) break;
        ioctl(in_soc->sock, FIONREAD, &r);
        add_log(__func__, LOG_DEBUG,"ioctl(FIONREAD) = %u", r);
    }
    *strptr = '\0';

    if (red_len == -1) {
        err_no=errno;
        sprintf(in_soc->str_error, "Read[%d]: %s", err_no, strerror(err_no));
        return(92);
    }

    return(0);
}

int hw_maria301_recv_cmd() {
    int rc=0;
    cmd_lst_t *cmd_tmp=NULL;
    cmd_lst_t *cmd_tek=NULL;
    uint16_t cmd_len=0;
    uint16_t crc=0;
    int x=0,y=0;
    char strtmp[MAX_STRING_LEN];

    /* clean cmd list */
    for(cmd_tmp=hw_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) free(cmd_tmp);
    hw_fd->cmd_lst=NULL;

    if ((rc=hw_maria301_recv_buf(hw_fd))) return(rc);
    if (hw_fd->buf[0] == 253) {
        add_log(__func__, LOG_DEBUG,"r< %d bytes", hw_fd->buf_len);
        memset(strtmp,0,MAX_STRING_LEN);
        for(x=0;x<hw_fd->buf_len;x++) {
            strtmp[y++] = hw_fd->buf[x];
            if (hw_fd->buf[x] == 254) {
                strtmp[y] = '\0';
                if ( (hw_use_crc) && (x<hw_fd->buf_len) && (hw_fd->buf[x+1] != 253) ) {
                    memcpy(&crc,hw_fd->buf+x,2);
                    x+=2;
                }
                cmd_len = (uint8_t)strtmp[y-2];
                memcpy(strtmp, strtmp+1,cmd_len-1);
                strtmp[cmd_len-1] = '\0';
                add_log(__func__, LOG_DEBUG,"rc< (%d|%d) \"%s\"",cmd_len-1, strlen(strtmp), strtmp);

                if (!strcmp(strtmp,"DONE")) hw_fd->state |= HW_STATE_IS_DONE;
                else if (!strcmp(strtmp,"READY")) hw_fd->state |= HW_STATE_IS_READY;
                else if (!strcmp(strtmp,"WRK")) hw_fd->state |= HW_STATE_IS_WORK;
                else if (!strcmp(strtmp,"WAIT")) hw_fd->state |= HW_STATE_IS_WAIT;
                else if (!strcmp(strtmp,"PRN")) hw_fd->state |= HW_STATE_IS_PRN;
                else if (!strcmp(strtmp,"SOFTBLOCK")) hw_fd->state |= HW_STATE_IS_SOFTBLOCK;

                if (strlen(strtmp)>=3) {
                    hw_fd->lastop = time(NULL);
                    if (!(cmd_tmp=(struct cmd_lst_s *)calloc(sizeof(struct cmd_lst_s),1))) return(86);
                    cmd_tmp->str = xstrcpy(strtmp);
                    cmd_tmp->ctime = time(NULL);
                    if (!hw_fd->cmd_lst) {
                         hw_fd->cmd_lst=cmd_tmp;
                    } else {
                        for(cmd_tek=hw_fd->cmd_lst;cmd_tek->next;cmd_tek=cmd_tek->next);
                        cmd_tek->next=cmd_tmp;
                        cmd_tek=NULL;
                    }
                    cmd_tmp=NULL;
                }
                strtmp[0] = '\0';
                y=0;
            }
        }
    }
    return(0);
}

int hw_maria301_send_buf(hw_socket_t *in_soc) {
    in_soc->state = HW_STATE_CLEAN;
    return(hw_send_buf(&(*in_soc)));
}

int hw_maria301_send_cmd(char *in_cmd) {
    uint16_t crc=0;
    int x=0;

    if(!in_cmd || !in_cmd[0]) return(90);

    for(x=0;x<strlen(in_cmd);x++)
        if ( (in_cmd[x] == '\r') || (in_cmd[x] == '\n') ) in_cmd[x]='\0';

    hw_fd->buf_len=0;
    memset(hw_fd->buf, 0, NET_MAX_BUFF_SIZE);
    hw_fd->buf[hw_fd->buf_len++] = 253;
    hw_fd->buf_len+= sprintf((char*)(hw_fd->buf+1), "%s", in_cmd);
    hw_fd->buf[hw_fd->buf_len++] = strlen(in_cmd)+1;
    hw_fd->buf[hw_fd->buf_len++] = 254;
    hw_fd->buf[hw_fd->buf_len] = '\0';
    if (hw_use_crc) {
        crc = getCRC16(&hw_fd->buf, hw_fd->buf_len);
        memcpy(&hw_fd->buf[hw_fd->buf_len], &crc, 2);
        add_log(__func__, LOG_DEBUG,"add_crc: [%u][%u]",hw_fd->buf[hw_fd->buf_len], hw_fd->buf[hw_fd->buf_len+1]);
        hw_fd->buf_len+=2;
    }
    if (!strcmp("CSIN1",in_cmd)) hw_use_crc=1;
    if (!strcmp("CSIN0",in_cmd)) hw_use_crc=0;
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",strlen(in_cmd), in_cmd);
    return(hw_maria301_send_buf(hw_fd));
}

int hw_maria301_check(char *device, uint8_t reset) {
    int rc=0;
    time_t ctime;

    add_log(__func__, LOG_DEBUG,"[detect] check maria301 device \"%s\"",device);

    if ((rc = hw_sock_new(&hw_fd))) return(rc);
    hw_fd->device=xstrcpy(device);
    if ((rc=hw_init_device(hw_fd, HW_MARIA301_DEFAULT_BAUDRATE, '8', 'E', '2', 0, 255, 1))) return(rc);

    if (reset) {
	if ((rc=setIoctlParam(hw_fd, TIOCM_RTS, 0))) return(rc);
	if ((rc=setIoctlParam(hw_fd, TIOCM_DTR, 0))) return(rc);
	sleep(2);
    }

    /* set reciver readiness to 1 */
    if ((rc=setIoctlParam(hw_fd, TIOCM_DTR, 1))) return(rc);
    /* check app readiness */
    ctime=time(NULL);

    add_log(__func__, LOG_DEBUG,"begin sync with ekka, DTR=%d", (getIoctlParam(hw_fd, TIOCM_DTR)) ? 1 : 0);
    while ((time(NULL)-ctime) <= 10) {
	memset(hw_fd->buf,0,sizeof(hw_fd->buf));
	hw_fd->buf[0] = 'U';
	hw_fd->buf_len = 1;
	add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",hw_fd->buf_len, hw_fd->buf);
	if ((rc=hw_maria301_send_buf(hw_fd))) return(rc);
	usleep(10000);
	add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",hw_fd->buf_len, hw_fd->buf);
	if ((rc=hw_maria301_send_buf(hw_fd))) return(rc);
	hw_fd->timeout = 1000;
	if ((rc=hw_maria301_recv_cmd()) && (rc != 73)) return(rc);
	hw_fd->timeout = HW_SOCKET_IO_TIMEOUT;
	if ( hw_fd->state & HW_STATE_IS_READY ) {
	    add_log(__func__, LOG_DEBUG,"[detect] Detected Maria301 on port \"%s\"", device);
	    hw_maria301_disconnect(hw_fd);
	    return(0);
	}
    }

    add_log(__func__, LOG_DEBUG,"[detect] sync with ekka false. Set DTR=0");
    hw_maria301_disconnect(hw_fd);
    return(13);
}
