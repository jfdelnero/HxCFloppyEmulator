/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : variablebitrate.c
// Contains: USB HxC FE data generator
//
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdint.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "variablebitrate.h"
#include "cpld_opcodes.h"

#define BITRATEBLOCKSIZE 128

//#define DEBUGVB 1

static trackzone trackzonebuffer_0[2048*2];
static trackzone trackzonebuffer_1[2048*2];

static trackzone trackzonebuffer_temp[2048*2];

static trackpart trackpartbuffer_0[4096*2];
static trackpart trackpartbuffer_1[4096*2];

static void adjustrand(unsigned char * d, unsigned char * r)
{
	if ( (*d & 0xC0) == 0xC0)
	{
		*d = *d & ~0xC0;
		*r = *r |  0xC0;
	}

	if ( (*d & 0x30) == 0x30)
	{
		*d = *d & ~0x30;
		*r = *r |  0x30;
	}

	if ( (*d & 0x0C) == 0x0C)
	{
		*d = *d & ~0x0C;
		*r = *r |  0x0C;
	}

	if ( (*d & 0x03) == 0x03)
	{
		*d = *d & ~0x03;
		*r = *r |  0x03;
	}
}


unsigned char * realloc_buffer(unsigned char * buffer,uint32_t numberofbit,uint32_t factor)
{
	uint32_t i,j,k,l;
	uint32_t newsize;
	unsigned char * ptr;


	newsize=(numberofbit*factor);

	if(newsize&0x7)
	{
		ptr=malloc(((numberofbit*factor)/8)+1);
		memset(ptr,0,((numberofbit*factor)/8)+1);
	}
	else
	{
		ptr=malloc(((numberofbit*factor)/8));
		memset(ptr,0,((numberofbit*factor)/8));
	}

	l=0;
	k=0;
	j=0;

	if(!(newsize&0x7) && factor == 1)
	{
		memcpy(ptr, buffer, numberofbit/8);
	}
	else
	{
		for(l=0;l<factor;l++)
		{
			for(i=0;i<numberofbit;i++)
			{
				if( (buffer[k>>3]>>(0x7-(k&0x7)))&1)
				{
					ptr[j>>3]=ptr[j>>3]|(0x80>>(j&0x7));
				}
				//else
				//{
				//  ptr[j>>3]=ptr[j>>3]&(~((0x80)>>(j&0x7)));
				//}

				j++;
				if(j>=newsize) j=0;

				k++;
				if(k>=numberofbit) k=0;
			}
		}
	}

	return ptr;
}


uint32_t * realloc_time_buffer(uint32_t * buffer,uint32_t numberofbit,uint32_t factor)
{
	uint32_t i,k,l;
	uint32_t newsize,nbelement;
	uint32_t * ptr;


	if((numberofbit*factor)&0x7)
	{
		newsize=(((numberofbit*factor)/8)+1);
		ptr=malloc((newsize+16)*sizeof(uint32_t));
		memset(ptr,0,(newsize+16)*sizeof(uint32_t));
		nbelement=(((numberofbit)/8)+1);
	}
	else
	{
		newsize=((numberofbit*factor)/8);
		ptr=malloc((newsize+16)*sizeof(uint32_t));
		memset(ptr,0,(newsize+16)*sizeof(uint32_t));
		nbelement=(((numberofbit)/8));
	}

	l=0;
	k=0;

	for(l=0;l<factor;l++)
	{
		for(i=0;i<nbelement;i++)
		{
			ptr[k]=buffer[i];
			k++;
		}
	}

	if(newsize && nbelement)
	{
		l=newsize-1;
		i=nbelement-1;
		while(i && l && !ptr[l])
		{
			ptr[l]=buffer[i];
			l--;
			i--;
		}
	}

	return ptr;
}

