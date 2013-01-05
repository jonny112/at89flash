/*
 * at89flash/main.c:
 * AT89FLASH main program routines
 *
 * J. Schmitz, 2011
 *
 */

#include "main.h"
#include "util.h"
#include "ihex.h"
#include "at89dev.h"


void op_read_ihex(AT89FLASH_GLOBALS *glob, FILE *fd, AT89_MEM *mem) {
  int nline, nrslt;
  fprintf(stderr, "Parsing Intel-Hex... ");
  memset(mem, 0xFF, sizeof(AT89_MEM));
  nrslt = ihex2mem(fd, mem, glob->nmaxaddr, &nline);
  if (nrslt == 0) {
    fprintf(stderr, "okay.\n");
    glob->err = E_OK;
  } else {
    fprintf(stderr, "error.\n");
    if (nrslt == AT89_ERR_PARSE) fprintf(stderr, "Invalid record at line %d.\n", nline);
    if (nrslt == AT89_ERR_MEM) fprintf(stderr, "Memory overrun at line %d.\n", nline);
    if (nrslt == AT89_ERR_BADREC) fprintf(stderr, "Bad record type at line %d.\n", nline);
    if (nrslt == AT89_ERR_NOENDREC) fprintf(stderr, "Premature end of input. Closing record not seen yet.\n");
    glob->err = E_OPER;
  }
}


void op_start_app(AT89FLASH_GLOBALS *glob, uint16_t naddr) {
  uint8_t dat[4];
  IHEX_RECORD ih;
  fprintf(stderr, "Starting application at %.4Xh... ", (int)naddr);
  ih.type = 3; ih.addr = 0; ih.len = 4; ih.data = dat;
  dat[0] = 0x03; dat[1] = 0x01; dat[2] = naddr >> 8; dat[3] = naddr & 0xF;
  if (ihex_send(glob->serport, &ih) > 0) {
    fprintf(stderr, "done.\n");
    glob->err = E_OK;
  } else {
    fprintf(stderr, "failed.\n");
    glob->err = E_DEV;
  }
}


void op_watchdog_reset(AT89FLASH_GLOBALS *glob) {
  uint8_t dat[2];
  IHEX_RECORD ih;
  fprintf(stderr, "Generating watchdog reset pulse... ");
  ih.type = 3; ih.addr = 0; ih.len = 2; ih.data = dat;
  dat[0] = 0x03; dat[1] = 0x00;
  if (ihex_send(glob->serport, &ih) > 0) {
    fprintf(stderr, "done.\n");
    glob->err = E_OK;
  } else {
    fprintf(stderr, "failed.\n");
    glob->err = E_DEV;
  }
}


void op_blank_check(AT89FLASH_GLOBALS *glob) {
  uint8_t dat[8];
  uint16_t naddr;
  int nread, nrslt, ntime;
  char ncnt;
  IHEX_RECORD ih;
  fprintf(stderr, "Blank checking Flash... ");
  ih.type = 4; ih.addr = 0; ih.len = 5; ih.data = dat;
  dat[0] = 0x00; dat[1] = 0x00; dat[2] = 0xFF; dat[3] = 0xFF; dat[4] = 0x01;
  if (ihex_send(glob->serport, &ih) > 0) {
    nread = 0; nrslt = -1; ntime = 500; ncnt = 0;
    while (nrslt < 0 && ntime > 0) {
      nrslt = readto_async(glob->serport, (char *)dat, &nread, 8);
      spinbar(stderr, &ncnt);
      usleep(100000);
      ntime--;
    }
    spinbar(stderr, 0);
    if (ntime == 0) {
      glob->err = E_DEV;
      fprintf(stderr, "timeout.\n");
    } else if (dat[0] == '.') {
      glob->err = E_OK;
      fprintf(stderr, "okay.\n");
    } else {
      glob->err = E_DEV;
      fprintf(stderr, "failed.\n");
      if (hex2int16((char *)dat, &naddr) == 0) {
	glob->err = E_OPER;
	fprintf(stderr, "Error at address %.4Xh.\n", naddr);
      }
    }
  } else {
    glob->err = E_DEV;
    fprintf(stderr, "failed.\n");
  }
}


