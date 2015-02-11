#include <stdlib.h>

#include "net_connect.h"
#include "main.h"

net_socket_t *net_fd=NULL;

char *net_rem_host=NULL;
char *net_rem_port=NULL;

int net_sock_new(net_socket_t **in_soc, char *ip, char *port) {
    if(!in_soc) return(1);
    if(!(*in_soc)) {
        if(!((*in_soc)=(net_socket_t *)calloc(sizeof(net_socket_t), 1))) return(2);
    }
    if(ip) {
        if((*in_soc)->ip_addr) free((*in_soc)->ip_addr);
        (*in_soc)->ip_addr=xstrcpy(ip);
    }
    if(port) {
        if((*in_soc)->ip_port) free((*in_soc)->ip_port);
        (*in_soc)->ip_port=xstrcpy(port);
    }

    (*in_soc)->out_sock=1; /* stdout */
    (*in_soc)->inp_sock=0; /* stdin */
    (*in_soc)->rm_sock=0; /* network in-out sock */

    (*in_soc)->timeout=5; /* timeout */

    (*in_soc)->proto=0;

    return(0);
}

int net_make_connect(net_socket_t *in_soc) {
    struct sockaddr_in srv;
    struct pollfd ufd;
    int err_no=0;
    int sopt;
    int sstat;
    int slen;
    int rc;

    in_soc->str_error[0]='\0';

    if((!in_soc->ip_addr) || (!in_soc->ip_addr[0])) return(66);
    if((!in_soc->ip_port) || (!in_soc->ip_port[0])) return(66);

    if((in_soc->rm_sock=socket(AF_INET,SOCK_STREAM,0)) == -1) {
        err_no=errno;
        sprintf(in_soc->str_error, "Make Socket[%d]: %s", err_no, strerror(err_no));
        return(67);
    }

    memset(&srv, 0, sizeof(srv));
    srv.sin_family=AF_INET;
    srv.sin_port=htons(atoi(in_soc->ip_port));
    srv.sin_addr.s_addr=inet_addr(in_soc->ip_addr);

    /* Non-blocked socket */
    sopt=fcntl(in_soc->rm_sock, F_GETFL, 0);
    fcntl(in_soc->rm_sock, F_SETFL, sopt|O_NONBLOCK);

    rc=connect(in_soc->rm_sock,(struct sockaddr*)&srv,sizeof(srv));

    in_soc->out_sock=in_soc->rm_sock;
    in_soc->inp_sock=in_soc->rm_sock;

    if(rc == 0) {
        /* Connection establised */
        fcntl(in_soc->rm_sock, F_SETFL, sopt);
        return (0);
    }

    /* If real error, but not waiting connection */
    ufd.fd=in_soc->rm_sock;
    ufd.events=POLLOUT;
    rc=poll(&ufd, 1, in_soc->timeout*100);
    if (rc<0) {
        err_no=errno;
        sprintf(in_soc->str_error, "Connect Poll[%d]: %s", err_no, strerror(err_no));
        return(68);
    }
    if(rc==0) {
        sprintf(in_soc->str_error, "Connect Poll Timeout: %d s.", in_soc->timeout);
        return(69);
    }

    fcntl(in_soc->rm_sock, F_SETFL, sopt);

    /* Now we have to be sure connection is OK */
    slen=sizeof(sstat);
    if((rc=getsockopt(in_soc->rm_sock, SOL_SOCKET, SO_ERROR, (void *)&sstat, (socklen_t *)&slen)) < 0) {
        err_no=errno;
        sprintf(in_soc->str_error, "Connect GetSocOpt[%d]: %s", err_no, strerror(err_no));
        return(59);
    }
    return(sstat);
}