int32_t GetNewTrackRevolution(HXCFE* floppycontext,uint8_t * index_h0,uint8_t * datah0,uint32_t lendatah0,uint8_t * datah1,uint32_t lendatah1,uint8_t * randomh0,uint8_t * randomh1,int32_t fixedbitrateh0,uint32_t * timeh0,int32_t fixedbitrateh1,uint32_t * timeh1,uint8_t ** finalbuffer_param,uint8_t ** randomfinalbuffer_param,uint8_t readysignal,uint8_t diskchange,uint8_t writeprotect,uint8_t amigaready,uint8_t selectconfig)
{
	uint32_t i,k,j;

	uint32_t finalsizebuffer;
	unsigned char * finalbuffer;
	unsigned char * randomfinalbuffer;

	int32_t tick_offset_h0;
	int32_t tick_offset_h1;

	uint32_t trackzoneindex0;
	uint32_t trackzoneindex1;

	uint32_t trackparthead0index,trackparthead1index;
	int32_t lencode_track0,lencode_track1;

	unsigned char datah0tmp;
	unsigned char datah1tmp;
	unsigned char randomh0tmp,randomh1tmp;
	unsigned char speedcfg_track0,speedcfg_track1;

	uint32_t numberofzoneh0,numberofzoneh1;
	int32_t deltatick;
	int32_t inserttimecode;
	unsigned char currentindex;

	float time_base;
	uint32_t bloclen;

	//int lencode_track0_error=0;
	//int lencode_track1_error=0;

	int numberofpart,indexstart,newzoneindex,sizefactor;

#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"********************************************************************");
#endif

	if( lendatah0 < 64 || lendatah1 < 64 )
		return 0;

	sizefactor=1;
	if( (lendatah0&7) || (lendatah1&7) )
	{
		sizefactor=8;
	}


	datah0=realloc_buffer(datah0,lendatah0,sizefactor);
	datah1=realloc_buffer(datah1,lendatah1,sizefactor);
	index_h0=realloc_buffer(index_h0,lendatah0,sizefactor);

	if(randomh0)
		randomh0=realloc_buffer(randomh0,lendatah0,sizefactor);
	if(randomh1)
		randomh1=realloc_buffer(randomh1,lendatah1,sizefactor);

	if(timeh0)
		timeh0=realloc_time_buffer(timeh0,lendatah0,sizefactor);

	if(timeh1)
		timeh1=realloc_time_buffer(timeh1,lendatah1,sizefactor);

	lendatah0=((lendatah0*sizefactor)/8);
	lendatah1=((lendatah1*sizefactor)/8);

	trackzoneindex0=0;
	trackzoneindex1=0;

	if(lendatah0>lendatah1)
		finalsizebuffer=(lendatah0*2)+(32000*sizefactor);
	else
		finalsizebuffer=(lendatah1*2)+(32000*sizefactor);

	finalbuffer=malloc(finalsizebuffer);
	memset(finalbuffer,0,finalsizebuffer);
	randomfinalbuffer=malloc(finalsizebuffer);
	memset(randomfinalbuffer,0,finalsizebuffer);

	*finalbuffer_param=finalbuffer;
	*randomfinalbuffer_param=randomfinalbuffer;

	/////////////////////////////////////////////////////////////
	// delimitation des zones de bitrate

	// head 0
	if(fixedbitrateh0==VARIABLEBITRATE)
	{
		trackzonebuffer_0[0].bitrate=timeh0[0];
		trackzonebuffer_0[0].start=0;
		i=0;
		do
		{
			if(trackzonebuffer_0[trackzoneindex0].bitrate!=timeh0[i])
			{

				trackzonebuffer_0[trackzoneindex0].end=i-1;
				/*
				t=trackzonebuffer_0[trackzoneindex0].end-trackzonebuffer_0[trackzoneindex0].start;
				t=t/128;
				for(j=0;j<t;j++)
				{
					trackzonebuffer_0[trackzoneindex0].end=trackzonebuffer_0[trackzoneindex0].start+(128);
					if(trackzoneindex0<2048) trackzoneindex0++;
					trackzonebuffer_0[trackzoneindex0].start=trackzonebuffer_0[trackzoneindex0-1].end+1;
					trackzonebuffer_0[trackzoneindex0].bitrate=timeh0[i-1];
				}
				trackzonebuffer_0[trackzoneindex0].end=i-1;
				*/

				if(trackzoneindex0<2048)trackzoneindex0++;
				trackzonebuffer_0[trackzoneindex0].start=i;
				trackzonebuffer_0[trackzoneindex0].bitrate=timeh0[i];
			}
			i++;
		}while(i<lendatah0);
		trackzonebuffer_0[trackzoneindex0].end=i-1;
	}
	else
	{
		trackzonebuffer_0[0].bitrate=fixedbitrateh0;
		trackzonebuffer_0[0].start=0;
		trackzonebuffer_0[0].end=lendatah0-1;
		trackzoneindex0=0;
	}

#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution : head 0 number of time zone = %d!",trackzoneindex1 );
#endif

	// head 1
	if(fixedbitrateh1==VARIABLEBITRATE)
	{
		trackzonebuffer_1[0].bitrate=timeh1[0];
		trackzonebuffer_1[0].start=0;
		i=0;
		do
		{
			if(trackzonebuffer_1[trackzoneindex1].bitrate!=timeh1[i])
			{
				trackzonebuffer_1[trackzoneindex1].end=i-1;

				/*
				t=trackzonebuffer_1[trackzoneindex1].end-trackzonebuffer_1[trackzoneindex1].start;
				t=t/128;
				for(j=0;j<t;j++)
				{
					trackzonebuffer_1[trackzoneindex1].end=trackzonebuffer_1[trackzoneindex1].start+(128);
					if(trackzoneindex1<2048) trackzoneindex1++;
					trackzonebuffer_1[trackzoneindex1].start=trackzonebuffer_1[trackzoneindex1-1].end+1;
					trackzonebuffer_1[trackzoneindex1].bitrate=timeh1[i-1];
				}
				trackzonebuffer_1[trackzoneindex1].end=i-1;
				*/

				if(trackzoneindex1<2048)trackzoneindex1++;
				trackzonebuffer_1[trackzoneindex1].start=i;
				trackzonebuffer_1[trackzoneindex1].bitrate=timeh1[i];
			}
			i++;
		}while(i<lendatah1);
		trackzonebuffer_1[trackzoneindex1].end=i-1;
	}
	else
	{
		trackzonebuffer_1[0].bitrate=fixedbitrateh1;
		trackzonebuffer_1[0].start=0;
		trackzonebuffer_1[0].end=lendatah1-1;
		trackzoneindex1=0;
	}

#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution : head 1 number of time zone = %d!",trackzoneindex1 );
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// debug
	////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");
	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head0:");
	for(i=0;i<trackzoneindex0+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_0[i].bitrate,
											 trackzonebuffer_0[i].start,
											 trackzonebuffer_0[i].end,
											 trackzonebuffer_0[i].end-trackzonebuffer_0[i].start
											 );
	}

	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head1:");
	for(i=0;i<trackzoneindex1+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_1[i].bitrate,
											 trackzonebuffer_1[i].start,
											 trackzonebuffer_1[i].end,
											 trackzonebuffer_1[i].end-trackzonebuffer_0[i].start
											 );
	}
