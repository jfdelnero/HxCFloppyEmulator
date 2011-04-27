/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : ISO_IBM_track.c
// Contains: ISO/IBM track builder/encoder
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "crc.h"
#include "iso_ibm_track.h"


unsigned short MFM_tab[]=
{
	0xAAAA,0xAAA9,0xAAA4,0xAAA5,0xAA92,0xAA91,0xAA94,0xAA95,
	0xAA4A,0xAA49,0xAA44,0xAA45,0xAA52,0xAA51,0xAA54,0xAA55,
	0xA92A,0xA929,0xA924,0xA925,0xA912,0xA911,0xA914,0xA915,
	0xA94A,0xA949,0xA944,0xA945,0xA952,0xA951,0xA954,0xA955,
	0xA4AA,0xA4A9,0xA4A4,0xA4A5,0xA492,0xA491,0xA494,0xA495,
	0xA44A,0xA449,0xA444,0xA445,0xA452,0xA451,0xA454,0xA455,
	0xA52A,0xA529,0xA524,0xA525,0xA512,0xA511,0xA514,0xA515,
	0xA54A,0xA549,0xA544,0xA545,0xA552,0xA551,0xA554,0xA555,
	0x92AA,0x92A9,0x92A4,0x92A5,0x9292,0x9291,0x9294,0x9295,
	0x924A,0x9249,0x9244,0x9245,0x9252,0x9251,0x9254,0x9255,
	0x912A,0x9129,0x9124,0x9125,0x9112,0x9111,0x9114,0x9115,
	0x914A,0x9149,0x9144,0x9145,0x9152,0x9151,0x9154,0x9155,
	0x94AA,0x94A9,0x94A4,0x94A5,0x9492,0x9491,0x9494,0x9495,
	0x944A,0x9449,0x9444,0x9445,0x9452,0x9451,0x9454,0x9455,
	0x952A,0x9529,0x9524,0x9525,0x9512,0x9511,0x9514,0x9515,
	0x954A,0x9549,0x9544,0x9545,0x9552,0x9551,0x9554,0x9555,
	0x4AAA,0x4AA9,0x4AA4,0x4AA5,0x4A92,0x4A91,0x4A94,0x4A95,
	0x4A4A,0x4A49,0x4A44,0x4A45,0x4A52,0x4A51,0x4A54,0x4A55,
	0x492A,0x4929,0x4924,0x4925,0x4912,0x4911,0x4914,0x4915,
	0x494A,0x4949,0x4944,0x4945,0x4952,0x4951,0x4954,0x4955,
	0x44AA,0x44A9,0x44A4,0x44A5,0x4492,0x4491,0x4494,0x4495,
	0x444A,0x4449,0x4444,0x4445,0x4452,0x4451,0x4454,0x4455,
	0x452A,0x4529,0x4524,0x4525,0x4512,0x4511,0x4514,0x4515,
	0x454A,0x4549,0x4544,0x4545,0x4552,0x4551,0x4554,0x4555,
	0x52AA,0x52A9,0x52A4,0x52A5,0x5292,0x5291,0x5294,0x5295,
	0x524A,0x5249,0x5244,0x5245,0x5252,0x5251,0x5254,0x5255,
	0x512A,0x5129,0x5124,0x5125,0x5112,0x5111,0x5114,0x5115,
	0x514A,0x5149,0x5144,0x5145,0x5152,0x5151,0x5154,0x5155,
	0x54AA,0x54A9,0x54A4,0x54A5,0x5492,0x5491,0x5494,0x5495,
	0x544A,0x5449,0x5444,0x5445,0x5452,0x5451,0x5454,0x5455,
	0x552A,0x5529,0x5524,0x5525,0x5512,0x5511,0x5514,0x5515,
	0x554A,0x5549,0x5544,0x5545,0x5552,0x5551,0x5554,0x5555
};


