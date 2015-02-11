#include <sys/time.h>
#include "main.h"

char *ag_log_file=NULL;

int add_log(const char *func_name, int level, char *msg, ...) {
    va_list lm;
    FILE *fp=NULL;
    struct timeval tv;
    struct tm *ltm=NULL;
    char strmsg[MAX_STRING_LEN];

    if(level > LOG_LEVEL) return(0);

    if(msg) {
	va_start(lm, msg);
	vsprintf(strmsg, msg, lm);
	va_end(lm);
    } else {
	sprintf(strmsg, "-=MARK=-");
    }

    openlog(sp_main_name,LOG_PID,LOG_DAEMON);
    syslog(level | LOG_DAEMON, strmsg);
    closelog();

    if(!(fp=fopen(ag_log_file, "a"))) return(1);

    gettimeofday(&tv, NULL);
    ltm = localtime(&tv.tv_sec);

    fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d.%06lu [pid=%d] [%20s] %s\n",
	ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec ,tv.tv_usec,
	proc_pid, func_name, strmsg);
    return(fclose(fp));
}