int net_get_peer(net_socket_t *in_soc) {
    char strtmp[MAX_STRING_LEN];
    struct sockaddr_in srv;
    int srvlen=sizeof(struct sockaddr_in);
    int err_no=0;

    /* Clear R/T buffer */
    memset(in_soc->buf, 0, NET_MAX_BUFF_SIZE);

    /* First be sure we work with the socket as input */
    memset(&srv, 0, sizeof(srv));
    if(getsockname(0, (struct sockaddr *)&srv, (socklen_t *)&srvlen)) {
        // So, we are work with standart input and output
        return(0);
    }

    if(srv.sin_family == AF_INET) {
        sprintf(strtmp, "%s", inet_ntoa(srv.sin_addr));
        if(strtmp[0]) in_soc->sock_host=xstrcpy(strtmp);
    }

    memset(&srv, 0, sizeof(srv));
    if(getpeername(0, (struct sockaddr *)&srv, (socklen_t *)&srvlen)) {
        err_no=errno;
        sprintf(in_soc->str_error, "GetPeerName[%d]: %s", err_no, strerror(err_no));
        // We can not to get know who is another side
        return(1);
    }

    if(srv.sin_family != AF_INET) {
        //add_log(__func__, 2, "It is not Internet income call.");
        // We accept only inet ip4 work, no another
        return(1);
    }

    sprintf(strtmp, "%s", inet_ntoa(srv.sin_addr));
    if(!strtmp[0]) {
        //add_log(__func__, 2, "Unknow remote host.");
        return(1);
    }

    in_soc->peer_host=xstrcpy(strtmp);
    return(0);
}

int net_recv_buf(net_socket_t *in_soc) {
    struct pollfd ufd;
    int res=0;
    int red_len=0;
    int err_no=0;
    int sopt=0;
    int x=0;

    //in_soc->lastop=time(NULL);
    memset(in_soc->buf, 0, NET_MAX_BUFF_SIZE);

    if(in_soc->peer_host) {
        /* Non-blocked socket */
        sopt=fcntl(in_soc->inp_sock, F_GETFL, 0);
        if((res=fcntl(in_soc->inp_sock, F_SETFL, sopt|O_NONBLOCK))) {
            err_no=errno;
            sprintf(in_soc->str_error, "SetSock[%d]: %s", err_no, strerror(err_no));
            return(60);
        }
    }

    ufd.fd=in_soc->inp_sock;
    ufd.events=POLLIN;
    if((res=poll(&ufd, 1, in_soc->timeout*1000)) < 0) {
        err_no=errno;
        sprintf(in_soc->str_error, "Read Poll[%d]: %s", err_no, strerror(err_no));
        return(61);
    }
    if (!res) return(0);

    in_soc->buf_len=0;
    ioctl(in_soc->inp_sock, FIONREAD, &in_soc->buf_len);
    if (!in_soc->buf_len) return(255);
    add_log(__func__, LOG_DEBUG,"ioctl(FIONREAD) = %u", in_soc->buf_len);

    if ((red_len = read(in_soc->inp_sock, in_soc->buf, in_soc->buf_len)) !=  in_soc->buf_len) {
        sprintf(in_soc->str_error, "Net: Read %d bytes, need %d bytes", red_len, in_soc->buf_len);
        return(62);
    }

    if (!in_soc->proto)
        for(x=0;x<red_len;x++)
            if ( (in_soc->buf[x] == '\r') || (in_soc->buf[x] == '\n') ) {
                in_soc->buf[x] = '\0';
                break;
            }
    return(0);
}