unsigned short CLK_tab[]=
{
	0x5555,0x5557,0x555D,0x555F,0x5575,0x5577,0x557D,0x557F,
	0x55D5,0x55D7,0x55DD,0x55DF,0x55F5,0x55F7,0x55FD,0x55FF,
	0x5755,0x5757,0x575D,0x575F,0x5775,0x5777,0x577D,0x577F,
	0x57D5,0x57D7,0x57DD,0x57DF,0x57F5,0x57F7,0x57FD,0x57FF,
	0x5D55,0x5D57,0x5D5D,0x5D5F,0x5D75,0x5D77,0x5D7D,0x5D7F,
	0x5DD5,0x5DD7,0x5DDD,0x5DDF,0x5DF5,0x5DF7,0x5DFD,0x5DFF,
	0x5F55,0x5F57,0x5F5D,0x5F5F,0x5F75,0x5F77,0x5F7D,0x5F7F,
	0x5FD5,0x5FD7,0x5FDD,0x5FDF,0x5FF5,0x5FF7,0x5FFD,0x5FFF,
	0x7555,0x7557,0x755D,0x755F,0x7575,0x7577,0x757D,0x757F,
	0x75D5,0x75D7,0x75DD,0x75DF,0x75F5,0x75F7,0x75FD,0x75FF,
	0x7755,0x7757,0x775D,0x775F,0x7775,0x7777,0x777D,0x777F,
	0x77D5,0x77D7,0x77DD,0x77DF,0x77F5,0x77F7,0x77FD,0x77FF,
	0x7D55,0x7D57,0x7D5D,0x7D5F,0x7D75,0x7D77,0x7D7D,0x7D7F,
	0x7DD5,0x7DD7,0x7DDD,0x7DDF,0x7DF5,0x7DF7,0x7DFD,0x7DFF,
	0x7F55,0x7F57,0x7F5D,0x7F5F,0x7F75,0x7F77,0x7F7D,0x7F7F,
	0x7FD5,0x7FD7,0x7FDD,0x7FDF,0x7FF5,0x7FF7,0x7FFD,0x7FFF,
	0xD555,0xD557,0xD55D,0xD55F,0xD575,0xD577,0xD57D,0xD57F,
	0xD5D5,0xD5D7,0xD5DD,0xD5DF,0xD5F5,0xD5F7,0xD5FD,0xD5FF,
	0xD755,0xD757,0xD75D,0xD75F,0xD775,0xD777,0xD77D,0xD77F,
	0xD7D5,0xD7D7,0xD7DD,0xD7DF,0xD7F5,0xD7F7,0xD7FD,0xD7FF,
	0xDD55,0xDD57,0xDD5D,0xDD5F,0xDD75,0xDD77,0xDD7D,0xDD7F,
	0xDDD5,0xDDD7,0xDDDD,0xDDDF,0xDDF5,0xDDF7,0xDDFD,0xDDFF,
	0xDF55,0xDF57,0xDF5D,0xDF5F,0xDF75,0xDF77,0xDF7D,0xDF7F,
	0xDFD5,0xDFD7,0xDFDD,0xDFDF,0xDFF5,0xDFF7,0xDFFD,0xDFFF,
	0xF555,0xF557,0xF55D,0xF55F,0xF575,0xF577,0xF57D,0xF57F,
	0xF5D5,0xF5D7,0xF5DD,0xF5DF,0xF5F5,0xF5F7,0xF5FD,0xF5FF,
	0xF755,0xF757,0xF75D,0xF75F,0xF775,0xF777,0xF77D,0xF77F,
	0xF7D5,0xF7D7,0xF7DD,0xF7DF,0xF7F5,0xF7F7,0xF7FD,0xF7FF,
	0xFD55,0xFD57,0xFD5D,0xFD5F,0xFD75,0xFD77,0xFD7D,0xFD7F,
	0xFDD5,0xFDD7,0xFDDD,0xFDDF,0xFDF5,0xFDF7,0xFDFD,0xFDFF,
	0xFF55,0xFF57,0xFF5D,0xFF5F,0xFF75,0xFF77,0xFF7D,0xFF7F,
	0xFFD5,0xFFD7,0xFFDD,0xFFDF,0xFFF5,0xFFF7,0xFFFD,0xFFFF
};


// Fast Bin to MFM converter
int BuildCylinder(unsigned char * mfm_buffer,int mfm_size,unsigned char * track_clk,unsigned char * track_data,int track_size)
{
	int i,l;
	unsigned char byte,clock;
	unsigned short lastbit;
	unsigned short mfm_code;
	
	if(track_size*2>mfm_size)
	{
		track_size=mfm_size/2;
	}

	// MFM Encoding
	lastbit=0xFFFF;
	i=0;
	for(l=0;l<track_size;l++)
	{
		byte =track_data[l];
		clock=track_clk[l];

		mfm_code = MFM_tab[byte] & CLK_tab[clock] & lastbit;

		mfm_buffer[i++]=mfm_code>>8;
		mfm_buffer[i++]=mfm_code&0xFF;

		lastbit=~(MFM_tab[byte]<<15);		
	}

	// Clear the remaining buffer bytes
	while(i<mfm_size)
	{
		mfm_buffer[i++]=0x00;
	}

	return track_size;
}

