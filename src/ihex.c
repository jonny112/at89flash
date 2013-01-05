/*
 * at89flash/ihex.c
 * Intel-Hex parsing and conversion functions
 *
 * J.Schmitz, 2011
 *
 */

#include "ihex.h"
#include <stdlib.h>
#include <stdio.h>


uint8_t hexnibble(char c) {
  switch (c) {
    case '0': return 0x00; break;
    case '1': return 0x01; break;
    case '2': return 0x02; break;
    case '3': return 0x03; break;
    case '4': return 0x04; break;
    case '5': return 0x05; break;
    case '6': return 0x06; break;
    case '7': return 0x07; break;
    case '8': return 0x08; break;
    case '9': return 0x09; break;
    case 'a': return 0x0A; break;
    case 'A': return 0x0A; break;
    case 'b': return 0x0B; break;
    case 'B': return 0x0B; break;
    case 'c': return 0x0C; break;
    case 'C': return 0x0C; break;
    case 'd': return 0x0D; break;
    case 'D': return 0x0D; break;
    case 'e': return 0x0E; break;
    case 'E': return 0x0E; break;
    case 'f': return 0x0F; break;
    case 'F': return 0x0F; break;
    default: return 0x10; break;
  }
}


char hex2int8(char *in, uint8_t *out) {
  uint8_t dat;
  int pos = 0;
  *out = 0;
  while (pos < 2) {
    if ((dat = hexnibble(in[pos])) > 0xF) break;
    *out |= dat << ((1 - pos) * 4);
    pos++;
  }
  return ((pos < 2) ? -1 : 0);
}


char hex2int16(char *in, uint16_t *out) {
  uint8_t dat;
  int pos = 0;
  *out = 0;
  while (pos < 2) {
    if (hex2int8(&in[pos * 2], &dat) != 0) break;
    *out |= dat << ((1 - pos) * 8);
    pos++;
  }
  return ((pos < 2) ? -1 : 0);
}


int ihex_parse(char *str, IHEX_RECORD *rec) {
  int pos = 0;
  int datpos;
  rec->data = 0;
  
  if (str[pos] != ':') return (pos + 1);
  pos = 1;
  if (hex2int8(&str[pos], &rec->len) != 0) return (pos + 1);
  pos = 3;
  if (hex2int16(&str[pos], &rec->addr) != 0) return (pos + 1);
  pos = 7;
  if (hex2int8(&str[pos], &rec->type) != 0) return (pos + 1);
  pos = 9;
  rec->data = malloc(rec->len);
  datpos = rec->len;
  while (datpos > 0) {
    if (hex2int8(&str[pos], &rec->data[rec->len - datpos]) != 0) return (pos + 1);
    pos += 2;
    datpos--;
  }
  if (hex2int8(&str[pos], &rec->cksum) != 0) return (pos + 1);
  return 0;
}


char *ihex_string(IHEX_RECORD *rec, char *str) {
  int pos;
  sprintf(&str[0], ":%.2X%.4X%.2X", rec->len, rec->addr, rec->type);
  for (pos = 0; pos < rec->len; pos++) sprintf(&str[9 + 2 * pos], "%.2X", rec->data[pos]);
  sprintf(&str[9 + 2 * pos], "%.2X", rec->cksum);
  return str;
}


uint8_t ihex_cksum(IHEX_RECORD *rec) {
  uint8_t sum = rec->len + (rec->addr >> 8) + (rec->addr & 0xFF) + rec->type;
  int n; for (n = 0; n < rec->len; n++) sum += rec->data[n];
  return ((sum ^ 0xFF) + 1);
}
