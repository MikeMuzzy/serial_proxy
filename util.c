#include "util.h"

#define LF 10
#define CR 13

/* ============================================== */
int getword(char *word, char *line, char stop) {
    int x = 0,y,l;

    for(x=0;((line[x]) && (line[x] != stop));x++)
        word[x] = line[x];

    word[x] = '\0';
    l=x;

    if(line[x]) ++x;
    y=0;

    while((line[y++] = line[x++]));
    return(l); // Word len
}
/* ============================================== */
int my_getline(char *s, int n, FILE *f) {
    register int i=0;
    char rt='\0';

    while(1) {
        s[i] = '\0';
        rt = (char)fgetc(f);
        if(feof(f)) return(i?0:1);
        if(rt == CR) {
            rt = (char)fgetc(f);
            if(feof(f)) return(0);
        }
        if((rt == 0x4) || (rt == LF) || (i == (n-1))) {
            return (0);
        }
        s[i] = rt;
        ++i;
    }
}
/* ============================================== */
int ind(char *s, char c) {
    register int x;

    for(x=0;s[x];x++)
        if(s[x] == c) return x;

    return -1;
}
/* ============================================== */
int rind(char *s, char c) {
    register int x;
    for(x=strlen(s) - 1;x != -1; x--)
        if(s[x] == c) return x;
    return(-1);
}
/* ========================== xutil ========================= */
void xabort(void)
{
   printf("No memory for xutil.\n");
}

char *xmalloc(size)
size_t size;
{
	char *tmp;
	tmp=malloc(size);
	if (!tmp) xabort();

	return tmp;
}

char *xstrcpy(src)
char *src;
{
	char	*tmp;

	if ((src == NULL) || (*src=='\0')) return(NULL);
	tmp=xmalloc(strlen(src)+1);
	strcpy(tmp,src);
	return tmp;
}

unsigned int getCRC16(void *mem, unsigned int len) {
    unsigned int a, crc;
    unsigned char *pch;
    pch=(uint8_t *)mem;
    crc=0;
    while(len--) {
        crc^=*pch;
        a=(crc^(crc<<4))&0x00FF;
        crc=(crc>>8)^(a<<8)^(a<<3)^(a>>4);
        pch+=1;
    }
    return(crc);
}