int net_send_buf(net_socket_t *in_soc) {
    struct pollfd ufd;
    int err_no=0;
    int res=0;

    in_soc->lastop=time(NULL);

    if(in_soc->buf_len > NET_MAX_BUFF_SIZE) return(63);

    /* Prepare to output Poll */
    ufd.fd=in_soc->out_sock;
    ufd.events=POLLOUT;
    res=poll(&ufd, 1, in_soc->timeout*1000);
    if (res<0) {
	err_no=errno;
	sprintf(in_soc->str_error, "Net: Send Poll[%d]: %s", err_no, strerror(err_no));
	return(64);
    }
    if (res==0) {
	err_no=errno;
        sprintf(in_soc->str_error, "Net: Send Poll Timeout: %d s.", in_soc->timeout);
	return(93);
    }

    res=write(in_soc->out_sock, in_soc->buf, in_soc->buf_len);
    if(res != in_soc->buf_len) {
        sprintf(in_soc->str_error, "Sent %d bytes instead %d", res, in_soc->buf_len);
        return(65);
    }
    return(0);
}

int net_v2_send_buf(net_socket_t *in_soc) {
    struct pollfd ufd;
    uint16_t buf_len=0;
    int err_no=0;
    int res=0;

    in_soc->lastop=time(NULL);

    buf_len=in_soc->buf[0] + in_soc->buf[1] * 256;
    if((buf_len < sizeof(buf_len)) || (buf_len > NET_MAX_BUFF_SIZE)) return(163);

    /* Prepare to output Poll */
    ufd.fd=in_soc->out_sock;
    ufd.events=POLLOUT;

    res=poll(&ufd, 1, in_soc->timeout*1000);
    if (res<0) {
	err_no=errno;
	sprintf(in_soc->str_error, "Net: Send Poll[%d]: %s", err_no, strerror(err_no));
	return(164);
    }
    if (res==0) {
	err_no=errno;
        sprintf(in_soc->str_error, "Net: Send Poll Timeout: %d s.", in_soc->timeout);
	return(193);
    }

    res=write(in_soc->out_sock, in_soc->buf, buf_len);
    if(res != buf_len) {
	sprintf(in_soc->str_error, "Sent %d bytes instead %d", res, buf_len);
	return(165);
    }
    return(0);
}

int net_v2_recv_buf(net_socket_t *in_soc) {
    struct pollfd ufd;
    int err_no=0;
    int res=0;
    uint16_t red_len=0;
    uint16_t in_len=2;
    int sopt=0;

    memset(in_soc->buf, 0, NET_MAX_BUFF_SIZE);
    in_soc->buf_len=0;

    while(red_len<in_len) {
	if(in_soc->peer_host) {
	    /* Non-blocked socket */
	    sopt=fcntl(in_soc->inp_sock, F_GETFL, 0);
	    if((res=fcntl(in_soc->inp_sock, F_SETFL, sopt|O_NONBLOCK))) {
		err_no=errno;
		sprintf(in_soc->str_error, "SetSock[%d]: %s", err_no, strerror(err_no));
		return(60);
	    }
        }

	ufd.fd=in_soc->inp_sock;
	ufd.events=POLLIN;
	if((res=poll(&ufd, 1, in_soc->timeout*1000)) < 0) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "Read Poll[%d]: %s", err_no, strerror(err_no));
	    return(161);
	}
	if (!res) return(0);

	if((res=read(in_soc->inp_sock, in_soc->buf+red_len, in_len-red_len)) == -1) {
	    err_no=errno;
	    sprintf(in_soc->str_error, "Net: Read[%d]: %s", err_no, strerror(err_no));
	    return(162);
	}
	red_len+=res;
	if(red_len==2) {
	    in_len=in_soc->buf[0] + in_soc->buf[1] * 256;
	    in_soc->buf_len=in_len;
//	    add_log(__func__, LOG_DEBUG,"buf[0]=%d, buf[1]=%d", in_soc->buf[0],in_soc->buf[1]);
//	    add_log(__func__, LOG_DEBUG,"red_len=%d, in_len=%d",red_len,in_len);
	    if(in_len < sizeof(in_len)) return(122);
	    if(in_len > NET_MAX_BUFF_SIZE) return(123);
	}
    }
    add_log(__func__, LOG_DEBUG,"finish red_len=%d, in_len=%d",red_len,in_len);
    return(0);
}
