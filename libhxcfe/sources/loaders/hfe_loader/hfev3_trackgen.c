/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : hfev3_trackgen.c
// Contains: HFE V3 track generator
//
// Written by: Jean-François DEL NERO
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

#include "hfev3_trackgen.h"

#define BITRATEBLOCKSIZE 128

//#define DEBUGVB 1

trackzone trackzonebuffer_0[2048*2];
trackzone trackzonebuffer_1[2048*2];

trackzone trackzonebuffer_temp[2048*2];

trackpart trackpartbuffer_0[4096*2];
trackpart trackpartbuffer_1[4096*2];

void adjustrand(unsigned char * d, unsigned char * r)
{
	if ( ((*d & 0xF0) == 0xF0) && (*d != RAND_OPCODE) ) // Shouldn't match with an opcode at this point !
	{
		*d = *d ^ 0x90;
	}

	return;
}

int32_t GenOpcodesTrack(HXCFE* floppycontext,uint8_t * index_h0,uint8_t * datah0,uint32_t lendatah0,uint8_t * datah1,uint32_t lendatah1,uint8_t * randomh0,uint8_t * randomh1,int32_t fixedbitrateh0,uint32_t * timeh0,int32_t fixedbitrateh1,uint32_t * timeh1,uint8_t ** finalbuffer_H0_param,uint8_t ** finalbuffer_H1_param,uint8_t ** randomfinalbuffer_param)
{
	uint32_t i,k,j;

	uint32_t finalsizebuffer;

	unsigned char * finalbuffer_H0;
	unsigned char * randomfinalbuffer_H0;
	unsigned char * finalbuffer_H1;

	uint8_t prev_randomh0,prev_randomh1;

	int32_t tick_offset_h0;
	int32_t tick_offset_h1;

	uint32_t trackzoneindex0;
	uint32_t trackzoneindex1;

	uint32_t trackparthead0index,trackparthead1index;
	int32_t lencode_track0,lencode_track1;
	int32_t lenbitdatah0,lenbitdatah1;
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

	int lencode_track0_error=0;
	int lencode_track1_error=0;

	int numberofpart,indexstart,newzoneindex;


#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"********************************************************************");
#endif

	lenbitdatah0 = lendatah0;
	lenbitdatah1 = lendatah1;

	lendatah0 = (lendatah0/8);
	lendatah1 = (lendatah1/8);

	trackzoneindex0=0;
	trackzoneindex1=0;

	prev_randomh0 = 0;
	prev_randomh1 = 0;

	if(lendatah0>lendatah1)
		finalsizebuffer = (lendatah0)+(32000);
	else
		finalsizebuffer = (lendatah1)+(32000);

	finalbuffer_H0 = malloc(finalsizebuffer);
	finalbuffer_H1 = malloc(finalsizebuffer);
	randomfinalbuffer_H0 = malloc(finalsizebuffer);

	if(!finalbuffer_H0 || !finalbuffer_H1 || !randomfinalbuffer_H0)
	{
		free(finalbuffer_H0);
		free(finalbuffer_H1);
		free(randomfinalbuffer_H0);

		return 0;
	}

	memset(finalbuffer_H0,0,finalsizebuffer);
	memset(finalbuffer_H1,0,finalsizebuffer);
	memset(randomfinalbuffer_H0,0,finalsizebuffer);

	*finalbuffer_H0_param = finalbuffer_H0;
	*finalbuffer_H1_param = finalbuffer_H1;
	*randomfinalbuffer_param = randomfinalbuffer_H0;

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
		}while( i < lendatah0 );
		trackzonebuffer_0[trackzoneindex0].end=i-1;
	}
	else
	{
		trackzonebuffer_0[0].bitrate = fixedbitrateh0;
		trackzonebuffer_0[0].start = 0;
		trackzonebuffer_0[0].end = lendatah0-1;
		trackzoneindex0 = 0;
	}

