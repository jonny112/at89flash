/*
 * at89flash/util.c
 * Various conversion and interface functions
 *
 * J. Schmitz, 2011
 *
 */

#include "util.h"
#include <errno.h>


// scan for command line argument
int getcmd(int argc, char *argv[], char *q, int offset) {
  int n;
  for (n = 1; n < argc; n++) {
    if (strcmp(argv[n], q) == 0) {
      if (offset < 1) { 
	return n;
      } else {
	offset--; 
      }
    }
  }
  return 0;
}


// read from stream to buffer till newline or timeout
int readto(int strm, char *buff, int nmax, int mstime) {
  int bpos = 0, nread;
  while (mstime > 0) {
    nread = read(strm, &buff[bpos], nmax - bpos);
    if (nread > 0) bpos += nread;
    if (bpos >= nmax || (bpos > 0 && buff[bpos - 1] == '\n')) return bpos;
    usleep(1000);
    mstime--;
  }
  return 0;
}


int readto_async(int strm, char *buff, int *bpos, int nmax) {
  int nread;
  nread = read(strm, &buff[*bpos], nmax - *bpos);
  if (nread > 0) *bpos += nread;
  if (*bpos >= nmax || (*bpos > 0 && buff[*bpos - 1] == '\n')) return 0;
  else return -1;
}


// convert byte to bin string
char *byte2bin(char in, char *out) {
  int n;
  for (n = 0; n < 8; n++) out[n] = ((in & (1 << (7 - n))) == 0 ? '0' : '1');
  out[8] = '\0';
  return out;
}


void spinbar(FILE *fd, char *pos) {
  char c;
  if (pos == 0) {
    fprintf(fd, "\b\b");  
    return;
  } else if (*pos == 0) fprintf(fd, "  ");
  else if (*pos > 3) *pos = 0;
  (*pos)++;
  switch (*pos) {
    case 1: c = '-'; break;
    case 2: c = '\\'; break;
    case 3: c = '|'; break;
    case 4: c = '/'; break;
  }
  fprintf(fd, "\b\b%c ", c);
}


void progbar(FILE *fd, float q) {
  int n;
  char bar[33];
  bar[32] = '\0';
  if (q <= 1) {
    memset(bar, '\b', sizeof(bar) - 1);
    fprintf(fd, "%s", bar);
  } else q = 0;
  if (q >= 0) {
    bar[0] = '[';
    for (n = 1; n <= 24; n++) bar[n] = ((q * 24) >= n ? '=' : ' ');
    sprintf(&bar[25], "] %3d%% ", (int)(q * 100));
  } else {
    memset(bar, ' ', sizeof(bar) - 1);
    fprintf(fd, "%s", bar);
    memset(bar, '\b', sizeof(bar) - 1);
  }
  fprintf(fd, "%s", bar);
}