#endif
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// "etalement"/separation des zones si necessaire

	i=0;
	newzoneindex=0;
	indexstart=0;
	do
	{
		numberofpart=((trackzonebuffer_0[i].end-trackzonebuffer_0[i].start)/BITRATEBLOCKSIZE)+1;


		if(numberofpart)
		{
			for(j=0;j<(uint32_t)numberofpart;j++)
			{

				trackzonebuffer_temp[newzoneindex].bitrate=trackzonebuffer_0[i].bitrate;
				trackzonebuffer_temp[newzoneindex].start=indexstart;
				trackzonebuffer_temp[newzoneindex].end=indexstart+BITRATEBLOCKSIZE-1;
				indexstart=indexstart+BITRATEBLOCKSIZE;
				newzoneindex++;
			}
			trackzonebuffer_temp[newzoneindex-1].end=trackzonebuffer_0[i].end;
			indexstart=trackzonebuffer_temp[newzoneindex-1].end+1;
		}
		else
		{
			trackzonebuffer_temp[newzoneindex].bitrate=trackzonebuffer_0[i].bitrate;
			trackzonebuffer_temp[newzoneindex].start=indexstart;
			trackzonebuffer_temp[newzoneindex].end=trackzonebuffer_0[i].end;
			newzoneindex++;
		}

		i++;
	}while(i<=trackzoneindex0);

	trackzoneindex0=newzoneindex-1;
	memcpy(trackzonebuffer_0,trackzonebuffer_temp,sizeof(trackzonebuffer_0));

	i=0;
	newzoneindex=0;
	indexstart=0;
	do
	{
		numberofpart=((trackzonebuffer_1[i].end-trackzonebuffer_1[i].start)/BITRATEBLOCKSIZE)+1;


		if(numberofpart)
		{
			for(j=0;j<(uint32_t)numberofpart;j++)
			{

				trackzonebuffer_temp[newzoneindex].bitrate=trackzonebuffer_1[i].bitrate;
				trackzonebuffer_temp[newzoneindex].start=indexstart;
				trackzonebuffer_temp[newzoneindex].end=indexstart+BITRATEBLOCKSIZE-1;
				indexstart=indexstart+BITRATEBLOCKSIZE;
				newzoneindex++;
			}
			trackzonebuffer_temp[newzoneindex-1].end=trackzonebuffer_1[i].end;
			indexstart=trackzonebuffer_temp[newzoneindex-1].end+1;
		}
		else
		{
			trackzonebuffer_temp[newzoneindex].bitrate=trackzonebuffer_1[i].bitrate;
			trackzonebuffer_temp[newzoneindex].start=indexstart;
			trackzonebuffer_temp[newzoneindex].end=trackzonebuffer_1[i].end;
			newzoneindex++;
		}

		i++;
	}while(i<=trackzoneindex1);

	trackzoneindex1=newzoneindex-1;
	memcpy(trackzonebuffer_1,trackzonebuffer_temp,sizeof(trackzonebuffer_1));


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// debug
	////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");
	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head0:");
	for(i=0;i<trackzoneindex0+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_0[i].bitrate,
											 trackzonebuffer_0[i].start,
											 trackzonebuffer_0[i].end,
											 trackzonebuffer_0[i].end-trackzonebuffer_0[i].start
											 );
	}

	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head1:");
	for(i=0;i<trackzoneindex1+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_1[i].bitrate,
											 trackzonebuffer_1[i].start,
											 trackzonebuffer_1[i].end,
											 trackzonebuffer_1[i].end-trackzonebuffer_1[i].start
											 );
	}
