/* 
 * File:   main.c
 * Author: muzzy
 *
 * Created on May 15, 2009, 3:34 PM
 */

#include "main.h"
#include "version.h"

char *sp_main_name=NULL;
int proc_pid = 0;

void interrupted() {
    int rc=0;
    add_log(__func__, LOG_NOTICE,"interrupted. shutdown.");
    if (hw_fd) {
        if (net_fd->proto==NET_PROTO_D) {
            rc = hw_datecs_disconnect(hw_fd);
            add_log(__func__, LOG_DEBUG ,"hw_datecs_disconnect=%d", rc);
        } else {
            rc = hw_maria301_disconnect(hw_fd);
            add_log(__func__, LOG_DEBUG ,"hw_maria301_disconnect=%d", rc);
        }
        _exit(0);
    } else
        _exit(0);
}

int net_call_remote(char *inarg) {
    int rc=0;
    char strtmp[MAX_STRING_LEN];
    char strtm1[MAX_STRING_LEN];

    if((!inarg) || (!inarg[0])) return(0);

    ag_log_file=REM_LOG_FILE;

    sprintf(strtmp,"%s",inarg);
    getword(strtm1,strtmp,':');
    net_rem_host=xstrcpy(strtm1);
    net_rem_port=xstrcpy(strtmp);

    if(!net_rem_host || !net_rem_port) {
        printf("ERROR! dont set host:port addr\n");
        return(1);
    }

    if((rc=net_wait_session(net_rem_host, net_rem_port))) {
        if(rc != 255) printf("Finished with: %d\n", rc);
    }

    return(1);
}

int get_conf() {
    FILE *cf=NULL;
    char strtmp[MAX_STRING_LEN];
    char strtm1[MAX_STRING_LEN];

    if (!(cf=fopen(CONF_FILE,"r"))) return(1);
    while(!my_getline(strtmp, MAX_STRING_LEN,cf)) {
        if ((strtmp[0] == '\0') || (strtmp[0] == '#') || (strtmp[0] == ';')) continue;
        getword(strtm1,strtmp,' ');
        if(!strcmp("device",strtm1))
            pr_device = xstrcpy(strtmp);
    }
    fclose(cf);
    return(0);
}

int main(int argc, char** argv) {
    int rc=0;
    char strtmp[MAX_STRING_LEN];

    proc_pid = getpid();
    ag_log_file=LOCAL_LOG_FILE;

    add_log(__func__, LOG_INFO, "serial_proxy version %s/%s started", SVN_REVISION, COMPILE_TIME);

    signal(SIGINT,(void (*)())interrupted);
    signal(SIGTERM,(void (*)())interrupted);
    signal(SIGQUIT,(void (*)())interrupted);
    signal(SIGHUP,(void (*)())interrupted);
    //signal(SIGALRM, &sig_alarm);

    if(net_sock_new(&net_fd, NULL, NULL)) {
        fprintf(stderr, "Can't make Sock structure.\n");
        return(0);
    }

    if(net_get_peer(net_fd)) {
        fprintf(stderr, "Problems with getting peer host.\n");
        return(0);
    }

    if(!net_fd->peer_host) {
        if(argc>1) {
	    if (strcmp("detect", argv[1]) == 0) {
		return(hw_detect_device(argv[2], argv[3]));
	    } else if (strcmp("version", argv[1]) == 0) {
		printf("Version: %s\nCompiled: %s\n", SVN_REVISION, COMPILE_TIME);
		return(0);
	    } else {
		if(net_call_remote(argv[1])) return(0);
	    }
        }
        printf("This util should be running as INet daemon.\n");
        return(0);
    }

    add_log(__func__, LOG_INFO, "--- Connect from: %s", net_fd->peer_host);

    sp_main_name=xstrcpy(basename(argv[0]));
    sprintf(strtmp,"%s",sp_main_name);
//    if ((rc=checkpid(strtmp)) && (rc!=getpid())) {
    if ((rc=checkpid(strtmp)) && (rc!=proc_pid)) {
        kill(rc,SIGINT);
        sleep(3);
    }


    if (get_conf()) return(5);
    //pr_device=SERIAL_DEVICE;
    //alarm(70);
    if((rc=net_wait_session(NULL,NULL))) {
        if(rc != 255) {
            if(net_fd->str_error[0]) {
                add_log(__func__, LOG_ALERT, "ERROR! %s", net_fd->str_error);
            }
            if(hw_fd->str_error[0]) {
                if (net_fd->peer_host) {
                    sprintf(strtmp,"PRHWERR%d",rc);
                    net_send_cmd(strtmp);
                }
                add_log(__func__, LOG_ALERT, "ERROR! %s", hw_fd->str_error);
		if (net_fd->proto==NET_PROTO_D) {
		    add_log(__func__, LOG_DEBUG ,"hw_datecs_close_connect=%d", hw_datecs_disconnect(hw_fd));
		} else {
		    add_log(__func__, LOG_DEBUG ,"setIoctlParam(TIOCM_DTR)=%d", setIoctlParam(hw_fd, TIOCM_DTR, 0));
		    add_log(__func__, LOG_DEBUG ,"hw_close_connect=%d", hw_maria301_disconnect(hw_fd));
		}
            }
        }
        add_log(__func__, LOG_INFO , "--- Finished with: %d", rc);
    }
    return(rc);
}
