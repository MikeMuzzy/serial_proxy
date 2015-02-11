#include "net_prot.h"
#include "main.h"

hw_socket_t *hw_fd=NULL;
uint8_t hw_use_crc=0;
char *pr_device=NULL;

int setIoctlParam(hw_socket_t *in_soc, int param, unsigned short level) {
    int state;
    int err_no=0;

    if (ioctl(in_soc->sock, TIOCMGET, &state) == -1) {
        err_no=errno;
        sprintf(in_soc->str_error,"setIoctlParam[%d]: %s", err_no, strerror(err_no));
        return(18);
    }
    if (level) state |= param;
    else state &= ~param;
    if (ioctl(in_soc->sock, TIOCMSET, &state) == -1) {
        err_no=errno;
        sprintf(in_soc->str_error,"setIoctlParam[%d]: %s", err_no, strerror(err_no));
        return(19);
    }
    return(0);
}

int getIoctlParam(hw_socket_t *in_soc, int param) {
    int state;
    int err_no=0;

    if (ioctl(in_soc->sock, TIOCMGET, &state) == -1) {
        err_no=errno;
        sprintf(in_soc->str_error,"getIoctlParam[%d]: %s", err_no, strerror(err_no));
        return(20);
    }
    return((state & param));
}

unsigned int getBaudRateConst(speed_t baudrate) {
    switch(baudrate) {
        case B9600:
            return(9600);
        case B19200:
            return(19200);
        case B38400:
            return(38400);
        case B57600:
            return(57600);
        case B115200:
            return(115200);
        default:
            return(0);
    }
}

/*
int hw_add_opt_lst(char *in_name, char *in_val, int is_ro) {
    opt_lst_t *opt_tmp=NULL;
    opt_lst_t *opt_tek=NULL;

    if((!in_name) || (!in_name[0])) return(1);

    opt_tek=NULL;
    for(opt_tmp=hw_fd->opt_lst; opt_tmp && !opt_tek; opt_tmp=opt_tmp->next) {
	if (!strcmp(opt_tmp->opt_name, in_name)) opt_tek=opt_tmp;
    }
    opt_tmp=NULL;
    if(opt_tek) {
	if (opt_tek->is_ro) return(3);
	free(opt_tek->opt_val);
	opt_tek->opt_val=xtrcpy(in_val);
	return(0);
    }
    if(!(opt_tmp=(opt_lst_t *)calloc(sizeof(opt_lst_t), 1))) return(2);
    opt_tmp->opt_name=xstrcpy(in_name);
    opt_tmp->opt_val=xstrcpy(in_val);
    opt_tmp->is_ro=is_ro;
    if(!hw_fd->opt_lst) {
        hw_fd->opt_lst=opt_tmp;
    } else {
	for(opt_tek=hw_fd->opt_lst; opt_tek->next; opt_tek=opt_tek->next);
	opt_tek->next=opt_tmp;
	opt_tek=NULL;
    }
    opt_tmp=NULL;
    return(0);
}

int hw_get_opt_lst(char *in_name, char *out_val) {
    opt_lst_t *opt_tmp=NULL;

    for(opt_tmp=hw_fd->opt_lst; opt_tmp; opt_tmp=opt_tmp->next) {
	if (!strcmp(opt_tmp->opt_name, in_name)) break;
    }
    if (!opt_tmp) return(1);
    out_val=xstrcpy(opt_tmp->(*opt_val));
    return(0);
} */

int hw_sock_new(hw_socket_t **in_soc) {
    if (!in_soc) return(17);
    if (!(*in_soc)) {
        if (!((*in_soc)=(hw_socket_t *)calloc(sizeof(hw_socket_t),1))) return(12);
    }
    (*in_soc)->sock=0;

    (*in_soc)->timeout=HW_SOCKET_IO_TIMEOUT; /* timeout */

    (*in_soc)->state=HW_STATE_CLEAN;

    (*in_soc)->software=NULL;

    return(0);
}