#endif

	/////////////////////////////////////////////////////////////
	// calcul de la granularit� de chaque zone

	// head 0
	i=0;
	do
	{
		time_base=(float)FLOPPYEMUFREQ/((float)trackzonebuffer_0[i].bitrate);
		bloclen=(trackzonebuffer_0[i].end-trackzonebuffer_0[i].start)+1;

		trackzonebuffer_0[i].code1=(unsigned char)floor(time_base);

		if(time_base-trackzonebuffer_0[i].code1)
		{
			trackzonebuffer_0[i].code1lenint=(uint32_t)((time_base-trackzonebuffer_0[i].code1)*bloclen);

			trackzonebuffer_0[i].code2=(unsigned char)ceil(time_base);
			trackzonebuffer_0[i].code2lenint=(uint32_t)((trackzonebuffer_0[i].code2-time_base)*bloclen);
		}
		else
		{
			trackzonebuffer_0[i].code1lenint=0;
			trackzonebuffer_0[i].code2lenint=bloclen;
		}

		//correction:
		if((trackzonebuffer_0[i].code2lenint+trackzonebuffer_0[i].code1lenint)!=bloclen)
		{
			if(trackzonebuffer_0[i].code2lenint<trackzonebuffer_0[i].code1lenint)
			{
				trackzonebuffer_0[i].code2lenint++;
			}
			else
			{
				trackzonebuffer_0[i].code1lenint++;
			}
		}

		i++;

	}while(i<=trackzoneindex0);

	// head 1
	i=0;
	do
	{
		time_base=(float)FLOPPYEMUFREQ/((float)trackzonebuffer_1[i].bitrate);
		bloclen=(trackzonebuffer_1[i].end-trackzonebuffer_1[i].start)+1;

		trackzonebuffer_1[i].code1=(unsigned char)floor(time_base);

		if(time_base-trackzonebuffer_1[i].code1)
		{
			trackzonebuffer_1[i].code1lenint=(int)((time_base-trackzonebuffer_1[i].code1)*bloclen);

			trackzonebuffer_1[i].code2=(unsigned char)ceil(time_base);
			trackzonebuffer_1[i].code2lenint=(int)((trackzonebuffer_1[i].code2-time_base)*bloclen);

		}
		else
		{
			trackzonebuffer_1[i].code1lenint=0;
			trackzonebuffer_1[i].code2lenint=bloclen;
		}

		//correction:
		if((trackzonebuffer_1[i].code2lenint+trackzonebuffer_1[i].code1lenint)!=bloclen)
		{
			if(trackzonebuffer_1[i].code2lenint<trackzonebuffer_1[i].code1lenint)
			{
				trackzonebuffer_1[i].code2lenint++;
			}
			else
			{
				trackzonebuffer_1[i].code1lenint++;
			}
		}

		i++;

	}while(i<=trackzoneindex1);

	/////////////////////////////////////////////////////////////
	// construction de la liste des zone a g�n�rer.

	// head 0
	i=0;
	j=0;
	do
	{
		if(trackzonebuffer_0[i].code2lenint)
		{
			trackpartbuffer_0[j].code=trackzonebuffer_0[i].code1;
			trackpartbuffer_0[j].len=trackzonebuffer_0[i].code2lenint;
			if(j<(2047*2))
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GetNewTrackRevolution : trackpartbuffer_0 overrun !");

		}

		if(trackzonebuffer_0[i].code1lenint)
		{
			trackpartbuffer_0[j].code=trackzonebuffer_0[i].code2;
			trackpartbuffer_0[j].len=trackzonebuffer_0[i].code1lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GetNewTrackRevolution : trackpartbuffer_0 overrun !");
		}

		i++;
	}while(i<=trackzoneindex0);
	numberofzoneh0=j;

	// head 1
	i=0;
	j=0;
	do
	{
		if(trackzonebuffer_1[i].code2lenint)
		{
			trackpartbuffer_1[j].code=trackzonebuffer_1[i].code1;
			trackpartbuffer_1[j].len=trackzonebuffer_1[i].code2lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GetNewTrackRevolution : trackpartbuffer_1 overrun !");
		}

		if(trackzonebuffer_1[i].code1lenint)
		{
			trackpartbuffer_1[j].code=trackzonebuffer_1[i].code2;
			trackpartbuffer_1[j].len=trackzonebuffer_1[i].code1lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GetNewTrackRevolution : trackpartbuffer_1 overrun !");
		}

		i++;
	}while(i<=trackzoneindex1);
	numberofzoneh1=j;

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// debug
	////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");
	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head0:");
	j=0;
	k=0;
	for(i=0;i<numberofzoneh0;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"code %d len %d (%d)",trackpartbuffer_0[i].code,trackpartbuffer_0[i].len,j);
		j=j+trackpartbuffer_0[i].len;
		k=k+(trackpartbuffer_0[i].code*625)*trackpartbuffer_0[i].len*4;
		k=k%finalsizebuffer;
	}
	floppycontext->hxc_printf(MSG_DEBUG,"Total track Head0: %d timing : %dns",j,k/10);


	floppycontext->hxc_printf(MSG_DEBUG,"GetNewTrackRevolution: Head1:");
	j=0;
	k=0;
	for(i=0;i<numberofzoneh1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"code %d len %d (%d)",trackpartbuffer_1[i].code,trackpartbuffer_1[i].len,j);
		j=j+trackpartbuffer_1[i].len;
		k=k+(trackpartbuffer_1[i].code*625)*trackpartbuffer_1[i].len*4;
		k=k%finalsizebuffer;
	}
	floppycontext->hxc_printf(MSG_DEBUG,"Total track Head1: %d timing : %dns",j,k/10);
