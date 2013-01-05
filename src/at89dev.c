/*
 * at89flash/at89dev.c:
 * Communications functions for the Atmel 80C51 UART bootloader
 * 
 * J. Schmitz, 2011
 * 
 */

#include "at89dev.h"


// write Intel-Hex record to serial port
int ihex_send(int serport, IHEX_RECORD *rec) {
  char *buff;
  char *ihstr;
  int nread, nrslt;
  int len = rec->len * 2 + 11;
  
  rec->cksum = ihex_cksum(rec);
  ihstr = malloc(len + 1);
  tcflush(serport, TCIOFLUSH);
  write(serport, ihex_string(rec, ihstr), len);
  tcdrain(serport);
  
  buff = malloc(len);
  nread = readto(serport, buff, len, 3000);
  if (nread < len) nrslt = 0;
  else if (memcmp(buff, ihstr, len) == 0) nrslt = len;
  else nrslt = -1;
  
  free(ihstr);
  free(buff);
  return nrslt;
}


// synchronize device
int dev_sync(int serport) {
  int ntimer;
  char nread;
  char csync = 'U';
  ntimer = 0; nread = 0;
  do {
    if (ntimer % 10 == 0) write(serport, &csync, 1);
    usleep(100000);
    nread = read(serport, &csync, 1);
    ntimer++;
  } while (nread < 1 && ntimer < 50);
  
  if (nread < 1) return -1;
  else if (csync != 'U') return 1;
  else return 0;
}


// read byte value or error status
int dev_getval(int serport, uint8_t *val) {
  char buff[5];
  int nread;
  nread = readto(serport, buff, sizeof(buff), 3000);
  if (nread < 2) return -1;
  else if (buff[0] == '.') return AT89_ERR_OK;
  else if (buff[0] == 'X') return AT89_ERR_CKSUM;
  else if (buff[0] == 'P' || buff[0] == 'L') return AT89_ERR_SECU;
  else {
    if (val != 0) return hex2int8(buff, val);
    else return 0;
  }
}


// read config byte (0x100 if forbidden)
int dev_getconf(int serport, char *data, uint16_t *nval) {
  IHEX_RECORD ih;
  uint8_t val;
  int nrslt;
  
  ih.type = 5;
  ih.addr = 0;
  ih.len = 2;
  ih.data = (uint8_t*)data;
  
  if (ihex_send(serport, &ih) > 0) {
    nrslt = dev_getval(serport, &val);
    if (nrslt == AT89_ERR_SECU) {
      *nval = 0x100;
    } else  {
      if (nrslt != 0) return -1;
      *nval = val;
    }
    return 0;
  } else return -1;
}


// set page containing address to be written
void dev_setpage(AT89_MEM *mem, uint16_t addr) {
  uint16_t npage = addr / 128;
  mem->pages[npage / 8] &= ~(1 << (npage % 8));
}


// check if page is to be written
char dev_getpage(AT89_MEM *mem, uint16_t addr) {
  uint16_t npage = addr / 128;
  return ((mem->pages[npage / 8] & (1 << (npage % 8))) == 0 ? 1 : 0);
}


// count pages to be written
uint16_t dev_pagecnt(AT89_MEM *mem) {
  uint16_t ncnt = 0, npage;
  for (npage = 0; npage < 512; npage++) if ((mem->pages[npage / 8] & (1 << (npage % 8))) == 0) ncnt++;
  return ncnt;
}


// initiate data transfer from device
int dev_initread(int serport, uint16_t naddr_start, uint16_t naddr_end, char eeprom) {
  int nread;
  uint8_t dat[5];
  char buff[2];
  IHEX_RECORD ih;
  ih.type = 4; ih.addr = 0; ih.len = 5; ih.data = dat;
  dat[0] = (naddr_start >> 8) & 0xFF;
  dat[1] = naddr_start & 0xFF;
  dat[2] = (naddr_end >> 8) & 0xFF;
  dat[3] = naddr_end & 0xFF;
  dat[4] = (eeprom ? 0x02 : 0x00);
  if (ihex_send(serport, &ih) > 0) {
    nread = readto(serport, buff, 2, 10000);
    if (nread < 2) return AT89_ERR_TIMEOUT;
    else if (buff[0] == 'L') return AT89_ERR_SECU;
    else if (buff[0] == 'X') return AT89_ERR_CKSUM;
    else if (buff[0] == '\r' && buff[1] == '\n') return AT89_ERR_OK;
    else return 0;
  } return 0;
}


// read device data to virtual memory
int dev_getdata(int serport, AT89_MEM *mem) {
  char buff[39];
  uint16_t naddr;
  uint8_t npos = 0;
  if (readto(serport, buff, 39, 10000) > 5) {
    if (hex2int16(buff, &naddr) < 0) return -1;
    while (npos < 16) if (hex2int8(&buff[5 + 2 * npos], &mem->data[naddr + npos]) < 0) break; else npos++;
    return npos;
  } else return -1;
}


// generate Intel-Hex from virtual memory
void mem2ihex(FILE *fd, AT89_MEM *mem, uint16_t nmaxaddr) {
  IHEX_RECORD ih;
  char ihstr[44];
  int naddr;
  ih.type = 0; ih.len = 16;
  for (naddr = 0; naddr < nmaxaddr; naddr += ih.len){
    ih.addr = naddr;
    ih.data = &mem->data[naddr];
    ih.cksum = ihex_cksum(&ih);
    fprintf(fd, "%s\n", ihex_string(&ih, ihstr));
  }
  ih.type = 1; ih.addr = 0; ih.len = 0; ih.cksum = ihex_cksum(&ih);
  fprintf(fd, "%s\n", ihex_string(&ih, ihstr));
}


// read ihex to virtual device memory
int ihex2mem(FILE *fd, AT89_MEM *mem, int nmaxaddr, int *nline) {
  char buff[1024], cend = 0;
  int err = 0;
  IHEX_RECORD ih;
  *nline = 0;
  
  while (!feof(fd) && !cend && !err) {
    (*nline)++;
    fgets(buff, sizeof(buff) - 1, fd);
    if (buff[0] == ':') {
      if (ihex_parse(buff, &ih) == 0) {
	if (ih.type == 0) {
	  if (ih.addr + ih.len <= nmaxaddr + 1) {
	    memcpy(&mem->data[ih.addr], ih.data, ih.len);
	    dev_setpage(mem, ih.addr);
	    dev_setpage(mem, ih.addr + ih.len);
	  } else err = AT89_ERR_MEM;
	} else if (ih.type == 1) cend = 1;
	else err = AT89_ERR_BADREC;
      } else err = AT89_ERR_PARSE;
      if (ih.data != 0) free(ih.data);
    }
  }
  
  if (!cend && !err) err = AT89_ERR_NOENDREC;
  return err;
}

