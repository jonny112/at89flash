
#ifndef __IHEX_H
#define __IHEX_H

#include <stdint.h>

typedef struct {
  uint8_t len;
  uint16_t addr;
  uint8_t type;
  uint8_t *data;
  uint8_t cksum;
} IHEX_RECORD;

int ihex_parse(char *in, IHEX_RECORD *out);
char *ihex_string(IHEX_RECORD *in, char *out);
uint8_t ihex_cksum(IHEX_RECORD *in);
char hex2int8(char *in, uint8_t *out);
char hex2int16(char *in, uint16_t *out);

#endif
