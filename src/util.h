
#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#define OPTS_ERR_INVALID -1
#define OPTS_ERR_NOMULTI -2
#define OPTS_ERR_FEWARGS -3
#define OPTS_ERR_NOAGR -4

#define OPTS_STR { "hhelp", "p", "b", "" }
#define OPTS_ARG { 0x00, 0x01, 0x01 }

typedef struct termios TERMIOS;

int getcmd(int argc, char *argv[], char *q, int offset);
int readto(int strm, char *buff, int nmax, int mstime);
int readto_async(int strm, char *buff, int *bpos, int nmax);
char *byte2bin(char in, char *out);
void spinbar(FILE *fd, char *pos);
void progbar(FILE *fd, float q);

#endif