void op_erase_dev(AT89FLASH_GLOBALS *glob) {
  uint8_t dat[3];
  int nread, nrslt, ntime;
  char ncnt;
  IHEX_RECORD ih;
  fprintf(stderr, "Performing full chip erase... ");
  ih.type = 3; ih.addr = 0; ih.len = 1; ih.data = dat; dat[0] = 0x07;
  if (ihex_send(glob->serport, &ih) > 0) {
    nread = 0; nrslt = -1; ntime = 500; ncnt = 0;
    while (nrslt < 0 && ntime > 0) {
      nrslt = readto_async(glob->serport, (char *)dat, &nread, 3);
      spinbar(stderr, &ncnt);
      usleep(100000);
      ntime--;
    }
    spinbar(stderr, 0);
    if (ntime == 0) {
      glob->err = E_DEV;
      fprintf(stderr, "timeout.\n");
    } else if (dat[0] == '.') {
      glob->err = E_OK;
      fprintf(stderr, "done.\n");
    } else {
      glob->err = E_DEV;
      fprintf(stderr, "failed.\n");
    }
  } else {
    fprintf(stderr, "failed.\n");
    glob->err = E_DEV;
  }
}

void op_mod_mem(AT89FLASH_GLOBALS *glob) {
  IHEX_RECORD ih;
  uint8_t mdata[128];
  int nrslt = 0, npos = 0, naddr;

  ih.type = (glob->cmd.eeprom ? 0x07 : 0x00); ih.data = mdata; ih.len = 0;
  naddr = strtol(glob->cmd.modify_addr, 0, 16);
  if (naddr > glob->nmaxaddr || naddr < 0) {
    fprintf(stderr, "%s address out of range!\n", (glob->cmd.eeprom ? "EEPROM" : "Flash"));
    glob->err = E_OPT;
    return;
  }
  ih.addr = naddr;
  while (ih.len < 128) {
    if (glob->cmd.modify_data[npos] == '\0') break;
    if (hex2int8(&glob->cmd.modify_data[npos], &mdata[ih.len]) != 0) {
      glob->err = E_OPT;
      break;
    }
    ih.len++;
    npos += 2;
  }
  if (glob->err == E_UNK) {
    fprintf(stderr, "Modifying %s memory at %.4Xh (%d bytes)... ", (glob->cmd.eeprom ? "EEPROM" : "Flash"), (int)ih.addr, (int)ih.len);
    if (ihex_send(glob->serport, &ih) > 0) nrslt = dev_getval(glob->serport, 0); else nrslt = AT89_ERR_TIMEOUT;
    if (nrslt == AT89_ERR_OK) {
      fprintf(stderr, "done.\n");
      glob->err = E_OK;
    } else if (nrslt == AT89_ERR_TIMEOUT) {
      fprintf(stderr, "timeout.\n");
      glob->err = E_DEV;
    } else if (nrslt == AT89_ERR_SECU) {
      fprintf(stderr, "prohibited.\n");
      glob->err = E_DEV;
    } else {
      fprintf(stderr, "failed.\n");
      glob->err = E_DEV;
    }
  } else {
    fprintf(stderr, "Bad data argument!\n");
  }
}

void op_read_mem(AT89FLASH_GLOBALS *glob, AT89_MEM *mem) {
  int naddr, nrslt;
  memset(mem, 0xFF, sizeof(AT89_MEM));
  fprintf(stderr, "Reading %s: ", (glob->cmd.eeprom ? "EEPROM" : "Flash"));
  nrslt = dev_initread(glob->serport, 0, glob->nmaxaddr, glob->cmd.eeprom);
  if (nrslt == AT89_ERR_OK) {
    progbar(stderr, 2);
    naddr = 0;
    while (naddr < glob->nmaxaddr) {
      nrslt = dev_getdata(glob->serport, mem);
      if (nrslt < 0) break;
      naddr += nrslt;
      progbar(stderr,  naddr / (glob->nmaxaddr + 1.0));
    }
    progbar(stderr, -1);
    if (naddr == (glob->nmaxaddr + 1)) {
      fprintf(stderr, "done.\n");
      glob->err = E_OK;
    } else {
      fprintf(stderr, "timeout.\n");
      glob->err = E_DEV; 
    }
  } else if (nrslt == AT89_ERR_TIMEOUT) {
    fprintf(stderr, "timeout.\n");
    glob->err = E_DEV;
  } else if (nrslt == AT89_ERR_SECU) {
    fprintf(stderr, "prohibited.\n");
    glob->err = E_SECU;
  } else {
    fprintf(stderr, "failed.\n");
    glob->err = E_DEV;
  }
}


