/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator library
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

extern unsigned short biteven[];

int  getbit(unsigned char * input_data,int bit_offset);
void setbit(unsigned char * input_data,int bit_offset,int state);

void setfieldbit(unsigned char * dstbuffer,unsigned char byte,int bitoffset,int size);

int  mfmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit);
int  bintomfm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset);

int  fmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit);
int  bintofm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset);

int arburgsysfmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit);

int  searchBitStream(unsigned char * input_data,unsigned long input_data_size,int searchlen, \
					unsigned char * chr_data,unsigned long chr_data_size,unsigned long bit_offset);

void sortbuffer(unsigned char * buffer,unsigned char * outbuffer,int size);

int chgbitptr(int tracklen,int cur_offset,int offset);
