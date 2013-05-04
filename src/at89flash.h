/*
 * AT89FLASH
 * 
 * This is a command line based implementation of the protocol, specified
 * in the Atmel datasheet on UART bootloaders in 80C51 microcontrollers,
 * to read and write the flash memory contents and modify settings
 * of the AT89C51AC3 and similar devices.
 * 
 * Copyright (C) 2011 Johannes Schmitz.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#ifndef __AT98FLASH
#define __AT98FLASH

#define VERSION "0.1.5"
#define DEV_SUPPORTED "AT89C51AC3, AT98C51CC03"
 
typedef enum {
  E_UNK = -1,
  E_OK = 0,
  E_OPT = 1,
  E_TERM = 2,
  E_DEV = 3,
  E_SECU = 4,
  E_OPER = 5
} AT89FLASH_STATUS;

#endif