void op_write_mem(AT89FLASH_GLOBALS *glob, AT89_MEM *mem) {
  int nrslt = 0;
  int naddr, npage = 0, npages = dev_pagecnt(mem);
  IHEX_RECORD ih;
  fprintf(stderr, "Programming %s: ", (glob->cmd.eeprom ? "EEPROM" : "Flash"));
  progbar(stderr, 2);
  for (naddr = 0; naddr < glob->nmaxaddr; naddr += 128) {
    if (dev_getpage(mem, naddr)) {
      ih.type = (glob->cmd.eeprom ? 0x07 : 0x00); ih.addr = naddr; ih.len = 128; ih.data = &mem->data[naddr];
      if (ihex_send(glob->serport, &ih) > 0) {
	nrslt = dev_getval(glob->serport, 0);
	if (nrslt != AT89_ERR_OK) break;
      } else {
	nrslt = AT89_ERR_TIMEOUT;
	break;
      }
      npage++;
      progbar(stderr, npage * 1.0 / npages);
    }
  }
  progbar(stderr, -1);
  if (nrslt == AT89_ERR_OK) {
    fprintf(stderr, "done.\n");
    glob->err = E_OK;
  } else if (nrslt == AT89_ERR_TIMEOUT) {
    fprintf(stderr, "timeout.\n");
    glob->err = E_DEV;
  } else if (nrslt == AT89_ERR_SECU) {
    fprintf(stderr, "prohibited.\n");
    glob->err = E_DEV;
  } else if (nrslt == 0) {
    fprintf(stderr, "omitted.\n");
    glob->err = E_DEV;
  } else {
    fprintf(stderr, "failed.\n");
    glob->err = E_DEV;
  }
}


void op_verify_mem(AT89FLASH_GLOBALS *glob, AT89_MEM *mem) {
  int nrslt, naddr, nread, npage = 0, npages = dev_pagecnt(mem);
  AT89_MEM mem_dev;
  fprintf(stderr, "Verifying %s: ", (glob->cmd.eeprom ? "EEPROM" : "Flash"));
  progbar(stderr, 2);
  for (naddr = 0; naddr < glob->nmaxaddr; naddr += 128) {
    if (dev_getpage(mem, naddr)) {
      nrslt = dev_initread(glob->serport, naddr, naddr + 127, glob->cmd.eeprom);
      if (nrslt != AT89_ERR_OK) break;
      nread = 0;
      while (nread < 128) {
	nrslt = dev_getdata(glob->serport, &mem_dev);
	if (nrslt < 0) break;
	nread += nrslt;
      }
      if (nread < 128) {
	nrslt = AT89_ERR_TIMEOUT;
	break;
      } else {
	if (memcmp(&mem->data[naddr], &mem_dev.data[naddr], 128) == 0) {
	  nrslt = AT89_ERR_OK;
	} else {
	  nrslt = AT89_ERR_MEM;
	  break;
	}
      }
      npage++;
      progbar(stderr, npage * 1.0 / npages);
    }
  }
  progbar(stderr, -1);
  if (nrslt == AT89_ERR_OK) {
    fprintf(stderr, "okay.\n");
    glob->err = E_OK;
  } else if (nrslt == AT89_ERR_TIMEOUT) {
    fprintf(stderr, "timeout.\n");
    glob->err = E_DEV;
  } else if (nrslt == AT89_ERR_SECU) {
    fprintf(stderr, "prohibited.\n");
    glob->err = E_DEV;
  } else {
    fprintf(stderr, "failed.\nVerification failed at block starting at %.4Xh.\n", naddr);
    glob->err = E_DEV;
  }
}


