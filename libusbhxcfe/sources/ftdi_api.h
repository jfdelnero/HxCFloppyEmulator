/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
#define SRAMSIZE 8*1024

#define FT_PURGE_RX         1
#define FT_PURGE_TX         2
#define FT_EVENT_RXCHAR		    1

int32_t ftdi_load_lib (HXCFE* floppycontext);

int32_t open_ftdichip(uint32_t * ftdihandle);
int32_t close_ftdichip(uint32_t ftdihandle);
int32_t purge_ftdichip(uint32_t ftdihandle,uint32_t buffer);
int32_t setusbparameters_ftdichip(uint32_t ftdihandle,uint32_t buffersizetx,uint32_t buffersizerx);
int32_t setlatencytimer_ftdichip(uint32_t ftdihandle,unsigned char latencytimer_ms);
int32_t write_ftdichip(uint32_t ftdihandle,unsigned char * buffer,uint32_t size);
int32_t read_ftdichip(uint32_t ftdihandle,unsigned char * buffer,uint32_t size);
int32_t getfifostatus_ftdichip(uint32_t ftdihandle,int32_t * txlevel,int32_t *rxlevel,uint32_t * event);
int32_t seteventnotification_ftdichip(uint32_t ftdihandle,uint32_t eventmask,void * event);



