
#ifndef __AT98DEV
#define __AT98DEV

#include "util.h"
#include "ihex.h"

#define AT89_ERR_TIMEOUT -1
#define AT89_ERR_CKSUM -2
#define AT89_ERR_SECU -3
#define AT89_ERR_PARSE -4
#define AT89_ERR_MEM -5
#define AT89_ERR_BADREC -6
#define AT89_ERR_NOENDREC -7
#define AT89_ERR_OK -128

#define AT89_CONF_MFC "\x00\x00"
#define AT89_CONF_FMC "\x00\x01"
#define AT89_CONF_PDN "\x00\x02"
#define AT89_CONF_PDR "\x00\x03"
#define AT89_CONF_SSB "\x07\x00"
#define AT89_CONF_BSB "\x07\x01"
#define AT89_CONF_SBV "\x07\x02"
#define AT89_CONF_EB "\x07\x06"
#define AT89_CONF_HSB "\x0B\x00"
#define AT89_CONF_ID1 "\x0E\x00"
#define AT89_CONF_ID2 "\x0E\x01"
#define AT89_CONF_BLV "\x0F\x00"

typedef struct {
  uint8_t pages[64];
  uint8_t data[0x10000];
} AT89_MEM;

int dev_sync(int serport);
int ihex_send(int serport, IHEX_RECORD *rec);
int dev_getval(int serport, uint8_t *val);
int dev_getconf(int serport, char *data, uint16_t *nval);
int dev_initread(int serport, uint16_t naddr_start, uint16_t naddr_end, char eeprom);
int dev_getdata(int serport, AT89_MEM *mem);
int ihex2mem(FILE *fd, AT89_MEM *mem, int nmaddr, int *nline);
void mem2ihex(FILE *fd, AT89_MEM *mem, uint16_t nmaxaddr);
void dev_setpage(AT89_MEM *mem, uint16_t addr);
char dev_getpage(AT89_MEM *mem, uint16_t addr);
uint16_t dev_pagecnt(AT89_MEM *mem);

#endif