void op_dump_conf(AT89FLASH_GLOBALS *glob) {
  uint16_t cf_mfc, cf_fmc, cf_pdn, cf_pdr, cf_ssb, cf_bsb, cf_sbv, cf_eb, cf_hsb, cf_id1, cf_id2, cf_blv;
  char cbin[9];
  fprintf(stderr, "Reading device configuration... ");
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_MFC, &cf_mfc) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_FMC, &cf_fmc) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_PDN, &cf_pdn) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_PDR, &cf_pdr) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_SSB, &cf_ssb) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_BSB, &cf_bsb) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_SBV, &cf_sbv) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_EB, &cf_eb) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_HSB, &cf_hsb) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_ID1, &cf_id1) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_ID2, &cf_id2) < 0) glob->err = E_DEV;
  if (glob->err != E_UNK || dev_getconf(glob->serport, AT89_CONF_BLV, &cf_blv) < 0) glob->err = E_DEV;
  
  if (glob->err == E_UNK) {
    fprintf(stderr, "done.\n");
    printf("\nManufacturer Code     :  ");
    if (cf_mfc < 0x100) printf("%.2Xh", cf_mfc); else printf("[locked]");
    printf("\nFamily Code           :  ");
    if (cf_fmc < 0x100) printf("%.2Xh", cf_fmc); else printf("[locked]");
    printf("\nProduct Code          :  ");
    if (cf_pdn < 0x100) printf("%.2Xh", cf_pdn); else printf("[locked]");
    printf("\nProduct Revision      :  ");
    if (cf_pdr < 0x100) printf("%.2Xh", cf_pdr); else printf("[locked]");
    printf("\nSoftware Security     :  ");
    if (cf_ssb < 0x100) printf("%.2Xh (%s)", cf_ssb, (cf_ssb == 0xFF ? "L0: no protection" : (cf_ssb == 0xFE ? "L1: write protection" : (cf_ssb == 0xFC ? "L2: read/write protection" : "??")))); else printf("[locked]");
    printf("\nBoot Status Byte      :  ");
    if (cf_bsb < 0x100) printf("%.2Xh [%s]", cf_bsb, byte2bin(cf_bsb, cbin)); else printf("[locked]");
    printf("\nSoftware Boot Vector  :  ");
    if (cf_sbv < 0x100) printf("%.2Xh", cf_sbv); else printf("[locked]");
    printf("\nExtra Byte            :  ");
    if (cf_eb < 0x100) printf("%.2Xh [%s]", cf_eb, byte2bin(cf_eb, cbin)); else printf("[locked]");
    printf("\nFuse Bits             :  ");
    if (cf_hsb < 0x100) printf("%s %s [%s]", (cf_hsb & (1 << 7) ? "-X2" : "+X2"), (cf_hsb & (1 << 6) ? "-BLJB" : "+BLJB"), byte2bin(cf_hsb, cbin)); else printf("[locked]");
    printf("\nDevice IDs            :  ");
    if (cf_id1 < 0x100 && cf_id2 < 0x100) printf("1:%.2Xh 2:%.2Xh", cf_id1, cf_id2); else printf("[locked]");
    printf("\nBootloader Version    :  ");
    if (cf_blv < 0x100) printf("%d", cf_blv); else printf("[locked]");
    printf("\n");
    glob->err = E_OK;
  } else {
    fprintf(stderr, "failed.\n");
  }
}


