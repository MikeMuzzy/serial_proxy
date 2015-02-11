#include "net_prot.h"
#include "hw_prot.h"
#include "main.h"

int net_v2_send_cmd(uint8_t in_cmd, char *cmd_str);

int net_send_cmd(char *in_cmd) {
    //int cmd_len=0;
    uint16_t crc=0;
    int x=0;

    if (!in_cmd || !in_cmd[0]) return(89);

    for(x=0;x<strlen(in_cmd);x++)
        if ( (in_cmd[x] == '\r') || (in_cmd[x] == '\n') ) in_cmd[x]='\0';

    if (net_fd->proto==NET_PROTO_V2) return(net_v2_send_cmd(NET_PRT_SEND_CMD, in_cmd));

    net_fd->buf_len=0;
    memset(net_fd->buf, 0, NET_MAX_BUFF_SIZE);
    if (net_fd->proto) {
        net_fd->buf[net_fd->buf_len++] = 253;
        net_fd->buf_len+= sprintf((char*)(net_fd->buf+1), "%s", in_cmd);
        net_fd->buf[net_fd->buf_len++] = strlen(in_cmd)+1;
        net_fd->buf[net_fd->buf_len++] = 254;
        net_fd->buf[net_fd->buf_len] = '\0';
        if (hw_use_crc) {
            crc = getCRC16(&net_fd->buf, net_fd->buf_len);
            memcpy(&net_fd->buf[net_fd->buf_len], &crc, 2);
            net_fd->buf_len+=2;
        }
    } else {
	sprintf((char *)net_fd->buf,"%s\r\n",in_cmd);
	net_fd->buf_len=strlen(in_cmd);
    }
    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",strlen(in_cmd), in_cmd);
    return(net_send_buf(net_fd));
}

int net_v2_send_cmd(uint8_t in_cmd, char *cmd_str) {
    uint16_t cmd_len=0;
    memset(net_fd->buf, 0, NET_MAX_BUFF_SIZE);
    net_fd->buf[3] = in_cmd;
    net_fd->buf[2] = NET_PROT_NUM;
//    char strtmp[MAX_STRING_LEN];
//    int x=0;

    if((cmd_str) && (cmd_str[0])) {
//	if((cmd_len=strlen(cmd_str) + NET_HDR_SIZE + 1) > NET_MAX_SIZE) return(190);
	if((cmd_len=strlen(cmd_str) + NET_HDR_SIZE) > NET_MAX_SIZE) return(190);
	net_fd->buf[0] = cmd_len % 256;
	net_fd->buf[1] = cmd_len / 256;
	sprintf((char *)(net_fd->buf+NET_HDR_SIZE), "%s", cmd_str);
/*
	memset(strtmp, 0, MAX_STRING_LEN);
	for (x=0; x<strlen(cmd_str); x++) {
	    if ( ((uint8_t)cmd_str[x] >= 0x21) && ((uint8_t)cmd_str[x] <= 0x7e) )
		sprintf(strtmp, "%s%c", strtmp, cmd_str[x]);
	    else
		sprintf(strtmp, "%s[%02x]", strtmp, (uint8_t)cmd_str[x]);
	}*/
	add_log(__func__, LOG_DEBUG,"s> (%d) [%d]\"%s\"",cmd_len, in_cmd,cmd_str);
//	add_log(__func__, LOG_DEBUG,"s> (%d) [%d]\"%s\"", cmd_len, in_cmd, strtmp);
    } else {
	cmd_len=NET_HDR_SIZE;
	net_fd->buf[0] = cmd_len % 256;
	net_fd->buf[1] = cmd_len / 256;
	add_log(__func__, LOG_DEBUG,"s> (%d) [%d]",cmd_len, in_cmd);
    }
    return(net_v2_send_buf(net_fd));
}

int net_v2_recv_cmd(uint8_t exp_cmd) {
    uint16_t in_len=0;
    int rc=0;

    if((rc=net_v2_recv_buf(net_fd))) return(rc);
    in_len=net_fd->buf[0] + net_fd->buf[1] * 256;
    if(in_len > NET_HDR_SIZE) {
	add_log(__func__, LOG_DEBUG,"r< (%d) [%d]\"%s\"",in_len, net_fd->buf[3], net_fd->buf+NET_HDR_SIZE);
    } else if (in_len == NET_HDR_SIZE) {
	add_log(__func__, LOG_DEBUG,"r< (%d) [%d]",in_len, net_fd->buf[3]);
    } else {
	return(0);
    }
    if (exp_cmd != 0) {
	if (net_fd->buf[3] != exp_cmd) return(124);
    }
    return(0);
}

