/*
 * at89flash/cmd.c:
 * Command line parsing functions
 *
 * J. Schmitz, 2012
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "cmd.h"


// check for option argument
char cmd_chkopt(int argc, char **argv, int *idx, char opt) {
  if (*idx < argc && argv[*idx][0] != '-') {
    (*idx)++;
    return 0;
  }
  else {
    if (opt != 0) fprintf(stderr, "\n\nMissing argument to -%c option!\n", opt);
    return 1;
  }
}


// parse command line
AT89FLASH_STATUS cmd_parse(int argc, char **argv, AT89FLASH_CMD *cmd) {
  int opt;
  static char opts[] = "+bBeEhimpPqrRsSTvVwW";
  static struct option long_opts[] = {
      { "help" , 0 , 0, 'h' },
      { 0, 0, 0, 0 }
  };

  memset(cmd, 0, sizeof(AT89FLASH_CMD));
  cmd->argv = argv;
  opterr = 0;
  while ((opt = getopt_long(argc, argv, opts, long_opts, 0)) != -1) {
    switch (opt) {
      case 'b':
        cmd->baud = 1;
        cmd->baud_val = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        break;

      case 'B':
        cmd->blank = 1;
        break;

      case 'e':
        cmd->eeprom = 1;
        break;

      case 'E':
        cmd->erase = 1;
        break;

      case 'h':
        cmd->help = 1;
        break;

      case 'i':
        cmd->ihex = 1;
        break;

      case 'm':
        cmd->modify = 1;
        cmd->modify_addr = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        cmd->modify_data = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        break;

      case 'p':
        cmd->port = 1;
        cmd->port_dev = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        break;

      case 'P':
        cmd->watchdog = 1;
        break;

      case 'q':
        cmd->noverify = 1;
        break;

      case 'r':
        cmd->read = 1;
        break;

      case 'R':
        cmd->readbin = 1;
        break;

      case 's':
        cmd->set = 1;
        cmd->set_idx = optind;
        cmd->set_cnt = 1;
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        while (! cmd_chkopt(argc, argv, &optind, 0)) cmd->set_cnt++;
        break;

      case 'S':
        cmd->start = 1;
        cmd->start_addr = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, 0)) cmd->start_addr = 0;
        break;

      case 'T':
        cmd->stopbit = 1;
        cmd->stopbit_mode = argv[optind];
        if (cmd_chkopt(argc, argv, &optind, opt)) return E_OPT;
        break;

      case 'v':
        cmd->verify = 1;
        break;

      case 'V':
        cmd->verifybin = 1;
        break;

      case 'w':
        cmd->write = 1;
        break;

      case 'W':
        cmd->writebin = 1;
        break;

      case '?':
        fprintf(stderr, "\n\nInvalid option %s!\n", argv[optind - 1]);
      default:
        return E_OPT;
    }
  }

  // show help
  if (argc < 2 || cmd->help) {
    cmd_help();
    exit(0);
  }

  // check for exclusive options
  if ((cmd->blank + cmd->erase + cmd->ihex + cmd->modify + cmd->watchdog + cmd->read + cmd->readbin
   + cmd->set + cmd->start + cmd->verify + cmd->verifybin + cmd->write + cmd->writebin) > 1) {
    fprintf(stderr, "\n\nPlease use only one of the -B,E,i,m,P,r,R,s,S,v,V,w,W options!\n");
    return E_OPT;
  }

  return E_UNK;
}


// output usage information
void cmd_help() {
  fprintf(stderr, " - An open source programmer for Atmel 8051 MCUs.\nCurrently supporting %s.\n(c) 2011 by J. Schmitz. This is free software. See source for details.\n\n", DEV_SUPPORTED);
  printf("Usage: at89flash <options>\n\n");
  printf("  -b <rate>           Baud rate to set the serial port to (Default: 9600).\n");
  printf("                      Valid rates are: 2400, 4800, 9600, 19200, 38400, 57600, 115200.\n");
  printf("  -B                  Perform blank check on Flash.\n");
  printf("  -e                  Operate on EEPROM rather than Flash.\n");
  printf("  -E                  Perform full chip erase (clear Flash and reset boot and security options).\n");
  printf("                      Use with -e to clear EEPROM data.\n");
  printf("  -h, --help          Display usage information.\n");
  printf("  -i                  Read Intel-Hex from standard input and write binary buffer to standard output.\n");
  printf("  -m <addr> <data>    Modify Flash or EEPROM memory at the given address. At maximum one page (128 bytes).\n");
  printf("                      Address ranges are 0h-FFFFh for Flash and 0h-7FFh for EEPROM.\n");
  printf("  -p <device>         Serial port to use for communication with the device.\n");
  printf("  -P                  Trigger watchdog reset pulse.\n");
  printf("  -q                  Skip verification after writing.\n");
  printf("  -r                  Read contents of Flash or EEPROM and write Intel-Hex to standard output.\n");
  printf("  -R                  Dump binary data from Flash or EEPROM to standard output.\n");
  printf("  -s <set> [...]      Apply settings in the form <option[:value]>.\n");
  printf("                      Available options are: rstb, ssb, bsb, sbv, eb, bljb, x2.\n");
  printf("  -S [addr]           Start application at the given address (0h-FFFFh, Default: 0).\n");
  printf("  -T <1|2>            Change stop-bit mode of serial port. Try this if you're having problems with USB converters.\n");
  printf("  -v                  Compare Intel-Hex from standard input to Flash or EEPROM contents.\n");
  printf("  -V                  Compare contents of Flash or EEPROM to binary data from standard input.\n");
  printf("  -w                  Write Intel-Hex from standard input to Flash or EEPROM.\n");
  printf("  -W                  Write binary data from standard input to Flash or EEPROM.\n");
  printf("\nIf no other operation is specified the device configuration is read and dumped to standard output.\nThe -p option must always be given unless -i is used.\n\n");
}