#endif
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////
	// fusion des 2 faces avec insertion des opcodes de controle du cpld

	i=0;
	j=0;
	k=0;
	trackparthead0index=0;
	trackparthead1index=0;
	lencode_track0=0;
	lencode_track1=0;
	tick_offset_h0=0;
	tick_offset_h1=0;
	speedcfg_track0=0;
	speedcfg_track1=0;
	//lencode_track0_error=0;
	//lencode_track1_error=0;

	currentindex=index_h0[lendatah0-1];

	inserttimecode=0;
	do
	{
		// opcode d'index a inserer ?
		if(i<lendatah0)
		{
			if((index_h0[i] && !currentindex) || (!index_h0[i] && currentindex))
			{
				finalbuffer[k]=SETINDEX_H0_OPCODE | SETINDEX_H1_OPCODE;
				k=(k+1)%finalsizebuffer;

				if(index_h0[i])
				{
					finalbuffer[k]=1;
					currentindex=1;
				}
				else
				{
					finalbuffer[k]=0;
					currentindex=0;
				}
				finalbuffer[k]=finalbuffer[k] | ((readysignal<<1)&0x2);
				finalbuffer[k]=finalbuffer[k] | ((diskchange<<2)&0x4);
				finalbuffer[k]=finalbuffer[k] | ((writeprotect<<3)&0x8);
				finalbuffer[k]=finalbuffer[k] | ((amigaready<<4)&0x10);
				finalbuffer[k]=finalbuffer[k] | ((selectconfig<<5)&0xE0);
				k=(k+1)%finalsizebuffer;
			}
		}

		if(((lencode_track0<=0) && (lencode_track1<=0)) && (trackpartbuffer_0[trackparthead0index].code==trackpartbuffer_1[trackparthead1index].code) )
		{

			speedcfg_track0=trackpartbuffer_0[trackparthead0index].code;
			lencode_track0=trackpartbuffer_0[trackparthead0index].len;
			trackparthead0index++;

			speedcfg_track1=trackpartbuffer_1[trackparthead1index].code;
			lencode_track1=trackpartbuffer_1[trackparthead1index].len;
			trackparthead1index++;

			finalbuffer[k]=SETBITRATE_H0_OPCODE | SETBITRATE_H1_OPCODE;
			k=(k+1)%finalsizebuffer;
			finalbuffer[k]=speedcfg_track0-2;
			k=(k+1)%finalsizebuffer;
		}
		else
		{

			// faut t'il configurer le bitrate en face 0 ? (changement de zone de bitrate)
			if(lencode_track0<=0)
			{
				if(trackparthead0index<numberofzoneh0)
				{
					speedcfg_track0=trackpartbuffer_0[trackparthead0index].code;
					lencode_track0=trackpartbuffer_0[trackparthead0index].len;
					trackparthead0index++;

					if(speedcfg_track0!=speedcfg_track1)
						finalbuffer[k]=SETBITRATE_H0_OPCODE | NOP_H1_OPCODE;
					else
						finalbuffer[k]=SETBITRATE_H0_OPCODE | SETBITRATE_H1_OPCODE;

					k=(k+1)%finalsizebuffer;
					finalbuffer[k]=speedcfg_track0-2;
					k=(k+1)%finalsizebuffer;

				}
			}

			// faut t'il configurer le bitrate en face 1 ? (changement de zone de bitrate)
			if(lencode_track1<=0)
			{
				if(trackparthead1index<numberofzoneh1)
				{
					speedcfg_track1=trackpartbuffer_1[trackparthead1index].code;
					lencode_track1=trackpartbuffer_1[trackparthead1index].len;
					trackparthead1index++;

					if(speedcfg_track0!=speedcfg_track1)
						finalbuffer[k]=NOP_H0_OPCODE | SETBITRATE_H1_OPCODE;
					else
						finalbuffer[k]=SETBITRATE_H0_OPCODE | SETBITRATE_H1_OPCODE;

					k=(k+1)%finalsizebuffer;
					finalbuffer[k]=speedcfg_track1-2;
					k=(k+1)%finalsizebuffer;
				}
			}

		}

		// faut t'il configurer les bitrates (programmation periodique) ?
		if(!inserttimecode)
		{
			inserttimecode=GRANULA;
		/*  if(speedcfg_track1==speedcfg_track0)
			{
				finalbuffer[k]=SETBITRATE_H0_OPCODE | SETBITRATE_H1_OPCODE;
				k++;
				finalbuffer[k]=speedcfg_track1-2;
				k++;
			}
			else
			{
				finalbuffer[k]=SETBITRATE_H0_OPCODE | NOP_H1_OPCODE;
				k++;
				finalbuffer[k]=speedcfg_track0-2;
				k++;
				finalbuffer[k]=NOP_H0_OPCODE | SETBITRATE_H1_OPCODE;
				k++;
				finalbuffer[k]=speedcfg_track1-2;
				k++;
			}*/
		}
		inserttimecode--;


		deltatick=tick_offset_h0-tick_offset_h1;


		// si pas de decalage temporel entre les 2 face : pas de compensation
		if(!((abs(deltatick)>(speedcfg_track0*4)) || (abs(deltatick)>(speedcfg_track1*4))))
		{

			datah0tmp=0;
			randomh0tmp=0;
			datah1tmp=0;
			randomh1tmp=0;

			// marque comme random (flaky bits) ?
			if(i<lendatah0)
			{
				datah0tmp=datah0[i];
				if(randomh0)
				{
					randomh0tmp=randomh0[i];
				}

				adjustrand(&datah0tmp, &randomh0tmp);

			}
			else
			{
				datah0tmp=NOP_H1_OPCODE|NOP_H0_OPCODE;
			}

			if(j<lendatah1)
			{
				datah1tmp=datah1[j];
				if(randomh1)
				{
					randomh1tmp=randomh1[j];
				}

				adjustrand(&datah1tmp, &randomh1tmp);

			}
			else
			{
				datah1tmp=NOP_H1_OPCODE|NOP_H0_OPCODE;
			}

			// fusion des 2 faces
			finalbuffer[k]=(datah0tmp&0xf0) | (datah1tmp>>4);
			randomfinalbuffer[k]=(randomh0tmp&0xf0) | (randomh1tmp>>4);
			k=(k+1)%finalsizebuffer;
			finalbuffer[k]=((datah0tmp<<4)&0xf0) | (datah1tmp&0x0f);
			randomfinalbuffer[k]=((randomh0tmp<<4)&0xf0) | (randomh1tmp&0x0f);
			k=(k+1)%finalsizebuffer;

			tick_offset_h0=tick_offset_h0+((speedcfg_track0)*4);
			tick_offset_h1=tick_offset_h1+((speedcfg_track1)*4);

			if(lencode_track0>0)
			{
				lencode_track0--;
				//lencode_track0_error=0;
				i++;
			}
			else
			{
				//lencode_track0_error++;
			}

			if(lencode_track1>0)
			{
				lencode_track1--;
				//lencode_track1_error=0;
				j++;
			}
			else
			{
				//lencode_track1_error++;
			}

		}
		else
		{
			if(deltatick>=0) // track h0 plus lente
			{
				// track 0 plus loin que trask 1 (bitrate track 0 < bitrate track 1)
				// inserer nop sur track 0
				// compense h0

				datah1tmp=0;
				randomh1tmp=0;

				if(j<lendatah1)
				{
					datah1tmp=datah1[j];
					if(randomh1)
					randomh1tmp=randomh1[j];
				}
				else
				{
					datah1tmp=NOP_H1_OPCODE|NOP_H0_OPCODE;
				}


				finalbuffer[k]=(NOP_H0_OPCODE) | (datah1tmp>>4);
				randomfinalbuffer[k]=(randomh1tmp>>4);
				k=(k+1)%finalsizebuffer;
				finalbuffer[k]=(NOP_H0_OPCODE) | (datah1tmp&0x0f);
				randomfinalbuffer[k]=(randomh1tmp&0x0f);
				k=(k+1)%finalsizebuffer;

				tick_offset_h1=tick_offset_h1+((speedcfg_track1)*4);

				if(lencode_track1>0)
				{
					lencode_track1--;
					//lencode_track1_error=0;
					j++;
				}
				else
				{
					//lencode_track1_error++;
				}
			}
			else
			{
				//(4*3) 00000000000000000000
				//(4*6) 1N1N1N1N1N1N1N1N1N1N

				// track 1 plus loin que trask 0 (bitrate track 1 < bitrate track 0)
				// inserer nop sur track 1
				// compense h1

				datah0tmp=0;
				randomh0tmp=0;

				if(i<lendatah0)
				{
					datah0tmp=datah0[i];
					if(randomh0)
					{
						randomh0tmp=randomh0[i];
					}
				}
				else
				{
					datah0tmp=NOP_H1_OPCODE|NOP_H0_OPCODE;
				}

				finalbuffer[k]=(datah0tmp&0xf0) | (NOP_H1_OPCODE);
				randomfinalbuffer[k]=(randomh0tmp&0xf0);
				k=(k+1)%finalsizebuffer;
				finalbuffer[k]=((datah0tmp<<4)&0xf0) | (NOP_H1_OPCODE);
				randomfinalbuffer[k]=((randomh0tmp<<4)&0xf0);
				k=(k+1)%finalsizebuffer;

				tick_offset_h0=tick_offset_h0+((speedcfg_track0)*4);

				if(lencode_track0>0)
				{
					lencode_track0--;
					//lencode_track0_error=0;
					i++;
				}
				else
				{
					//lencode_track0_error++;
				}
			}
		}

	}while(!((trackparthead0index>=numberofzoneh0 && trackparthead1index>=numberofzoneh1) && (!(lencode_track1>0) && !(lencode_track0>0))));

	free(datah0);
	free(datah1);
	free(index_h0);

	if(randomh0)
		free(randomh0);
	if(randomh1)
		free(randomh1);

	if(timeh0)
		free(timeh0);

	if(timeh1)
		free(timeh1);

	return k;
}