int hw_setup_port(hw_socket_t *in_soc, int flags, speed_t baud_rate) {
    int err_no=0;

    if (!in_soc->device) return(10);
    if (!in_soc->sock) {
	if ((in_soc->sock = open(in_soc->device, flags)) == -1) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "HW: Open Device(%s) Error[%d]: %s", in_soc->device, err_no, strerror(err_no));
	    return(11);
	}
	memset(&in_soc->old_keys, 0, sizeof(struct termios));
	tcgetattr(in_soc->sock, &in_soc->old_keys);
    }

    cfsetospeed(&in_soc->keys, baud_rate);
    cfsetispeed(&in_soc->keys, baud_rate);

    if (tcflush(in_soc->sock, TCIOFLUSH) == -1) {
	err_no=errno;
	sprintf(in_soc->str_error, "HW: tcflush Error[%d]: %s", err_no, strerror(err_no));
	return(14);
    }
    if (tcsetattr(in_soc->sock, TCSANOW, &in_soc->keys) == -1) {
	err_no=errno;
	sprintf(in_soc->str_error, "HW: tcsetattr Error[%d]: %s", err_no, strerror(err_no));
	return(15);
    }

    return(0);
}

int hw_init_device(hw_socket_t *in_soc, speed_t baud_rate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits, uint8_t full_modem, uint8_t vmin, uint8_t vtime) {
    int err_no=0;
    if (!in_soc->device) return(10);
    if (!in_soc->sock) {
	if ((in_soc->sock = open(in_soc->device, O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "HW: Open Device(%s) Error[%d]: %s", in_soc->device, err_no, strerror(err_no));
	    return(11);
	}
	fcntl(in_soc->sock, F_SETFL, FNDELAY);
	memset(&in_soc->old_keys, 0, sizeof(struct termios));
	tcgetattr(in_soc->sock, &in_soc->old_keys);
	tcgetattr(in_soc->sock, &in_soc->keys);
	in_soc->keys.c_lflag = 0;
	in_soc->keys.c_oflag = 0;
	in_soc->keys.c_iflag = 0;
    }
    in_soc->keys.c_cc[VMIN] = (char) vmin;
    in_soc->keys.c_cc[VTIME] = (char) vtime;

    cfsetospeed(&in_soc->keys, baud_rate);
    cfsetispeed(&in_soc->keys, baud_rate);

    in_soc->keys.c_cflag &= ~CSIZE;
    switch(data_bits) {
	case '8': in_soc->keys.c_cflag |= CS8;
	    break;
	case '7': in_soc->keys.c_cflag |= CS7;
	    break;
	case '6': in_soc->keys.c_cflag |= CS6;
	    break;
	case '5': in_soc->keys.c_cflag |= CS5;
	    break;
	default:
	    return(12);
    }

    switch(parity) {
	case 'O':
	case 'o':
	    in_soc->keys.c_iflag |= (INPCK);
	    in_soc->keys.c_cflag |= PARENB | PARODD;
	    break;
	case 'E':
	case 'e':
	    in_soc->keys.c_iflag |= (INPCK);
	    in_soc->keys.c_cflag |= PARENB;
	    in_soc->keys.c_cflag &= ~PARODD;
	    break;
	case 'N':
	case 'n':
	    in_soc->keys.c_iflag |= (INPCK);
	    in_soc->keys.c_cflag &= ~PARENB;
	    break;
	default:
	    in_soc->keys.c_iflag &= ~INPCK;
	    in_soc->keys.c_cflag &= ~PARENB;
    }

    switch(stop_bits) {
	case '1': in_soc->keys.c_cflag &= ~CSTOPB;
	    break;
	case '2': in_soc->keys.c_cflag |= CSTOPB;
	    break;
	default: return(13);
    }

    if (full_modem) {
	in_soc->keys.c_iflag &= ~( IXANY | IXON | IXOFF );
	in_soc->keys.c_cflag |= (CRTSCTS);
	in_soc->keys.c_cflag &= ~(CLOCAL);
    } else {
	in_soc->keys.c_iflag &= ~( IXANY | IXON | IXOFF );
	in_soc->keys.c_cflag &= ~(CRTSCTS);
	in_soc->keys.c_cflag |= (CLOCAL);
    }

    //if (tcflush(in_soc->sock, TCIFLUSH) == -1) {
    if (tcflush(in_soc->sock, TCIOFLUSH) == -1) {
	err_no=errno;
	sprintf(in_soc->str_error, "HW: tcflush Error[%d]: %s", err_no, strerror(err_no));
	return(14);
    }
    if (tcsetattr(in_soc->sock, TCSANOW, &in_soc->keys) == -1) {
        err_no=errno;
        sprintf(in_soc->str_error, "HW: tcsetattr Error[%d]: %s", err_no, strerror(err_no));
        return(15);
    }
    return(0);
}

int hw_close_device(hw_socket_t *in_soc) {
    int err_no=0;
    tcflush(in_soc->sock, TCIFLUSH);
    tcsetattr(in_soc->sock, TCSANOW, &in_soc->old_keys);
    if ( close(in_soc->sock) == -1 ) {
        err_no=errno;
	sprintf(in_soc->str_error, "HW: close Error[%d]: %s", err_no, strerror(err_no));
	return(16);
    }
    add_log(__func__, LOG_DEBUG,"Close device \"%s\"",in_soc->device);
    return(0);
}

int hw_send_buf(hw_socket_t *in_soc) {
    struct pollfd ufd;
    int err_no=0;
    int res=0;

    in_soc->lastop=time(NULL);

    if(in_soc->buf_len > HW_MAX_BUFF_SIZE) return(21);

    ufd.fd=in_soc->sock;
    ufd.events=POLLOUT;
    res=poll(&ufd, 1, in_soc->timeout);
    if (res<0) {
        err_no=errno;
        sprintf(in_soc->str_error, "HW: Send Poll[%d]: %s", err_no, strerror(err_no));
        return(22);
    }
    if (res==0) {
        err_no=errno;
        sprintf(in_soc->str_error, "HW: Send Poll Timeout: %d ms.", in_soc->timeout);
        return(23);
    }

    res=write(in_soc->sock, in_soc->buf, in_soc->buf_len);
    if(res != in_soc->buf_len) {
        err_no=errno;
        sprintf(in_soc->str_error, "Sent %d bytes instead %d, Err:(%d)%s", res, in_soc->buf_len,err_no,strerror(err_no));
        return(24);
    }
    add_log(__func__, LOG_DEBUG,"s> %d bytes sent",in_soc->buf_len);
    return(0);
}

int hw_detect_device(char *arg1, char *arg2) {
    int rc=0;

    if (arg1 == NULL) return(12);

    if (arg2 == NULL) {
	if (strncmp("ttyACM", basename(arg1), 6) == 0) {
	    if (!(rc=prot_privat_check(arg1))) {
		if (hw_fd->software)
		    printf("privat:%s",hw_fd->software);
		else 
		    printf("privat");
		return(0);
	    }
	}
	if (!(rc=prot_fuib_check(arg1))) {
	    printf("fuib");
	    return(0);
	}

	if (!(rc=hw_datecs_check(arg1, 0))) {
	    printf("datecs");
	    return(0);
	}
	if (!(rc=hw_maria301_check(arg1, 1))) {
	    printf("maria301");
	    return(0);
	}
	if(hw_fd->str_error[0]) add_log(__func__, LOG_ALERT, "ERROR! %s", hw_fd->str_error);
	add_log(__func__, LOG_INFO , "--- Finished with: %d", rc);
	if (!rc) printf("unknown");
	return(rc);
    } else {
	if (!strcmp("maria301", arg1)) {
	    rc = hw_maria301_check(arg2, 0);
	} else if (!strcmp("datecs", arg1)) {
	    rc = hw_datecs_check(arg2, 1);
	} else if (!strcmp("privat", arg1)) {
	    rc = prot_privat_check(arg2);
	} else if (!strcmp("fuib", arg1)) {
	    rc = prot_fuib_check(arg2);
	} else {
	    rc = 13;
	}
	if((hw_fd != NULL)&&(hw_fd->str_error[0])) add_log(__func__, LOG_ALERT, "ERROR! %s", hw_fd->str_error);
	add_log(__func__, LOG_INFO , "--- Finished with: %d", rc);
	return(rc);
    }
}
