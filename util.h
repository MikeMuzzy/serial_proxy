/* 
 * File:   util.h
 * Author: muzzy
 *
 * Created on May 15, 2009, 3:44 PM
 */

#ifndef _UTIL_H
#define _UTIL_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN 4096
#endif

int getword(char *word, char *line, char stop);
int my_getline(char *s, int n, FILE *f);
int ind(char *s, char c);
int rind(char *s, char c);

/* xutil */

void xabort(void);
char *xmalloc(size_t);
char *xstrcpy(char *);

unsigned int getCRC16(void *mem, unsigned int len);

#endif	/* _UTIL_H */