// mfm encoder
/*
void BuildCylinder(char * buffer,int mfmtracksize,char * bufferclk,char * track,int size)
{
	int i,j,l,m,s;
	unsigned char byte;
	int lastbit;
	
	// Clean up
	for(i=0;i<(mfmtracksize);i++)
	{
		buffer[i]=0x00;
	}

	j=0;
	s=0;

	// MFM Encoding
	lastbit=0;
	for(l=0;l<size;l++)
	{
		byte=track[l];
		for(m=0;m<8;m++)
		{
			if(byte&(0x80>>m))
			{
				buffer[j]=buffer[j]|((0x40)>>s);		
				s=s+2;
				if(s==8)
				{
					s=0;
					j++;
					if(j<mfmtracksize) buffer[j]=0;
					
				}
			}
			else
			{
				if(bufferclk[l]&(0x80>>m))
				{
					if(lastbit)
					{
						buffer[j]=buffer[j]|((0x00)>>s);
						
						s=s+2;
						if(s==8)
						{
							s=0;
							j++;
							if(j<mfmtracksize) buffer[j]=0;
						}
					}
					else
					{
						buffer[j]=buffer[j]|((0x80)>>s);
										
						s=s+2;
						if(s==8)
						{
							s=0;
							j++;
							if(j<mfmtracksize) buffer[j]=0;
						}
					}
				}
				else
				{
					buffer[j]=buffer[j]|((0x00)>>s);
					
					s=s+2;
					if(s==8)
					{
						s=0;
						j++;
						if(j<mfmtracksize) buffer[j]=0;
					}
				}
			}
			lastbit=byte&(0x80>>m);
		}
	}
}
*/

// FM encoder
void BuildFMCylinder(char * buffer,int fmtracksize,char * bufferclk,char * track,int size)
{
	int i,j,k,l;
	unsigned char byte,clock;
	
	// Clean up
	for(i=0;i<(fmtracksize);i++)
	{
		buffer[i]=0x00;
	}

	j=0;

	// FM Encoding
	j=0;
	for(l=0;l<size;l++)
	{
		byte=track[l];
		clock=bufferclk[l];
			
		for(k=0;k<4;k++)
		{
			buffer[j]=0x00;
			////////////////////////////////////
			// data
			for(i=0;i<2;i++)
			{
				if(byte&(0x80>>(i+(k*2)) ))
				{	// 0x10 
					// 00010001)
					buffer[j]=buffer[j] | (0x10>>(i*4));
				}
			}

			// clock
			for(i=0;i<2;i++)
			{
				if(clock&(0x80>>(i+(k*2)) ))
				{	// 0x40 
					// 01000100)
					buffer[j]=buffer[j] | (0x40>>(i*4));
				}
			}

			j++;
		}
	}
}

int ISOIBMGetTrackSize(int TRACKTYPE,unsigned int numberofsector,unsigned int sectorsize,unsigned int gap3len,SECTORCONFIG * sectorconfigtab)
{
	unsigned int i,j;
	isoibm_config * configptr;
	unsigned long finalsize;
	unsigned long totaldatasize;


	configptr=NULL;
	i=0;
	while (formatstab[i].indexformat!=TRACKTYPE &&  formatstab[i].indexformat!=0)
	{
		i++;
	};

	configptr=&formatstab[i];
	totaldatasize=0;
	if(sectorconfigtab)
	{
		sectorsize=0;
		for(j=0;j<numberofsector;j++)
		{
			totaldatasize=totaldatasize+(sectorconfigtab[j].sectorsize);
		}
	}

	finalsize=configptr->len_gap4a+configptr->len_isync+configptr->len_indexmarkp1+configptr->len_indexmarkp2 + \
			  configptr->len_gap1 +  \
			  numberofsector*(configptr->len_ssync+configptr->len_addrmarkp1+configptr->len_addrmarkp2 +6 +configptr->len_gap2 +configptr->len_dsync+configptr->len_datamarkp1+configptr->len_datamarkp2+sectorsize+2+gap3len) +\
			  totaldatasize;

	return finalsize;

}