int net_recv_cmd() {
    int rc=0;
    int is_ready=0;
    int is_done=0;
    uint16_t cmd_len=0;
    uint16_t crc=0;
    int x=0,y=0;
    char strtmp[MAX_STRING_LEN];

    while(!is_ready) {
        if((rc=net_recv_buf(net_fd))) return(rc);
        if (net_fd->buf[0] == 253) {
            add_log(__func__, LOG_DEBUG,"r< %d bytes", net_fd->buf_len);
            for(x=0;x<net_fd->buf_len;x++) {
                strtmp[y++] = net_fd->buf[x];
                if (net_fd->buf[x] == 254) {
                    strtmp[y] = '\0';
                    //if ( (use_crc) && (net_fd->buf[x+1]) && (net_fd->buf[x+1] != 253) ) {
                    if ( (hw_use_crc) && (x<net_fd->buf_len) && (net_fd->buf[x+1] != 253) ) {
                        memcpy(&crc,net_fd->buf+x,2);
                        x+=2;
                    }
                    cmd_len = (uint8_t)strtmp[y-2];
                    memcpy(strtmp, strtmp+1,cmd_len-1);
                    strtmp[cmd_len-1] = '\0';
                    add_log(__func__, LOG_DEBUG,"rc< (%d|%d) \"%s\"",cmd_len-1, strlen(strtmp), strtmp);
                    if (!strcmp(strtmp,"DONE")) is_done=1;
                    if (!strcmp(strtmp,"READY")) is_ready=1;
                    if (!strncmp(strtmp,"PRHWERR",7)) return(95);
                    strtmp[0] = '\0';
                    y=0;
                }
            }
        }
    }
    return((is_ready)?((is_done)? 0 : 70) : 71);
}

int net_v2_recv_ready() {
    int rc=0;
    int is_ready=0;
    int is_done=0;
    char strtmp[MAX_STRING_LEN];

    while(!is_ready) {
        if((rc=net_v2_recv_cmd(0))) return(rc);
        if (net_fd->buf_len > NET_HDR_SIZE) {
	    sprintf(strtmp,"%s", net_fd->buf+NET_HDR_SIZE);
            add_log(__func__, LOG_DEBUG,"rc< (%d) \"%s\"",net_fd->buf_len, strtmp);
            if (!strcmp(strtmp,"DONE")) is_done=1;
            if (!strcmp(strtmp,"READY")) is_ready=1;
            if (!strncmp(strtmp,"PRHWERR",7)) return(95);
        }
    }
    return((is_ready)?((is_done)? 0 : 70) : 71);
}

int net_parse_cmd() {
    int rc=0;
    cmd_lst_t *cmd_tmp=NULL;
    cmd_lst_t *cmd_tek=NULL;
    uint16_t cmd_len=0;
    uint16_t crc=0;
    int x=0,y=0;
    char strtmp[MAX_STRING_LEN];

    /* clean cmd list */
    for(cmd_tmp=net_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) free(cmd_tmp);
    net_fd->cmd_lst=NULL;

    if (!net_fd->buf[0]) return(0);
    add_log(__func__, LOG_DEBUG,"r< %d bytes",net_fd->buf_len);
    if (net_fd->buf[0] == 253) {
        for(x=0;x<net_fd->buf_len;x++) {
            strtmp[y++] = net_fd->buf[x];
            if (net_fd->buf[x] == 254) {
                strtmp[y] = '\0';
                if ( (hw_use_crc) && (x<net_fd->buf_len) && (net_fd->buf[x+1] != 253) ) {
                    memcpy(&crc,net_fd->buf+x,2);
                    x+=2;
                }
                cmd_len = (uint8_t)strtmp[y-2];
                memcpy(strtmp, strtmp+1,cmd_len-1);
                strtmp[cmd_len-1] = '\0';
                if (!strcmp(strtmp,"CSIN1")) hw_use_crc=1;
                else if (!strcmp(strtmp,"CSIN0")) hw_use_crc=0;
                else if (!strncmp(strtmp,"PRHWERR",7)) return(95);
                add_log(__func__, LOG_DEBUG,"rc< (%d|%d) \"%s\"",cmd_len-1, strlen(strtmp), strtmp);

                if (!(cmd_tmp=(struct cmd_lst_s *)calloc(sizeof(struct cmd_lst_s),1))) return(87);
                cmd_tmp->str = xstrcpy(strtmp);
                cmd_tmp->ctime = time(NULL);
                if (!net_fd->cmd_lst) {
                     net_fd->cmd_lst=cmd_tmp;
                } else {
                    for(cmd_tek=net_fd->cmd_lst;cmd_tek->next;cmd_tek=cmd_tek->next);
                    cmd_tek->next=cmd_tmp;
                    cmd_tek=NULL;
                }
                cmd_tmp=NULL;

                strtmp[0] = '\0';
                y=0;
            }
        }
        return(0);
    } else {
        add_log(__func__, LOG_DEBUG,"rc< (%d) \"%s\"",net_fd->buf_len, net_fd->buf);
        if (!strcasecmp("QUIT",(char *)net_fd->buf) || !strcasecmp("EXIT", (char *)net_fd->buf)) {
            kill(getpid(),SIGINT);
        } else if (!strcasecmp("PING",(char *)net_fd->buf)) {
            if ((rc=net_send_cmd("PONG"))) return(rc);
        } else {
            return(0);
        }
    }
    return(1);
}

