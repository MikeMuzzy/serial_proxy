/* 
 * File:   main.h
 * Author: muzzy
 *
 * Created on May 15, 2009, 3:35 PM
 */

#ifndef _MAIN_H
#define	_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h> // va_list
#include <signal.h>
#include <libgen.h> /* basename() */
#include <syslog.h>

#include "net_prot.h"
#include "hw_prot.h"
#include "util.h"

#define CONF_FILE "/etc/serial_proxy.conf"

#define LOG_LEVEL 9
#define REM_LOG_FILE "/tmp/serial_proxy_rem.log"
#define LOCAL_LOG_FILE "/var/log/serial_proxy.log"

extern char *ag_log_file;
extern char *sp_main_name;

int checkpid(char *pidname);

int add_log(const char *func_name, int level, char *msg, ...);

extern int proc_pid;

#endif	/* _MAIN_H */

