
#ifndef __AT98FLASH_MAIN
#define __AT98FLASH_MAIN

#include "at89flash.h"
#include "util.h"
#include "cmd.h"

typedef struct {
  AT89FLASH_STATUS err;
  AT89FLASH_CMD cmd;
  speed_t baud;
  int nbaud;
  int nmaxaddr;
  int serport;
  TERMIOS termios_old;
  TERMIOS termios_new;
} AT89FLASH_GLOBALS;

#endif