// fonction de debugage...
#if 0
int checkbuffer(unsigned char * buffer,int sizebf)
{
	int i;
	int size1,size0;
	int cmd_index0,cmd_nop0,cmd_setspeed0;
	int cmd_index1,cmd_nop1,cmd_setspeed1;
	unsigned char c;
	static FILE *hfileh0=0;
	static FILE *hfileh1=0;
	static unsigned char *buf0=0;
	static unsigned char *buf1=0;
	char  dbugfile[1*1024];


	if(!hfileh0)
	{
		sprintf(dbugfile,"d:\\h0.bin");
		hfileh0=fopen(dbugfile,"w");
		buf0=malloc(1024*1024);
	}

	if(!hfileh1)
	{
		sprintf(dbugfile,"d:\\h1.bin");
		hfileh1=fopen(dbugfile,"w");
		buf1=malloc(1024*1024);
	}

	size0=0;
	size1=0;
	cmd_index0=0;
	cmd_index1=0;
	cmd_nop0=0;
	cmd_nop1=0;
	cmd_setspeed0=0;
	cmd_setspeed1=0;

	// head 0
	i=0;
	do
	{
		c=buffer[i];
		c=c&0xF;

		switch(c)
		{

			case SETINDEX_H1_OPCODE:
				i=i+2;
				cmd_index0++;
				break;
			case SETBITRATE_H1_OPCODE:
				cmd_setspeed0++;
				i=i+2;
				break;
			case NOP_H1_OPCODE:
				cmd_nop0++;
				i=i+2;
				break;
			default:
				if(size0&1)
					buf0[size0>>1]=buf0[size0>>1] | c;
				else
					buf0[size0>>1]= (c<<4);

				size0++;
				i++;
				break;
		}

	}while(i<sizebf);
	/////////////////////////////////////
	// head 1

	i=0;
	do
	{
		c=buffer[i];
		c=(c>>4)&0xF;

		switch(c)
		{

			case SETINDEX_H1_OPCODE:
				i=i+2;
				cmd_index1++;
				break;
			case SETBITRATE_H1_OPCODE:
				cmd_setspeed1++;
				i=i+2;
				break;
			case NOP_H1_OPCODE:
				cmd_nop1++;
				i=i+2;
				break;
			default:
				if(size1&1)
					buf1[size1>>1]=buf1[size1>>1] |c;
				else
					buf1[size1>>1]= (c<<4);

				size1++;
				i++;
				break;
		}

	}while(i<sizebf);

	size0=size0/2;
	size1=size1/2;

	#ifdef DEBUGMODE
		fwrite(buf0,size0,1,hfileh0);
		fwrite(buf1,size1,1,hfileh1);
	#endif

 return 0;
}
#endif