#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack : head 0 number of time zone = %d!",trackzoneindex0 );
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
	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack : head 1 number of time zone = %d!",trackzoneindex1 );
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// debug
	////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");
	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head0:");
	for(i=0;i<trackzoneindex0+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_0[i].bitrate,
											 trackzonebuffer_0[i].start,
											 trackzonebuffer_0[i].end,
											 trackzonebuffer_0[i].end-trackzonebuffer_0[i].start
											 );
	}

	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head1:");
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
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// "etalement"/separation des zones si necessaire

	i=0;
	newzoneindex=0;
	indexstart=0;
	do
	{
		numberofpart=((trackzonebuffer_0[i].end-trackzonebuffer_0[i].start)/BITRATEBLOCKSIZE)+1;


		if(numberofpart > 0)
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

		if(numberofpart > 0)
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
	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head0:");
	for(i=0;i<trackzoneindex0+1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"bitrate %d -  %.4x:%.4x (%d)",
											 trackzonebuffer_0[i].bitrate,
											 trackzonebuffer_0[i].start,
											 trackzonebuffer_0[i].end,
											 trackzonebuffer_0[i].end-trackzonebuffer_0[i].start
											 );
	}

	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head1:");
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
	// calcul de la granularité de chaque zone

	// head 0
	i=0;
	do
	{
		time_base=(float)FLOPPYEMUFREQ/((float)trackzonebuffer_0[i].bitrate*2);
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
		time_base=(float)FLOPPYEMUFREQ/((float)trackzonebuffer_1[i].bitrate*2);
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
	// construction de la liste des zone a générer.

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
				floppycontext->hxc_printf(MSG_ERROR,"GenOpcodesTrack : trackpartbuffer_0 overrun !");
		}

		if(trackzonebuffer_0[i].code1lenint)
		{
			trackpartbuffer_0[j].code=trackzonebuffer_0[i].code2;
			trackpartbuffer_0[j].len=trackzonebuffer_0[i].code1lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GenOpcodesTrack : trackpartbuffer_0 overrun !");
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
			trackpartbuffer_1[j].code = trackzonebuffer_1[i].code1;
			trackpartbuffer_1[j].len = trackzonebuffer_1[i].code2lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GenOpcodesTrack : trackpartbuffer_1 overrun !");
		}

		if(trackzonebuffer_1[i].code1lenint)
		{
			trackpartbuffer_1[j].code = trackzonebuffer_1[i].code2;
			trackpartbuffer_1[j].len = trackzonebuffer_1[i].code1lenint;
			if(j<2047*2)
				j++;
			else
				floppycontext->hxc_printf(MSG_ERROR,"GenOpcodesTrack : trackpartbuffer_1 overrun !");
		}

		i++;
	}while(i<=trackzoneindex1);
	numberofzoneh1=j;

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// debug
	////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUGVB
	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");
	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head0: %d zones",numberofzoneh0);
	j=0;
	k=0;
	for(i=0;i<numberofzoneh0;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"code %d len %d (Ofs:%d) [P:%d]",trackpartbuffer_0[i].code,trackpartbuffer_0[i].len,j,i);
		j=j+trackpartbuffer_0[i].len;
		k=k+(trackpartbuffer_0[i].code*625)*trackpartbuffer_0[i].len*4;
		k=k%finalsizebuffer;
	}
	floppycontext->hxc_printf(MSG_DEBUG,"Total track Head0: %d timing : %dns",j,k/10);

	floppycontext->hxc_printf(MSG_DEBUG,"GenOpcodesTrack: Head1: %d zones",numberofzoneh1);

	j=0;
	k=0;
	for(i=0;i<numberofzoneh1;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"code %d len %d (Ofs:%d) [P:%d]",trackpartbuffer_1[i].code,trackpartbuffer_1[i].len,j,i);
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
	lencode_track0_error=0;
	lencode_track1_error=0;

	currentindex = 0;

	inserttimecode=0;

	// Insert bit shift if needed.

	if(lenbitdatah0 & 7 || lenbitdatah1 & 7)
	{
		if( (lenbitdatah0 & 7) && !(lenbitdatah1 & 7) )
		{
#ifdef DEBUGVB
			floppycontext->hxc_printf(MSG_DEBUG,"SKIPBITS_OPCODE (0X) (Ofs: %d)", k);
#endif
			finalbuffer_H0[k] = SKIPBITS_OPCODE;
			finalbuffer_H1[k] = NOP_OPCODE;

			k=(k+1)%finalsizebuffer;

			finalbuffer_H0[k] = 8 - (lenbitdatah0&7);
			finalbuffer_H1[k] = NOP_OPCODE;

			k=(k+1)%finalsizebuffer;

			// Copy last bits
			finalbuffer_H0[k] = datah0[(lenbitdatah0>>3)] >> (8 - (lenbitdatah0&7));
			finalbuffer_H1[k] = NOP_OPCODE;

			k=(k+1)%finalsizebuffer;

		}

		if( !(lenbitdatah0 & 7) && (lenbitdatah1 & 7) )
		{
#ifdef DEBUGVB
			floppycontext->hxc_printf(MSG_DEBUG,"SKIPBITS_OPCODE (X1) (Ofs: %d)", k);
#endif

			finalbuffer_H0[k] = NOP_OPCODE;
			finalbuffer_H1[k] = SKIPBITS_OPCODE;

			k=(k+1)%finalsizebuffer;

			finalbuffer_H0[k] = NOP_OPCODE;
			finalbuffer_H1[k] = 8 - (lenbitdatah1&7);

			k=(k+1)%finalsizebuffer;

			// Copy last bits
			finalbuffer_H0[k] = NOP_OPCODE;
			finalbuffer_H1[k] = datah1[(lenbitdatah1>>3)] >> (8 - (lenbitdatah1&7));

			k=(k+1)%finalsizebuffer;
		}

		if( (lenbitdatah0 & 7) && (lenbitdatah1 & 7) )
		{
#ifdef DEBUGVB
			floppycontext->hxc_printf(MSG_DEBUG,"SKIPBITS_OPCODE (01) (Ofs: %d)", k);
#endif
			finalbuffer_H0[k] = SKIPBITS_OPCODE;
			finalbuffer_H1[k] = SKIPBITS_OPCODE;

			k=(k+1)%finalsizebuffer;

			finalbuffer_H0[k] = 8 - (lenbitdatah0&7);
			finalbuffer_H1[k] = 8 - (lenbitdatah1&7);

			k=(k+1)%finalsizebuffer;

			// Copy last bits
			finalbuffer_H0[k] = datah0[(lenbitdatah0>>3)] >> (8 - (lenbitdatah0&7));
			finalbuffer_H1[k] = datah1[(lenbitdatah1>>3)] >> (8 - (lenbitdatah1&7));

			k=(k+1)%finalsizebuffer;
		}
	}

	do
	{
		// opcode d'index a inserer ?
		if(i<lendatah0)
		{
			if( index_h0[i] && !currentindex )
			{
#ifdef DEBUGVB
				floppycontext->hxc_printf(MSG_DEBUG,"SETINDEX_OPCODE (01) (Ofs: %d)", k);
#endif
				finalbuffer_H0[k] = SETINDEX_OPCODE;
				finalbuffer_H1[k] = SETINDEX_OPCODE;

				k=(k+1)%finalsizebuffer;

				if(index_h0[i])
				{
					currentindex=1;
				}
				else
				{
					currentindex=0;
				}
			}
			else
			{
				if(!index_h0[i] && currentindex)
				{
					currentindex = 0;
				}
			}
		}

		if( (lencode_track0<=0) && (lencode_track1<=0) )
		{
			if(trackparthead0index<numberofzoneh0)
			{
				lencode_track0  = trackpartbuffer_0[trackparthead0index].len;
				speedcfg_track0 = trackpartbuffer_0[trackparthead0index].code;
				trackparthead0index++;
			}

			if(trackparthead1index<numberofzoneh1)
			{
				lencode_track1  = trackpartbuffer_1[trackparthead1index].len;
				speedcfg_track1 = trackpartbuffer_1[trackparthead1index].code;
				trackparthead1index++;
			}

			#ifdef DEBUGVB
			if( !speedcfg_track0 || !speedcfg_track1 )
			{
				floppycontext->hxc_printf(MSG_DEBUG,"Invalid speed state ! : H0- P:%d L:%d S:%d --- H1- P:%d L:%d S:%d",trackparthead0index-1,lencode_track0,speedcfg_track0,trackparthead1index-1,lencode_track1,speedcfg_track1);
			}
			#endif

#ifdef DEBUGVB
			floppycontext->hxc_printf(MSG_DEBUG,"SETBITRATE_OPCODE (01) (Ofs: %d) (%d - %d)", k, speedcfg_track0, speedcfg_track1);
#endif

			finalbuffer_H0[k] = SETBITRATE_OPCODE;
			finalbuffer_H1[k] = SETBITRATE_OPCODE;
			k=(k+1)%finalsizebuffer;

			finalbuffer_H0[k] = speedcfg_track0;
			finalbuffer_H1[k] = speedcfg_track1;
			k=(k+1)%finalsizebuffer;
		}
		else
		{

			// faut t'il configurer le bitrate en face 0 ? (changement de zone de bitrate)
			if(lencode_track0<=0)
			{
				if(trackparthead0index<numberofzoneh0)
				{
					lencode_track0 = trackpartbuffer_0[trackparthead0index].len;
					speedcfg_track0 = trackpartbuffer_0[trackparthead0index].code;
					trackparthead0index++;

					#ifdef DEBUGVB
					if( !speedcfg_track0 )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Invalid speed state ! : H0-P:%d L:%d S:%d",trackparthead0index-1,lencode_track0,speedcfg_track0);
					}
					#endif

#ifdef DEBUGVB
					floppycontext->hxc_printf(MSG_DEBUG,"SETBITRATE_OPCODE (01) (Ofs: %d) (%d - %d)", k, speedcfg_track0, speedcfg_track1);
#endif

					finalbuffer_H0[k] = SETBITRATE_OPCODE;
					finalbuffer_H1[k] = SETBITRATE_OPCODE;
					k=(k+1)%finalsizebuffer;

					finalbuffer_H0[k] = speedcfg_track0;
					finalbuffer_H1[k] = speedcfg_track1;
					k=(k+1)%finalsizebuffer;
				}
			}

			// faut t'il configurer le bitrate en face 1 ? (changement de zone de bitrate)
			if(lencode_track1<=0)
			{
				if(trackparthead1index<numberofzoneh1)
				{
					lencode_track1 = trackpartbuffer_1[trackparthead1index].len;
					speedcfg_track1 = trackpartbuffer_1[trackparthead1index].code;
					trackparthead1index++;

					#ifdef DEBUGVB
					if( !speedcfg_track1 )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Invalid speed state ! : H1-P:%d L:%d S:%d",trackparthead1index-1,lencode_track1,speedcfg_track1);
					}
					#endif

#ifdef DEBUGVB
					floppycontext->hxc_printf(MSG_DEBUG,"SETBITRATE_OPCODE (01) (Ofs: %d) (%d - %d)", k, speedcfg_track0, speedcfg_track1);
#endif

					finalbuffer_H0[k] = SETBITRATE_OPCODE;
					finalbuffer_H1[k] = SETBITRATE_OPCODE;
					k=(k+1)%finalsizebuffer;

					finalbuffer_H0[k] = speedcfg_track0;
					finalbuffer_H1[k] = speedcfg_track1;
					k=(k+1)%finalsizebuffer;
				}
			}
		}

		// faut t'il configurer les bitrates (programmation periodique) ?
		if(!inserttimecode)
		{
			inserttimecode=GRANULA;
		/*	if(speedcfg_track1==speedcfg_track0)
			{
				finalbuffer[k]=SETBITRATE_H0_OPCODE | SETBITRATE_H1_OPCODE;
				k++;
				finalbuffer[k] = speedcfg_track1;
				k++;
			}
			else
			{
				finalbuffer[k]=SETBITRATE_H0_OPCODE | NOP_H1_OPCODE;
				k++;
				finalbuffer[k] = speedcfg_track0;
				k++;
				finalbuffer[k]=NOP_H0_OPCODE | SETBITRATE_H1_OPCODE;
				k++;
				finalbuffer[k] = speedcfg_track1;
				k++;
			}*/
		}

		inserttimecode--;

		deltatick = tick_offset_h0-tick_offset_h1;

		// si pas de decalage temporel entre les 2 face : pas de compensation
		if(!((abs(deltatick)>(speedcfg_track0*8)) || (abs(deltatick)>(speedcfg_track1*8))))
		{

			datah0tmp=0;
			randomh0tmp=0;
			datah1tmp=0;
			randomh1tmp=0;

			// marque comme random (flaky bits) ?
			if(i<lendatah0)
			{
				datah0tmp = datah0[i];

				if(randomh0)
				{
					if(prev_randomh0>0 && randomh0[i])
					{
						datah0tmp = RAND_OPCODE;
					}

					if(randomh0[i])
						prev_randomh0++;
					else
						prev_randomh0 = 0;

					randomh0tmp = randomh0[i];
				}

				adjustrand(&datah0tmp, &randomh0tmp);

			}
			else
			{
				datah0tmp = NOP_OPCODE;
			}

			if(j<lendatah1)
			{
				datah1tmp=datah1[j];
				if(randomh1)
				{
					if(prev_randomh1>0 && randomh1[j])
					{
						datah1tmp = RAND_OPCODE;
					}

					if(randomh1[j])
						prev_randomh1++;
					else
						prev_randomh1 = 0;

					randomh1tmp = randomh1[j];
				}

				adjustrand(&datah1tmp, &randomh1tmp);
			}
			else
			{
				datah1tmp = NOP_OPCODE;
			}

			// fusion des 2 faces
			finalbuffer_H0[k] = datah0tmp;
			finalbuffer_H1[k] = datah1tmp;
			//randomfinalbuffer[k]=(randomh0tmp&0xf0) | (randomh1tmp>>4);
			k=(k+1)%finalsizebuffer;

			tick_offset_h0 += ((speedcfg_track0)*8);
			tick_offset_h1 += ((speedcfg_track1)*8);

			if(lencode_track0>0)
			{
				lencode_track0--;
				lencode_track0_error=0;
				i++;
			}
			else
			{
				lencode_track0_error++;
			}

			if(lencode_track1>0)
			{
				lencode_track1--;
				lencode_track1_error=0;
				j++;
			}
			else
			{
				lencode_track1_error++;
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
					datah1tmp = datah1[j];
					if(randomh1)
					{
						if(prev_randomh1>0 && randomh1[j])
						{
							datah1tmp = RAND_OPCODE;
						}

						if(randomh1[j])
							prev_randomh1++;
						else
							prev_randomh1 = 0;

						randomh1tmp = randomh1[j];
					}

					adjustrand(&datah1tmp, &randomh1tmp);
				}
				else
				{
					datah1tmp = NOP_OPCODE;
				}


				finalbuffer_H0[k] = NOP_OPCODE;
				finalbuffer_H1[k] = datah1tmp;
				//randomfinalbuffer[k]=(randomh1tmp>>4);
				k=(k+1)%finalsizebuffer;

				tick_offset_h1 += ((speedcfg_track1)*8);

				if(lencode_track1>0)
				{
					lencode_track1--;
					lencode_track1_error=0;
					j++;
				}
				else
				{
					lencode_track1_error++;
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
					datah0tmp = datah0[i];
					if(randomh0)
					{
						randomh0tmp = randomh0[i];
						if(prev_randomh0>0 && randomh0[i])
						{
							datah0tmp = RAND_OPCODE;
						}

						if(randomh0[i])
							prev_randomh0++;
						else
							prev_randomh0 = 0;
					}

					adjustrand(&datah0tmp, &randomh0tmp);
				}
				else
				{
					datah0tmp = NOP_OPCODE;
				}

				finalbuffer_H0[k]= datah0tmp;
				finalbuffer_H1[k]= NOP_OPCODE;
				//randomfinalbuffer[k]=(randomh0tmp&0xf0);
				k=(k+1)%finalsizebuffer;

				tick_offset_h0 += ((speedcfg_track0)*8);

				if(lencode_track0>0)
				{
					lencode_track0--;
					lencode_track0_error=0;
					i++;
				}
				else
				{
					lencode_track0_error++;
				}
			}
		}
	}while(!((trackparthead0index>=numberofzoneh0 && trackparthead1index>=numberofzoneh1) && (!(lencode_track1>0) && !(lencode_track0>0))));

	return k;
}