void op_set_conf(AT89FLASH_GLOBALS *glob) {
  IHEX_RECORD ih;
  int nrslt;
  int narg;
  for (narg = glob->cmd.set_idx; narg < (glob->cmd.set_idx + glob->cmd.set_cnt) && glob->err == E_UNK; narg++) {
    if (strcmp(glob->cmd.argv[narg], "rstb") == 0) {
      // reset boot options
      uint8_t dat[2];
      ih.type = 3; ih.addr = 0; ih.len = 2; ih.data = dat;
      dat[0] = 0x04; dat[1] = 0x00;
      fprintf(stderr, "Resetting boot options... ");
    } else if (strlen(glob->cmd.argv[narg]) == 5 && memcmp(glob->cmd.argv[narg], "ssb:", 4) == 0) {
      // security byte
      uint8_t dat[2];
      ih.type = 3; ih.addr = 0; ih.len = 2; ih.data = dat;
      dat[0] = 0x05; dat[1] = 2;
      if (glob->cmd.argv[narg][4] == '1') dat[1] = 0;
      if (glob->cmd.argv[narg][4] == '2') dat[1] = 1;
      if (dat[1] > 1) {
	fprintf(stderr, "Bad value at '%s'.\n", glob->cmd.argv[narg]);
	glob->err = E_OPT;
      } else fprintf(stderr, "Setting software security level... ");
    } else if (strlen(glob->cmd.argv[narg]) > 4 && memcmp(glob->cmd.argv[narg], "bsb:", 4) == 0) {
      // boot status byte
      uint8_t dat[3];
      ih.type = 3; ih.addr = 0; ih.len = 3; ih.data = dat;
      dat[0] = 0x06; dat[1] = 0x00; dat[2] = strtol(&glob->cmd.argv[narg][4], 0, 16);
      fprintf(stderr, "Programming boot status byte... ");
    } else if (strlen(glob->cmd.argv[narg]) > 4 && memcmp(glob->cmd.argv[narg], "sbv:", 4) == 0) {
      // software boot vector
      uint8_t dat[3];
      ih.type = 3; ih.addr = 0; ih.len = 3; ih.data = dat;
      dat[0] = 0x06; dat[1] = 0x01; dat[2] = strtol(&glob->cmd.argv[narg][4], 0, 16);
      fprintf(stderr, "Programming software boot vector... ");
    } else if (strlen(glob->cmd.argv[narg]) > 3 && memcmp(glob->cmd.argv[narg], "eb:", 3) == 0) {
      // extra byte
      uint8_t dat[3];
      ih.type = 3; ih.addr = 0; ih.len = 3; ih.data = dat;
      dat[0] = 0x06; dat[1] = 0x06; dat[2] = strtol(&glob->cmd.argv[narg][3], 0, 16);
      fprintf(stderr, "Programming extra byte... ");
    } else if (strlen(glob->cmd.argv[narg]) == 6 && memcmp(glob->cmd.argv[narg], "bljb:", 5) == 0) {
      // bootloader jump bit
      uint8_t dat[3];
      ih.type = 3; ih.addr = 0; ih.len = 3; ih.data = dat;
      dat[0] = 0x0A; dat[1] = 0x04; dat[2] = 2;
      if (glob->cmd.argv[narg][5] == '0') dat[2] = 0;
      if (glob->cmd.argv[narg][5] == '1') dat[2] = 1;
      if (dat[2] > 1) {
	fprintf(stderr, "Bad value at '%s'.\n", glob->cmd.argv[narg]);
	glob->err = E_OPT;
      } else fprintf(stderr, "Setting bootloader jump bit... ");
    } else if(strlen(glob->cmd.argv[narg]) == 4 && memcmp(glob->cmd.argv[narg], "x2:", 3) == 0) {
      // X2 bit
      uint8_t dat[3];
      ih.type = 3; ih.addr = 0; ih.len = 3; ih.data = dat;
      dat[0] = 0x0A; dat[1] = 0x08; dat[2] = 2;
      if (glob->cmd.argv[narg][3] == '0') dat[2] = 0;
      if (glob->cmd.argv[narg][3] == '1') dat[2] = 1;
      if (dat[2] > 1) {
	fprintf(stderr, "Bad value at '%s'.\n", glob->cmd.argv[narg]);
	glob->err = E_OPT;
      } else fprintf(stderr, "Setting X2 bit... ");
    } else {
      glob->err = E_OPT;
      fprintf(stderr, "Invalid setting '%s'.\n", glob->cmd.argv[narg]);
    }
    
    // apply setting
    if (glob->err == E_UNK) {
      if (ihex_send(glob->serport, &ih) > 0) {
	nrslt = dev_getval(glob->serport, 0);
	if (nrslt == AT89_ERR_OK) fprintf(stderr, "acknowledged.\n");  
	else if (nrslt == AT89_ERR_SECU) {
	  glob->err = E_SECU;
	  fprintf(stderr, "prohibited.\n");
	} else {
	  glob->err = E_DEV;
	  fprintf(stderr, "failed.\n");
	}
      } else {
	glob->err = E_DEV;
	fprintf(stderr, "failed.\n");
      }
    }
  }
  if (glob->err == E_UNK) glob->err = E_OK;
}


