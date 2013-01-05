 
#ifndef __AT89FLASH_CMD
#define __AT89FLASH_CMD

#include "at89flash.h"

typedef struct {
    char **argv;
    unsigned char baud : 1;
    unsigned char blank : 1;
    unsigned char eeprom : 1;
    unsigned char erase : 1;
    unsigned char help : 1;
    unsigned char ihex : 1;
    unsigned char modify : 1;
    unsigned char port : 1;
    unsigned char watchdog : 1;
    unsigned char noverify : 1;
    unsigned char read : 1;
    unsigned char readbin : 1;
    unsigned char set : 1;
    unsigned char start : 1;
    unsigned char stopbit : 1;
    unsigned char verify : 1;
    unsigned char verifybin : 1;
    unsigned char write : 1;
    unsigned char writebin : 1;
    char *baud_val;
    char *modify_addr;
    char *modify_data;
    char *port_dev;
    int set_idx;
    int set_cnt;
    char *start_addr;
    char *stopbit_mode;
    char *msg;
} AT89FLASH_CMD;

char cmd_chkopt(int argc, char **argv, int *idx, char opt);
AT89FLASH_STATUS cmd_parse(int argc, char **argv, AT89FLASH_CMD *cmd);
void cmd_help();

#endif
