#include "main.h"
#include <dirent.h>

int checkpid(char *pidname) {
    DIR *p_dir=NULL;
    struct dirent *p_ent;
    FILE *pf=NULL;
    struct stat st;
    char strtmp[MAX_STRING_LEN];
    int pid=0;
    int err_no=0;

    if (!(p_dir = opendir("/proc/"))) {
        err_no=errno;
        add_log(__func__, LOG_WARNING,"can't open /proc: [%d]:%s", err_no, strerror(err_no));
        return(-1);
    }
    while((p_ent = readdir(p_dir))) {
        if (!p_ent->d_name) continue;
        if (!p_ent->d_name[0]) continue;
        if (!p_ent->d_name[0] == '.') continue;
        //if (!isdigit(p_ent->d_name[0])) continue;
        if ( !(p_ent->d_name[0] >= '0' && p_ent->d_name[0] <= '9') ) continue;
        sprintf(strtmp,"/proc/%s/cmdline",p_ent->d_name);
        if (stat(strtmp,&st)) continue;
        if (!(pf=fopen(strtmp,"r"))) continue;
        my_getline(strtmp,MAX_STRING_LEN,pf);
        if(!strncmp(pidname,strtmp,strlen(pidname))) {
            pid=atoi(p_ent->d_name);
            fclose(pf);
            break;
        }
        fclose(pf);
    }
    closedir(p_dir);
    return(pid);
}