int net_wait_session(char *ip, char *port) {
    int rc=0;
    int in_len=0;
    char strtmp[MAX_STRING_LEN];
    cmd_lst_t *cmd_tmp=NULL;

    if (net_sock_new(&net_fd, ip, port)) return(79);

    if (net_fd->peer_host) {
        if ((rc = hw_sock_new(&hw_fd))) {
            net_send_cmd("PRMEMERR");
            return(rc);
        }
        hw_fd->device=xstrcpy(pr_device);

        if ((rc=net_recv_buf(net_fd))) return(rc);
        if (!strcmp("PROTOM",(char *)net_fd->buf)) {
	    net_fd->proto=NET_PROTO_M;
        } else if (!strcmp("PROTO2",(char *)net_fd->buf)) {
	    net_fd->proto=NET_PROTO_V2;
        } else if (!strcmp("PROTOT",(char *)net_fd->buf)) {
            net_fd->proto=NET_PROTO_T;
        } else if (!strcmp("PROTOD",(char *)net_fd->buf)) {
            net_fd->proto=NET_PROTO_D;
        } else {
            strtmp[0]='\0';
            for(rc=0;rc<net_fd->buf_len;rc++)
                sprintf(strtmp,"%s [%d]",strtmp,net_fd->buf[rc]);
            add_log(__func__, LOG_DEBUG,"[debug] %s", strtmp);
            return(88);
        }
        add_log(__func__, LOG_DEBUG,"rc< PROTO=\"%s(proto=%d)\"", net_fd->buf,net_fd->proto);

        if (net_fd->proto==NET_PROTO_D) {
            rc = hw_datecs_connect(hw_fd, HW_DATECS_DEFAULT_BAUDRATE);
        } else {
            rc = hw_maria301_connect(hw_fd);
        }
        if (rc) {
            sprintf(strtmp,"PRHWERR%d",rc);
            net_send_cmd(strtmp);
            return(rc);
        }

        if (net_fd->proto == NET_PROTO_M) {
            memcpy(net_fd->buf,hw_fd->buf,sizeof(net_fd->buf));
            for (cmd_tmp=hw_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next)
                add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",strlen(cmd_tmp->str), cmd_tmp->str);
            cmd_tmp=NULL;
            if ((rc=net_send_buf(net_fd))) return(rc);
        } else if (net_fd->proto == NET_PROTO_V2) {
            for (cmd_tmp=hw_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) {
                add_log(__func__, LOG_DEBUG,"s> [p=%d] (%d) \"%s\"",net_fd->proto, strlen(cmd_tmp->str), cmd_tmp->str);
                if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD, cmd_tmp->str))) return(rc);
            }
            cmd_tmp=NULL;
        } else if (net_fd->proto == NET_PROTO_D) {
            if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD, "READY"))) return(rc);
        } else {
            for (cmd_tmp=hw_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) {
                add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",strlen(cmd_tmp->str), cmd_tmp->str);
                memset(net_fd->buf,0,sizeof(net_fd->buf));
                sprintf((char *)net_fd->buf,"%s\r\n",cmd_tmp->str);
                if ((rc=net_send_buf(net_fd))) return(rc);
            }
            cmd_tmp=NULL;
        }
    } else {
        if((rc=net_make_connect(net_fd))) return(rc);
        printf(" > PROTOD\n");
        if ((rc=net_send_cmd("PROTOD"))) return(rc);
//        if ((rc=net_send_cmd("PROTO2"))) return(rc);
        printf("?< READY\n");
        if ((rc=net_v2_recv_ready()) && (rc!=70)) return(rc);
        printf(" < READY\n");
/*
        if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,"CSIN1"))) return(rc);
        if ((rc=net_v2_recv_ready())) return(rc);
        if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,"UPAS1111111111111111111"))) return(rc);
        if ((rc=net_v2_recv_ready())) return(rc);
        if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,"NULL"))) return(rc);
        if ((rc=net_v2_recv_ready())) return(rc);
        if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,"NULL"))) return(rc);
        if ((rc=net_v2_recv_ready())) return(rc);
        if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,"NULL"))) return(rc);
        if ((rc=net_v2_recv_ready())) return(rc);
*/
//	sprintf(strtmp,"%c000000;",0xf3);
//	if ((rc=net_datecs_send_cmd(strtmp))) return(rc);
//	if ((rc=net_v2_recv_cmd(0))) return(rc);

       while(1) {
	sprintf(strtmp,"%c000000;0;1; ;",0x81);
	if ((rc=net_datecs_send_cmd(strtmp))) return(rc);
        if ((rc=net_v2_recv_cmd(0))) return(rc);
usleep(500000);
//	sprintf(strtmp,"%c000000;1;0;Pidarnaya xuyna!;",0x81);
	sprintf(strtmp,"%c000000;1;0;ololo! Trololo! \234;",0x81);
	if ((rc=net_datecs_send_cmd(strtmp))) return(rc);
        if ((rc=net_v2_recv_cmd(0))) return(rc);
//sleep(1);
usleep(500000);
	}

/*
        sleep(5);
        while(1) {
            sleep(1);
            printf("sleep\n");
        }*/
        return(0);
    }

    while(1) {
	if (net_fd->proto == NET_PROTO_D) {
	    if ((rc=net_v2_recv_cmd(0))) return(rc);
	    in_len=net_fd->buf[0] + net_fd->buf[1] * 256;
	    if (in_len > NET_HDR_SIZE) {
		memcpy(hw_fd->buf, net_fd->buf+NET_HDR_SIZE, (in_len - NET_HDR_SIZE));
		hw_fd->buf_len = in_len - NET_HDR_SIZE;
		if ((rc=hw_datecs_send_buf(hw_fd))) return(rc);
		if ((rc=hw_datecs_recv_cmd())) return(rc);
		if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,xstrcpy((char *)hw_fd->buf)))) return(rc);
	    }
	} else {
	    if (!(hw_fd->state & HW_STATE_IS_READY) && !(hw_fd->state & HW_STATE_IS_SOFTBLOCK)) {
		if ((rc = hw_maria301_recv_cmd())) return(rc);
		if (hw_fd->cmd_lst) {
		    for (cmd_tmp=hw_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) {
			if (net_fd->proto == NET_PROTO_M) {
			    sprintf(strtmp,"%s",cmd_tmp->str);
			    if ((rc=net_send_cmd(strtmp))) return(rc);
			} else if (net_fd->proto == NET_PROTO_V2) {
			    if ((rc=net_v2_send_cmd(NET_PRT_SEND_CMD,cmd_tmp->str))) return(rc);
			} else {
			    add_log(__func__, LOG_DEBUG,"s> (%d) \"%s\"",strlen(cmd_tmp->str), cmd_tmp->str);
			    memset(net_fd->buf,0,sizeof(net_fd->buf));
			    sprintf((char *)net_fd->buf,"%s\r\n",cmd_tmp->str);
			    if ((rc=net_send_buf(net_fd))) return(rc);
			}
		    }
		    cmd_tmp=NULL;
		}
	    } else {
		if (net_fd->proto == NET_PROTO_M) {
		    if ((rc=net_recv_buf(net_fd))) return(rc);
		    if(net_fd->buf[0] == 253) {
			net_parse_cmd();
			if (net_fd->cmd_lst) {
			    for (cmd_tmp=net_fd->cmd_lst;cmd_tmp;cmd_tmp=cmd_tmp->next) {
				sprintf(strtmp,"%s",cmd_tmp->str);
				if ((rc=hw_maria301_send_cmd(strtmp))) return(rc);
			    }
			    cmd_tmp=NULL;
			}
		    }
		} else if (net_fd->proto == NET_PROTO_V2) {
		    if ((rc=net_v2_recv_cmd(0))) return(rc);
		    in_len=net_fd->buf[0] + net_fd->buf[1] * 256;
		    if (in_len > NET_HDR_SIZE) {
			if ((rc=hw_maria301_send_cmd(xstrcpy((char *)(net_fd->buf+NET_HDR_SIZE))))) return(rc);
		    }
		} else {
		    if ((rc=net_recv_buf(net_fd))) return(rc);
		    if (net_fd->buf[0]) {
			if (!net_parse_cmd()) {
			    sprintf(strtmp,"%s",net_fd->buf);
			    if ((rc=hw_maria301_send_cmd(strtmp))) return(rc);
			}
		    }
		}
	    }
        }
    }
    return(255);
}