unsigned char* compute_interleave_tab(unsigned char interleave,unsigned char skew,unsigned short numberofsector)
{

	int i,j;
	unsigned char *interleave_tab;
	unsigned char *allocated_tab;



	interleave_tab=(unsigned char *)malloc(numberofsector*sizeof(unsigned char));
	if(numberofsector)
	{
		if(interleave_tab)
		{
			allocated_tab=(unsigned char *)malloc(numberofsector*sizeof(unsigned char));
			memset(interleave_tab,0,numberofsector*sizeof(unsigned char));
			memset(allocated_tab,0,numberofsector*sizeof(unsigned char));

			j=skew%(numberofsector);
			i=0;
			do
			{
				while(allocated_tab[j])
				{
					j=(j+1)%(numberofsector);
				}
				interleave_tab[j]=i;
				allocated_tab[j]=0xFF;
				j=(j+interleave)%(numberofsector);
				i++;
			}while(i<numberofsector);		

			free(allocated_tab);
		}
	}
	
	return interleave_tab;
}

int BuildISOTrack(HXCFLOPPYEMULATOR* floppycontext,int TRACKTYPE,unsigned int numberofsector,unsigned char startidsector,unsigned int sectorsize,unsigned int tracknumber,unsigned int sidenumber,unsigned int gap3len,unsigned char* datain,unsigned char * mfmdata,unsigned long * mfmsizebuffer,int interleave,int skew,SECTORCONFIG * sectorconfigtab)
{
	unsigned int i,j,k,l,t;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	isoibm_config * configptr;
	unsigned char *tempdata;
	unsigned char *tempclock;
	unsigned long finalsize;
	unsigned long indexbuffer;
	unsigned long totaldatasize;
	unsigned long current_buffer_size;
	unsigned long header_size;
	unsigned long datapart_size;

	unsigned char * interleave_tab;
	unsigned char crctable[32];
	configptr=NULL;
	i=0;
	while (formatstab[i].indexformat!=TRACKTYPE &&  formatstab[i].indexformat!=0)
	{
		i++;
	};

	configptr=&formatstab[i];
	totaldatasize=0;
	if(sectorconfigtab)
	{
		sectorsize=0;
		for(j=0;j<numberofsector;j++)
		{
			totaldatasize=totaldatasize+(sectorconfigtab[j].sectorsize);
		}
	}

	finalsize=configptr->len_gap4a+configptr->len_isync+configptr->len_indexmarkp1+configptr->len_indexmarkp2 + \
			  configptr->len_gap1 +  \
			  numberofsector*(configptr->len_ssync+configptr->len_addrmarkp1+configptr->len_addrmarkp2 +6 +configptr->len_gap2 +configptr->len_dsync+configptr->len_datamarkp1+configptr->len_datamarkp2+sectorsize+2+gap3len) +\
			  totaldatasize;

	indexbuffer=0;
	current_buffer_size=*mfmsizebuffer/2;
	if(TRACKTYPE==IBMFORMAT_SD)
	{
		current_buffer_size=*mfmsizebuffer/4;
	}

	interleave_tab=compute_interleave_tab((unsigned char)interleave,(unsigned char)skew,(unsigned short)numberofsector);


	if(finalsize<=current_buffer_size)
	{
		j=0;
		tempdata=(char *)malloc((*mfmsizebuffer/2)+1);
		tempclock=(char *)malloc((*mfmsizebuffer/2)+1);
		
		//gap4a (post index gap4)
		for(k=0;k<configptr->len_gap4a;k++)
		{
				tempdata[j]=configptr->data_gap4a;
				tempclock[j]=0xFF;
				j++;
		}

		//i sync
		for(k=0;k<configptr->len_isync;k++)
		{
				tempdata[j]=configptr->data_isync;
				tempclock[j]=0xFF;
				j++;
		}


		// index mark
		for(k=0;k<configptr->len_indexmarkp1;k++)
		{
				tempdata[j]=configptr->data_indexmarkp1;
				tempclock[j]=configptr->clock_indexmarkp1;
				j++;
		}
		for(k=0;k<configptr->len_indexmarkp2;k++)
		{
				tempdata[j]=configptr->data_indexmarkp2;
				tempclock[j]=configptr->clock_indexmarkp2;
				j++;
		}


		// gap1
		for(k=0;k<configptr->len_gap1;k++)
		{
				tempdata[j]=configptr->data_gap1;
				tempclock[j]=0xFF;
				j++;
		}

		// sectors
		for(l=0;l<numberofsector;l++)
		{

				if(sectorconfigtab)
				{
					sectorconfigtab[interleave_tab[l]].startsectorindex=j*2;
				}

			
				// sync
				for(k=0;k<configptr->len_ssync;k++)
				{
						tempdata[j]=configptr->data_ssync;
						tempclock[j]=0xFF;
						j++;
				}

				header_size=0;
				// add mark
				for(k=0;k<configptr->len_addrmarkp1;k++)
				{
					tempdata[j]=configptr->data_addrmarkp1;
					tempclock[j]=configptr->clock_addrmarkp1;
					j++;
					header_size++;	
				}

				for(k=0;k<configptr->len_addrmarkp2;k++)
				{

						if(sectorconfigtab)
						{
							if(sectorconfigtab[interleave_tab[l]].use_alternate_addressmark)
							{
								tempdata[j]=sectorconfigtab[interleave_tab[l]].alternate_addressmark;
								tempclock[j]=configptr->clock_addrmarkp2;
								j++;
								header_size++;
							}
							else
							{
								tempdata[j]=configptr->data_addrmarkp2;
								tempclock[j]=configptr->clock_addrmarkp2;
								j++;
								header_size++;
							}
						}
						else
						{
							tempdata[j]=configptr->data_addrmarkp2;
							tempclock[j]=configptr->clock_addrmarkp2;
							j++;
							header_size++;
						}
				}

				// track number
				tempdata[j]=tracknumber;
				tempclock[j]=0xFF;
				if(sectorconfigtab)
				{
					tempdata[j]=sectorconfigtab[interleave_tab[l]].cylinder;
				}
				j++;
				header_size++;


				//01 Side # The side number this is (0 or 1) 
				tempdata[j]=sidenumber;
				tempclock[j]=0xff;
				if(sectorconfigtab)
				{
					tempdata[j]=sectorconfigtab[interleave_tab[l]].head;
				}
				j++;
				header_size++;
			
				//01 Sector # The sector number 
				if(sectorconfigtab)
				{
					tempdata[j]=sectorconfigtab[interleave_tab[l]].sector;
					tempclock[j]=0xFF;
				}
				else
				{
					tempdata[j]=interleave_tab[l]+startidsector;
					tempclock[j]=0xFF;
				}
				j++;
				header_size++;
			

				if(sectorconfigtab)
				{
					sectorsize=sectorconfigtab[interleave_tab[l]].sectorsize;
				}

			
				//01 02 Sector size: 02=512. (00=128, 01=256, 02=512, 03=1024) 
				switch(sectorsize)
				{
					case 128:
						tempdata[j]=00;
					break;
					case 256:
						tempdata[j]=01;
					break;
					case 512:
						tempdata[j]=02;
					break;
					case 1024:
						tempdata[j]=03;
					break;
					case 2048:
						tempdata[j]=04;
					break;
					case 4096:
						tempdata[j]=05;
					break;
					case 8192:
						tempdata[j]=06;
					break;
					case 16384:
						tempdata[j]=07;
					break;
					default:
						tempdata[j]=02;
					break;
				}
				tempclock[j]=0xFF;
				
				if(sectorconfigtab)
				{
					if(sectorconfigtab[interleave_tab[l]].use_alternate_sector_size_id)
					{
						tempdata[j]=sectorconfigtab[interleave_tab[l]].alternate_sector_size_id;
					}
				}
				j++;
				header_size++;

				//02 CRC The sector Header CRC
				CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x1021,0xFFFF);
				for(t=0;t<header_size;t++)  CRC16_Update(&CRC16_High,&CRC16_Low, tempdata[j-header_size+t],(unsigned char*)&crctable );
				
				tempdata[j]=CRC16_High;
				tempclock[j]=0xFF;
				j++;
			
				tempdata[j]=CRC16_Low;
				tempclock[j]=0xFF;
				if(sectorconfigtab)
				{
					if(sectorconfigtab[interleave_tab[l]].use_alternate_header_crc&0x2)
					{
						// alternate crc
						tempdata[j]=(sectorconfigtab[interleave_tab[l]].header_crc&0xFF);
						tempdata[j-1]=(sectorconfigtab[interleave_tab[l]].header_crc>>8)&0xFF; // generate bad header crc	
					}
					else
					{	
						//bad crc
						if(sectorconfigtab[interleave_tab[l]].use_alternate_header_crc&0x1)
						{
							tempdata[j]=tempdata[j-1]^0xFF;
							tempdata[j-1]=tempdata[j]^0xFF;
						}
					}

					
				}
				j++;


				// gap2
				for(k=0;k<configptr->len_gap2;k++)
				{
						tempdata[j]=configptr->data_gap2;
						tempclock[j]=0xFF;
						j++;
				}

				// sync
				for(k=0;k<configptr->len_dsync;k++)
				{
						tempdata[j]=configptr->data_dsync;
						tempclock[j]=0xFF;
						j++;
				}
			
				datapart_size=0;
				// data mark
				for(k=0;k<configptr->len_datamarkp1;k++)
				{
						tempdata[j]=configptr->data_datamarkp1;
						tempclock[j]=configptr->clock_datamarkp1;
						j++;
						datapart_size++;
				}
				for(k=0;k<configptr->len_datamarkp2;k++)
				{

						if(sectorconfigtab)
						{
							if(sectorconfigtab[interleave_tab[l]].use_alternate_datamark)
							{
								tempdata[j]=sectorconfigtab[interleave_tab[l]].alternate_datamark;
								tempclock[j]=configptr->clock_datamarkp2;
								j++;
								datapart_size++;

							}
							else
							{
								tempdata[j]=configptr->data_datamarkp2;
								tempclock[j]=configptr->clock_datamarkp2;
								j++;
								datapart_size++;
							}
						}
						else
						{
							tempdata[j]=configptr->data_datamarkp2;
							tempclock[j]=configptr->clock_datamarkp2;
							j++;
							datapart_size++;
						}
				}

				if(sectorconfigtab)
				{
					sectorconfigtab[interleave_tab[l]].startdataindex=j*2;
				}


				if(!sectorconfigtab)
				{
					//Data  
					for(i=0;i<sectorsize;i++)
					{
						tempdata[j]=datain[(interleave_tab[l]*sectorsize)+i];
						tempclock[j]=0xFF;
						j++;
						datapart_size++;
					}
				
				}
				else
				{
					
					i=0;
					indexbuffer=0;
					while(i<interleave_tab[l])
					{
						indexbuffer=indexbuffer+sectorconfigtab[i].sectorsize;
						i++;
					}

					for(i=0;i<sectorsize;i++)
					{
						tempdata[j]=datain[(indexbuffer)+i];
						tempclock[j]=0xFF;
						j++;
						datapart_size++;
					}
					indexbuffer=indexbuffer+sectorsize;
				}
				
				//02 CRC The CRC of the data 
				CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x1021,0xFFFF);
				
				for(t=0;t<datapart_size;t++)  CRC16_Update(&CRC16_High,&CRC16_Low, tempdata[j-(datapart_size)+t],(unsigned char*)&crctable );
				
				tempdata[j]=CRC16_High;
				tempclock[j]=0xFF;
				j++;
				
				tempdata[j]=CRC16_Low;
				tempclock[j]=0xFF;
				if(sectorconfigtab)
				{

					if(sectorconfigtab[interleave_tab[l]].use_alternate_data_crc&0x2)
					{
						// alternate crc
						tempdata[j]=(sectorconfigtab[interleave_tab[l]].data_crc&0xFF);
						tempdata[j-1]=(sectorconfigtab[interleave_tab[l]].data_crc>>8)&0xFF; // generate bad header crc	
					}
					else
					{	
						//bad crc
						if(sectorconfigtab[interleave_tab[l]].use_alternate_data_crc&0x1)
						{
							tempdata[j]=tempdata[j-1]^0xFF;
							tempdata[j-1]=tempdata[j]^0xFF;
						}
					}

				}
				j++;
				
				//gap3
				for(k=0;k<gap3len;k++)
				{
						tempdata[j]=configptr->data_gap3;
						tempclock[j]=0xFF;
						j++;
				}


		}

		if(j<=current_buffer_size)
		{	
			for(i=j;i<(current_buffer_size+1);i++)
			{	
				tempdata[i]=configptr->data_gap4b;
				tempclock[i]=0xFF;
			}
		}

		if((TRACKTYPE!=IBMFORMAT_SD) && (TRACKTYPE!=ISOFORMAT_SD))		
		{
			BuildCylinder(mfmdata,*mfmsizebuffer,tempclock,tempdata,(*mfmsizebuffer)/2);
		}
		else
		{
			BuildFMCylinder(mfmdata,*mfmsizebuffer,tempclock,tempdata,(*mfmsizebuffer)/4);
		}

		if(interleave_tab) free(interleave_tab);
		free(tempdata);
		free(tempclock);
		return 0;
	 }
	 else
	 {
		if(interleave_tab) free(interleave_tab);
		floppycontext->hxc_printf(MSG_ERROR,"BuildISOTrack : No enough space on this track !");
		return finalsize;
	 }

}