int main(int argc, char *argv[]) {
  AT89FLASH_GLOBALS glob;
  glob.err = E_UNK;
  
  int nrslt;
  
  fprintf(stderr, "AT89Flash %s", VERSION);
  
  // parse command line
  glob.err = cmd_parse(argc, argv, &glob.cmd);
  if (glob.err == E_UNK) fprintf(stderr, "\n\n");
  
  // set memory limits
  glob.nmaxaddr = (glob.cmd.eeprom ? 0x07FF : 0xFFFF);
  
  // process Intel-Hex
  if (glob.err == E_UNK && glob.cmd.ihex) {
    AT89_MEM mem;
    op_read_ihex(&glob, stdin, &mem);
    if (glob.err == E_OK) fwrite(mem.data, 1, glob.nmaxaddr + 1, stdout);
  }
  
  // proceed to device mode
  if (glob.err == E_UNK) {
  
    // check for port option (mandatory)
    if (! glob.cmd.port) {
      glob.err = E_OPT;
      fprintf(stderr, "No communications port specified!\nPlease use the -p option.\n");
    }
    
    // check for baud rate option (optional)
    if (glob.cmd.baud) {
      glob.nbaud = atoi(glob.cmd.baud_val);
      switch (glob.nbaud) {
        case 2400: glob.baud = B2400; break;
        case 4800: glob.baud = B4800; break;
        case 9600: glob.baud = B9600; break;
        case 19200: glob.baud = B19200; break;
        case 38400: glob.baud = B38400; break;
        case 57600: glob.baud = B57600; break;
        case 115200: glob.baud = B115200; break;
        default:
          glob.err = E_OPT;
          fprintf(stderr, "Invalid baud rate specified!\n");
      }
    } else {
      glob.baud = B9600;
      glob.nbaud = 9600;
    }
    
    // open serial port
    if (glob.err == E_UNK) {
      fprintf(stderr, "Opening serial port %s... ", glob.cmd.port_dev);
      if ((glob.serport = open(glob.cmd.port_dev, O_RDWR | O_NOCTTY | O_NDELAY)) != -1) {
	tcgetattr(glob.serport, &glob.termios_old);
	memcpy(&glob.termios_new, &glob.termios_old, sizeof(TERMIOS));
	cfmakeraw(&glob.termios_new);
	cfsetspeed(&glob.termios_new, glob.baud);
	glob.termios_new.c_oflag |= ONLCR;
	
	if (glob.cmd.stopbit) {
          switch (atoi(glob.cmd.stopbit_mode)) {
            case 1: glob.termios_new.c_cflag &= ~CSTOPB; break;
            case 2: glob.termios_new.c_cflag |= CSTOPB; break;
            default:
              glob.err = E_OPT;
              fprintf(stderr, "Invalid stop-bit mode!\n");
          }
	}

	if (glob.err == E_UNK) {
	  tcflush(glob.serport, TCIOFLUSH);
	  if (tcsetattr(glob.serport, TCSANOW, &glob.termios_new) == 0)  fprintf(stderr, "done.\n");
	  else {
	    glob.err = E_TERM;
	    fprintf(stderr, "error.\n%s\n", strerror(errno));
	  }
	}
      } else {
	glob.err = E_TERM;
	fprintf(stderr, "failed.\n%s\n", strerror(errno));
      }
      
      // synchronize device
      if (glob.err == E_UNK) {
	fprintf(stderr, "Synchronizing device at %d baud... ", glob.nbaud);
	nrslt = dev_sync(glob.serport);
	if (nrslt < 0) {
	  glob.err = E_DEV;
	  fprintf(stderr, "timeout.\n");
	} else if (nrslt > 0) {
	  glob.err = E_DEV;
	  fprintf(stderr, "failed.\n");
	} else {
	  fprintf(stderr, "okay.\n");
	}
	
	// perform operations
	if (glob.err == E_UNK) {
	  fprintf(stderr, "\n");
	  // start application
	  if (glob.err == E_UNK && glob.cmd.start) {
	    long naddr = 0;
	    if (glob.cmd.start_addr != 0) naddr = strtol(glob.cmd.start_addr, 0, 16);
	    if (naddr < 0 || naddr > 0xFFFF) {
	      fprintf(stderr, "Bad starting address.\n");
	      glob.err = E_OPT;
	    } else {
	      op_start_app(&glob, naddr);
	    }
	  }
	  
	  // watchdog reset
	  if (glob.err == E_UNK && glob.cmd.watchdog) {
	    op_watchdog_reset(&glob);
	  }
	  
	  // blank check
	  if (glob.err == E_UNK && glob.cmd.blank) {
	    op_blank_check(&glob);
	  }
	  
	  // read flash/eeprom
	  if (glob.err == E_UNK && glob.cmd.read) {
	    AT89_MEM mem;
	    op_read_mem(&glob, &mem);
	    if (glob.err == E_OK) mem2ihex(stdout, &mem, glob.nmaxaddr);
	  }
	  if (glob.err == E_UNK && glob.cmd.readbin) {
	    AT89_MEM mem;
	    op_read_mem(&glob, &mem);
	    if (glob.err == E_OK) fwrite(mem.data, 1, glob.nmaxaddr + 1, stdout);
	  }

	  // verify flash contents
	  if (glob.err == E_UNK && glob.cmd.verify) {
	    AT89_MEM mem;
	    op_read_ihex(&glob, stdin, &mem);
	    if (glob.err == E_OK) {
	      glob.err = E_UNK;
	      op_verify_mem(&glob, &mem);
	    }
	  }
	  if (glob.err == E_UNK && glob.cmd.verifybin) {
	    AT89_MEM mem_local, mem_dev;
	    memset(&mem_local, 0xFF, sizeof(AT89_MEM));
	    fread(mem_local.data, 1, glob.nmaxaddr + 1, stdin);
	    op_read_mem(&glob, &mem_dev);
	    if (glob.err == E_OK) {  
	      fprintf(stderr, "Comparing buffers... ");
	      if (memcmp(mem_local.data, mem_dev.data, glob.nmaxaddr + 1) == 0) {
		fprintf(stderr, "match.\n");
	      } else {
		fprintf(stderr, "inconsistent.\n");  
		glob.err = E_OPER;
	      }
	    }
	  }

	  // configure settings
	  if (glob.err == E_UNK && glob.cmd.set) {
	    op_set_conf(&glob);
	  }
	  
	  // modify flash/eeprom
	  if (glob.err == E_UNK && glob.cmd.modify) {
	    op_mod_mem(&glob);
	  }
	  
	  // write flash/eeprom
	  if (glob.err == E_UNK && glob.cmd.write) {
	    AT89_MEM mem;
	    op_read_ihex(&glob, stdin, &mem);
	    if (glob.err == E_OK) {
	      glob.err = E_UNK;
	      op_write_mem(&glob, &mem);
	      if (glob.err == E_OK && getcmd(argc, argv, "-q", 0) == 0) {
		op_verify_mem(&glob, &mem);
	      }
	    }
	  }
	  if (glob.err == E_UNK && glob.cmd.writebin) {
	    AT89_MEM mem;
	    int naddr;
	    memset(&mem, 0xFF, sizeof(AT89_MEM));
	    nrslt = fread(mem.data, 1, glob.nmaxaddr + 1, stdin);
	    if (nrslt > 0) {
	      for (naddr = 0; naddr < nrslt; naddr += 128) dev_setpage(&mem, naddr);
	      op_write_mem(&glob, &mem);
	    } else {
	      fprintf(stderr, "No data provided for writing.\n");
	      glob.err = E_OPER;
	    }
	  }
	  
	  // full chip erase
	  if (glob.err == E_UNK && glob.cmd.erase) {
	    if (glob.cmd.eeprom) {
	      AT89_MEM mem;
	      memset(&mem, 0xFF, sizeof(AT89_MEM));
	      memset(&mem.pages, 0, 2);
	      op_write_mem(&glob,& mem);
	    } else {
	      op_erase_dev(&glob);
	      if (glob.err == E_OK) {
		glob.err = E_UNK;
		op_blank_check(&glob);
	      }
	    }
	  }

	  // read configuration
	  if (glob.err == E_UNK) {
	    op_dump_conf(&glob);
	  }

	}
	
	// restore serial port settings
	if (glob.serport > 0) {
	  tcsetattr(glob.serport, TCSANOW, &glob.termios_old);
	  close(glob.serport);
	}
      } 
    }
  }

  // report final status
  fprintf(stderr, "\nat89flash: ");
  switch (glob.err) {
    case E_OK: fprintf(stderr, "Finished successfully!"); break;
    case E_OPT: fprintf(stderr, "Bad list of arguments! Try -h to get help."); break;
    case E_TERM: fprintf(stderr, "Error accessing serial port!"); break;
    case E_DEV: fprintf(stderr, "Error communicating with the device!"); break;
    case E_SECU: fprintf(stderr, "Operation not permitted!"); break;
    case E_OPER: fprintf(stderr, "Operation failed!"); break;
    default: fprintf(stderr, "Please file a bug report!");
  }
  fprintf(stderr, "\n\n");
  
  return glob.err;  
}
