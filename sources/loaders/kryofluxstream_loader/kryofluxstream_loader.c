/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : KryoFluxStream_loader.c
// Contains: KryoFlux Stream floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "libhxcadaptor.h"

#define BASEINDEX 1

#define MAXPULSESKEW 10

//#define KFSTREAMDBG 1

HXCFLOPPYEMULATOR* floppycont;

int KryoFluxStream_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int found,track,side;
	struct stat staterep;
	char * filepath;
	FILE * f;
	s_oob_header oob;
	char filename[512];

	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libIsValidDiskFile");
	
	if(imgfile)
	{
		memset(&staterep,0,sizeof(struct stat));
		if(!hxc_stat(imgfile,&staterep))
		{
			if(staterep.st_mode&S_IFDIR)
			{
			
				filepath = malloc( strlen(imgfile) + 32 );

				track=0;
				side=0;
				found=0;
				do
				{					
					sprintf(filepath,"%s\\track%.2d.%d.raw",imgfile,track,side);
					f=hxc_fopen(filepath,"rb");
					if(f)
					{
						fread(&oob,sizeof(s_oob_header),1,f);
						if(oob.Sign==OOB_SIGN)
						{
							found=1;
						}
						hxc_fclose(f);
					}
					side++;
					if(side>1) 
					{
						side = 0;
						track++;
					}

				}while(track<84);

				free( filepath );

				if(found)
				{
					return HXCFE_VALIDFILE;
				}
				else
				{
					return HXCFE_BADFILE;
				}

			}
			else
			{
				hxc_getfilenamebase(imgfile,(char*)&filename);
				hxc_strlower((char*)&filename);
				found=0;

				if(!strstr(filename,".0.raw") && !strstr(filename,".1.raw") )
				{
					return HXCFE_BADFILE;
				}

				f=hxc_fopen(imgfile,"rb");
				if(f)
				{
					fread(&oob,sizeof(s_oob_header),1,f);
					if( ( oob.Sign == OOB_SIGN ) && ( oob.Type>=1 && oob.Type<=4 ) )
					{
						found=1;
					}
					hxc_fclose(f);

					if(found)
					{
						return HXCFE_VALIDFILE;
					}
					else
					{
						return HXCFE_BADFILE;
					}
				}
			}
		}
	}
	
	return HXCFE_BADPARAMETER;
}

void settrackbit(unsigned char * dstbuffer,int dstsize,unsigned char byte,int bitoffset,int size)
{
	int i,j,k;

	k=0;
	i=bitoffset;
	for(j=0;j<size;j++)
	{
		if( (i>>3) < dstsize)
		{
			if(byte&((0x80)>>(j&7)))
				dstbuffer[(i>>3)]=dstbuffer[(i>>3)]|( (0x80>>(i&7)));
			else
				dstbuffer[(i>>3)]=dstbuffer[(i>>3)]&(~(0x80>>(i&7)));
		}

		i++;
	}
}

void computehistogram(unsigned long *indata,int size,unsigned long *outdata)
{
	int i;

	memset(outdata,0,sizeof(unsigned long) * (65536) );
	i=0;
	do
	{	
		if(indata[i]<0x10000)
		{
			outdata[indata[i]]++;
		}
		i++;
	}while(i<size);
}

typedef struct stathisto_
{
	unsigned long val;
	unsigned long occurence;
	float pourcent;
}stathisto;

int detectpeaks(HXCFLOPPYEMULATOR* floppycontext,unsigned long *histogram)
{
	int i,k;
	int total;
	int nbval;

	int total250k;
	int total300k;
	int total500k;

	int ret;

	float pourcent250k,pourcent300k,pourcent500k,pourcenttotal;

	stathisto * stattab;

	total=0;
	for(i=0;i<65536;i++)
	{
		total = total + histogram[i];
	}

	nbval=0;
	for(i=0;i<65536;i++)
	{
		if(histogram[i]) nbval++;
	}

	stattab = malloc(sizeof(stathisto) * (nbval+1) );
	memset(stattab,0,sizeof(stathisto) * (nbval+1) );
	
	k=0;
	for(i=0;i<65536;i++)
	{
		if(histogram[i])
		{
			stattab[k].occurence=histogram[i];
			stattab[k].val=i;
			stattab[k].pourcent=((float)stattab[k].occurence/(float)total)*(float)100;		
			k++;
		}
	}
	
#ifdef KFSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"---- Stream values ----");
	for(i=0;i<nbval;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"Val %d : %d - %d",stattab[i].val, stattab[i].occurence , (int)(stattab[i].pourcent*100));
	}
	floppycontext->hxc_printf(MSG_DEBUG,"----------------------");
#endif
	
	total250k=0;
	total300k=0;
	total500k=0;

	i=0;
	pourcent250k = 0;
	pourcent300k = 0;
	pourcent500k = 0;
	pourcenttotal = 0;
	while(i<nbval)
	{
		ret=stattab[i].val;

		pourcenttotal = pourcenttotal + stattab[i].pourcent;

		if(ret<107 && ret>=87)
		{
			total250k=total250k+stattab[i].occurence;
			pourcent250k = pourcent250k + stattab[i].pourcent;
		}

		if(ret<87 && ret>68)
		{
			total300k=total300k+stattab[i].occurence;
			pourcent300k = pourcent300k + stattab[i].pourcent;
		}

		if(ret<55 && ret>42)
		{
			total500k=total500k+stattab[i].occurence;
			pourcent500k = pourcent500k + stattab[i].pourcent;
		}

		i++;
	}


//	return 96;

	if(pourcent500k > 2)
	{
		return 48;
	}

	if(pourcent250k > 2)
	{
		return 96;
	}

	if(pourcent300k > 2)
	{
		return 80;
	}


	free(stattab);

	return 96;
}

typedef struct pll_stat_
{
	// current cell size (1 cell size)
	int pump_charge;
	
	// current window phase 
	int phase;
	
	// center value
	int pll_max;
	int pivot;
	int pll_min;

	int last_error;

	// last pulse phase
	int lastpulsephase;
}pll_stat;

int getCellTiming(pll_stat * pll,int current_pulsevalue,int * badpulse,int t,int overlapval)
{
	//
	// Return: 0 -> pulse in previous window ! (bad)
	//         1 -> pulse in the current window ! (probably bad)
	// 2 or more -> valid pulses
	//

	int blankcell;
	int cur_pll_error,left_boundary,right_boundary,center;
	int current_pulse_position;
	
	blankcell = 0;

	current_pulsevalue = current_pulsevalue * 16;

	//////////////////////////////////////////////////////////////
	left_boundary = pll->phase;
	right_boundary = (pll->phase + pll->pump_charge);
	center = ((pll->phase + (pll->pump_charge/2)));
	current_pulse_position = pll->lastpulsephase + current_pulsevalue;
	//////////////////////////////////////////////////////////////

	pll->last_error = 0xFFFF;

	// the pulse is before the current window ?
	if( current_pulse_position < left_boundary )
	{
		
//		cur_pll_error = (pll->lastpulsephase + current_pulsevalue) - ((pll->phase + (pll->pump_charge/2)));

		pll->lastpulsephase = pll->lastpulsephase + current_pulsevalue;
		
//		*pumpcharge = *pumpcharge - 8;
		
//		*window_phase = *lastpulsephase - (*pumpcharge/2);

		if(badpulse)
			*badpulse = 1;

		//floppycont->hxc_printf(MSG_DEBUG,"ValIn:%d Nbcell:%d Pumpcharche:%d Window Phase:%d LastPulsePhase:%d Err:%d Bit:%d",current_pulsevalue,blankcell,pll->pump_charge,pll->phase,pll->lastpulsephase,cur_pll_error,t);
		return blankcell;
	}
	else
	{
		blankcell = 1;
	}


	blankcell = 1;
	// the pulse is after the current window ?
	while( current_pulse_position > right_boundary)
	{
		// Jump to the next window
		pll->phase = pll->phase + pll->pump_charge;

		left_boundary = pll->phase;
		right_boundary = (pll->phase + pll->pump_charge);
		center = ((pll->phase + (pll->pump_charge/2)));

		blankcell = blankcell + 1;
	}

	// Is the pulse before or after the middle of the window ?
	// negative value : clock too slow - positive value : clock too fast
	cur_pll_error = current_pulse_position - center;

	if(overlapval)
	{
		if(pll->pump_charge < pll->pivot/2 )
		{
			if(cur_pll_error<0)
				pll->pump_charge = ( (pll->pump_charge*31) + (pll->pump_charge+ cur_pll_error ) ) / 32;
			else
				pll->pump_charge = ( (pll->pump_charge*15) + (pll->pump_charge+ cur_pll_error ) ) / 16;
		}
		else
		{
			if(cur_pll_error<0)
				pll->pump_charge = ( (pll->pump_charge*15) + (pll->pump_charge+ cur_pll_error ) ) / 16;
			else
				pll->pump_charge = ( (pll->pump_charge*31) + (pll->pump_charge+ cur_pll_error ) ) / 32;
		}
			

		if( pll->pump_charge < (pll->pll_min/2) )
		{
			pll->pump_charge = (pll->pll_min/2);
		}

		if( pll->pump_charge > (pll->pll_max/2) )
		{
			pll->pump_charge = (pll->pll_max/2);
		}

		// Phase adjustement
		pll->phase = pll->phase + (cur_pll_error/16);
	}
	
	// next window
	pll->phase = pll->phase + pll->pump_charge;
	pll->lastpulsephase = pll->lastpulsephase + current_pulsevalue;

	pll->last_error = cur_pll_error;

	//floppycont->hxc_printf(MSG_DEBUG,"ValIn:%d Nbcell:%d Pumpcharge:%d Window Phase:%d left_boundary:%d center:%d right_boundary:%d CurPulsePhase:%d Err:%d Bit:%d",current_pulsevalue,blankcell,pll->pump_charge/16,pll->phase/16,left_boundary,center,right_boundary,pll->lastpulsephase,cur_pll_error/16,t);

	return blankcell;
}



typedef struct s_match_
{
	int yes;
	int no;
	unsigned long offset;
}s_match;


void exchange(s_match *  table, int a, int b)
{
    s_match temp;
	temp = table[a];
    table[a] = table[b];
    table[b] = temp;
}

void quickSort(s_match * table, int start, int end)
{
    int left = start-1;
    int right = end+1;
    const int pivot = table[start].yes;

    if(start >= end)
        return;

    while(1)
    {
        do right--; while(table[right].yes > pivot);
        do left++; while(table[left].yes < pivot);

        if(left < right)
            exchange(table, left, right);
        else break;
    }

    quickSort(table, start, right);
    quickSort(table, right+1, end);
}

SIDE* ScanAndDecodeStream(int initalvalue,s_track_dump * track,unsigned long * overlap_tab,unsigned long start_index, short rpm)
{
#define TEMPBUFSIZE 256*1024
	int pumpcharge;
	int i,j,size;
	unsigned long value;
	int cellcode;
	int centralvalue;
	int bitrate;

	int bitoffset;
	int tracksize;

	int olderror;
	unsigned char *outtrack;
	unsigned char *flakeytrack;
	unsigned long *trackbitrate;

	pll_stat pll;
	
	SIDE* hxcfe_track;

	centralvalue = initalvalue;
	pumpcharge = initalvalue ;

	pll.pump_charge = ( initalvalue * 16 ) / 2;
	pll.phase = 0;
	pll.pivot = initalvalue * 16;
	pll.pll_max = pll.pivot + ( ( pll.pivot * 17 ) / 100 );
	pll.pll_min = pll.pivot - ( ( pll.pivot * 17 ) / 100 );

	pll.lastpulsephase = 0;

	olderror = 0;

	
	hxcfe_track = 0;

	if(start_index < track->nb_of_pulses)
	{
		// Work buffer allocation.

		outtrack=(unsigned char*)malloc(TEMPBUFSIZE);
		flakeytrack=(unsigned char*)malloc(TEMPBUFSIZE);
		trackbitrate=(unsigned long*)malloc(TEMPBUFSIZE*sizeof(unsigned long));

		memset(outtrack,0,TEMPBUFSIZE);
		memset(flakeytrack,0,TEMPBUFSIZE);
		memset(trackbitrate,0,TEMPBUFSIZE*sizeof(unsigned long));

		for(i=0;i<TEMPBUFSIZE;i++)
		{
			trackbitrate[i] = 250000; //(int)(24027428/centralvalue);
		}

		// Sync the "PLL"

		bitoffset=0;
		size = ( overlap_tab[start_index] - (start_index) );

		bitoffset=0;
		if(start_index>2000)
			i = start_index - 2000;
		else
			i = 0;

		while(i<(int)start_index)
		{
			value = track->track_dump[i];
			cellcode = getCellTiming(&pll,value,0,bitoffset,overlap_tab[i]);
			i++;
		};

		//
		// "Decode" the stream...
		//
		outtrack[0] = 0x80;
		bitoffset=0;
		i=0;
		do
		{
			value = track->track_dump[start_index + i];

			cellcode = getCellTiming(&pll,value,0,bitoffset,overlap_tab[start_index + i]);

			bitoffset = bitoffset + cellcode;

//			if((bitoffset>30680 && bitoffset<31500) || (bitoffset>20860 && bitoffset<21680))
//				floppycont->hxc_printf(MSG_DEBUG,">%d\t\tV:%d\t\tC:%d\tPLL:%d\tP:%d\tE:%d",bitoffset,value,cellcode,(pll.pump_charge/16),pll.pivot/16,pll.last_error/16);

			settrackbit(outtrack,TEMPBUFSIZE,0xFF,bitoffset,1);

			if(!overlap_tab[start_index + i])
				settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset,1);

			if( (bitoffset>>3) < TEMPBUFSIZE )
				trackbitrate[(bitoffset>>3)] = (int)((float)(24027428)/(float)((float)((pll.pump_charge*2)/16)));

			i++;
		}while(i<size);

		if(bitoffset&7)
		{
			tracksize = ( bitoffset >> 3 ) + 1;
		}
		else
		{
			tracksize = ( bitoffset >> 3 );
		}

		if( tracksize >= TEMPBUFSIZE ) tracksize = TEMPBUFSIZE - 1;

		// Bitrate Filter

		// First pass
		i=0;
		do
		{

			j=0;
			bitrate=0;
			while(((i+j) < tracksize ) && j<64)
			{
				bitrate = ( bitrate + trackbitrate[(i+j)] );
				j++;
			}

			bitrate=bitrate/j;

			j=0;
			while(((i+j)< tracksize ) && j<64)
			{
				trackbitrate[(i+j)] = bitrate;
				j++;
			}

			i = i + 16;

		}while(i < tracksize );


		// Second pass filter
		
		i=0;
		do
		{

			j=0;
			bitrate=0;
			while(((i+j) < tracksize ) && j<64)
			{
				bitrate = ( bitrate + trackbitrate[(i+j)] );
				j++;
			}

			bitrate=bitrate/j;

			j=0;
			while(((i+j)< tracksize ) && j<64)
			{
				trackbitrate[(i+j)] = bitrate;
				j++;
			}

			i = i + 64;

		}while(i < tracksize );

		bitrate=(int)( 24027428 / (centralvalue) );

		hxcfe_track = tg_alloctrack(bitrate,ISOFORMAT_DD,rpm,bitoffset,2000,0,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_ALLOCTIMIMGBUFFER);

		if( tracksize >= TEMPBUFSIZE ) tracksize = TEMPBUFSIZE - 1;

		memcpy(hxcfe_track->databuffer,outtrack,tracksize);
		memcpy(hxcfe_track->flakybitsbuffer,flakeytrack,tracksize);
		memcpy(hxcfe_track->timingbuffer,trackbitrate, tracksize * sizeof(unsigned long));

		hxcfe_track->bitrate = VARIABLEBITRATE;

		free(outtrack);
		free(flakeytrack);
		free(trackbitrate);

	}

	return hxcfe_track;
}

unsigned long tick_to_time(unsigned long tick)
{
	return (unsigned long) (double)( (double)tick  * (double)( (double)( 1000 * 1000 * 100 ) / (double)24027428 ) );
}

unsigned long time_to_tick(unsigned long time)
{
	return (unsigned long) (double)(( (double)time * 24027428 ) / (1000 * 1000 * 100 ) );
}


// us
#define BLOCK_TIME 25000

unsigned long GetDumpTimelength(s_track_dump * track_dump)
{
	unsigned long len;

	unsigned long nb_pulses,i;

	len = 0;

	if(track_dump)
	{
		nb_pulses = track_dump->nb_of_pulses;

		for(i=0;i<nb_pulses;i++)
		{
			len = len + track_dump->track_dump[i];
		}
	}

	return tick_to_time(len);
}

typedef struct pulsesblock_
{
	unsigned long timelenght;
	unsigned long ticklenght;
	unsigned long start_index;
	unsigned long end_index;
	unsigned long number_of_pulses;

	unsigned long state;
	unsigned long overlap_offset;
	unsigned long overlap_size;

	int locked;
}pulsesblock;

pulsesblock * ScanAndFindBoundaries(s_track_dump * track_dump, int blocktimelength, unsigned long number_of_block)
{
	unsigned long i,j;
	pulsesblock * pb;
	unsigned long len;
	unsigned long blocklen;

	pb = 0;

	pb = malloc(sizeof(pulsesblock) * number_of_block);
	if( pb )
	{
		memset(pb,0, sizeof(pulsesblock) * number_of_block);

		blocklen = time_to_tick( blocktimelength * 100 );

		j = 0;

		for(i=0;i<number_of_block;i++)
		{
			pb[i].start_index = j;

			len = 0;
			while(len < blocklen && j < track_dump->nb_of_pulses)
			{
				len = len + track_dump->track_dump[j];
				j++;
			};

			pb[i].end_index = j;
			pb[i].ticklenght = len;
			pb[i].timelenght = tick_to_time(len);
			pb[i].number_of_pulses = pb[i].end_index - pb[i].start_index;
		}

	}

	return pb;
}

unsigned long ScanAndGetIndexPeriod(s_track_dump * track_dump)
{
	unsigned long len, nb_rotation;

	unsigned long nb_pulses,nb_of_index,i,j,k;
	unsigned long indexper[32],globalperiod;

	len = 0;
	nb_of_index = 0;


	if(track_dump)
	{
		if(track_dump->nb_of_index > 1)
		{

			nb_rotation = 0;
			nb_pulses = track_dump->nb_of_pulses;
			nb_of_index = track_dump->nb_of_index;
			if(nb_of_index > 32) nb_of_index = 32;

			for(j=0;j<(track_dump->nb_of_index - 1);j++)
			{

				nb_pulses = track_dump->index_evt_tab[j + 1].cellpos - \
							track_dump->index_evt_tab[j].cellpos;

				len = 0;

				k = track_dump->index_evt_tab[j].cellpos;
				if(k + nb_pulses < track_dump->nb_of_pulses)
				{
					for(i=0;i<nb_pulses;i++)
					{
						len = len + track_dump->track_dump[i + k];
					}

					indexper[j] = len;
					nb_rotation++;
				}
			}

			globalperiod = 0;
			for(j=0;j<(nb_rotation);j++)
			{
				globalperiod = globalperiod + indexper[j];
			}

			globalperiod = (unsigned long)((double)((double)globalperiod / ((double)nb_rotation)));

		}
		else
		{
			globalperiod = 0;
		}

	}


	return tick_to_time(globalperiod);

}

void compareblock(s_track_dump * td,pulsesblock * src_block, unsigned long dst_block_offset,unsigned long * pulses_ok,unsigned long * pulses_failed, unsigned long * margetab,int partial)
{
	long marge;
	int time1,time2,pourcent_error;
	int bad_pulses,good_pulses;

	unsigned long * ptr1, * ptr2, *ptr3,*ptr4;

	bad_pulses = 0;
	good_pulses = 0;

	if(dst_block_offset + src_block->number_of_pulses >= td->nb_of_pulses)
	{
		if(pulses_ok)
			*pulses_ok = 0;
		
		if(pulses_failed)
			*pulses_failed = 0;

		return;
	}

	src_block->overlap_offset = dst_block_offset;
	
	ptr1 = &td->track_dump[src_block->start_index];
	ptr2 = &td->track_dump[dst_block_offset];
	ptr3 = &td->track_dump[td->nb_of_pulses - 2];

	if(partial)
		ptr4 = &td->track_dump[src_block->start_index + (src_block->number_of_pulses/100)];
	else
		ptr4 = &td->track_dump[src_block->start_index + (src_block->number_of_pulses)];

	time1 = *ptr1;
	time2 = *ptr2;

	pourcent_error = MAXPULSESKEW;

	do
	{
		if(time1<65536)
			marge = margetab[time1];
		else
			marge = ( ( time1 * pourcent_error ) / 100 );

		if( time2 > ( time1 + marge ) )
		{
			time1 = time1 + *(++ptr1);

			bad_pulses++;
		}
		else
		{
			if( time2 < ( time1 - marge ) )
			{
				time2 = time2 + *(++ptr2);

				bad_pulses++;
			}
			else
			{
				time1 = *(++ptr1);
				time2 = *(++ptr2);

				good_pulses++;
			}
		}

	}while(ptr2<ptr3 && ptr1<ptr4);

	src_block->overlap_size = ptr2 - &td->track_dump[dst_block_offset];

	if(pulses_ok)
		*pulses_ok = good_pulses;

	if(pulses_failed)
		*pulses_failed = bad_pulses;

}

unsigned long detectflakeybits(s_track_dump * td,pulsesblock * src_block, unsigned long dst_block_offset,unsigned long * pulses_ok,unsigned long * pulses_failed,unsigned long * overlap_tab,unsigned long maxptr,unsigned long * margetab)
{	
	long marge;
	int time1,time2,pourcent_error;
	int bad_pulses,good_pulses;
	unsigned long nbpulses;

	unsigned long *srcptr,*start_ptr, *dst_ptr, *last_dump_ptr,*last_block_ptr,*maxptr5;

	bad_pulses = 0;
	good_pulses = 0;

	if(dst_block_offset + src_block->number_of_pulses >= td->nb_of_pulses)
	{
		if(pulses_ok)
			*pulses_ok = 0;

		if(pulses_failed)
			*pulses_failed = 0;

		return 0;
	}
	
	srcptr        = td->track_dump;
	start_ptr     = &td->track_dump[src_block->start_index];
	dst_ptr       = &td->track_dump[dst_block_offset];
	last_dump_ptr = &td->track_dump[td->nb_of_pulses - 2];
	last_block_ptr= &td->track_dump[src_block->start_index + src_block->number_of_pulses];

	if(maxptr == 0xFFFFFFFF)
	{
		maxptr5 = &td->track_dump[td->nb_of_pulses-1];
	}
	else
	{
		maxptr5 = &td->track_dump[maxptr];
	}

	nbpulses = td->nb_of_pulses;

	time1 = *start_ptr;
	time2 = *dst_ptr;

	pourcent_error = MAXPULSESKEW;

	do
	{
		if(time1<65536)
			marge = margetab[time1];
		else
			marge = ( ( time1 * pourcent_error ) / 100 );

		if( time2 > ( time1 + marge ) )
		{
			time1 = time1 + *(++start_ptr);

			bad_pulses++;
		}
		else
		{
			if( time2 < ( time1 - marge ) )
			{
				time2 = time2 + *(++dst_ptr);

				bad_pulses++;
			}
			else
			{
				time1 = *(++start_ptr);
				time2 = *(++dst_ptr);

				if(dst_ptr < maxptr5 && start_ptr < last_dump_ptr)
				{
					overlap_tab[start_ptr - srcptr] = (dst_ptr - srcptr);
				}

				good_pulses++;
			}
		}

	}while(dst_ptr < last_dump_ptr && start_ptr < last_block_ptr);

	if(pulses_ok)
		*pulses_ok = good_pulses;

	if(pulses_failed)
		*pulses_failed = bad_pulses;

	return (dst_ptr - srcptr);
}

unsigned long getnearestbit(unsigned long * src,unsigned long index,unsigned long * dst,unsigned long buflen,long * shift)
{
	unsigned long i,ret_value;

	if(src[index])
	{
		for(i=0;i<12;i++)
		{
			if( (index + *shift ) + i  < buflen)
			{
				ret_value = dst[index + *shift + i];
				if( ret_value )
				{
					// The pulse is late 
					*shift = *shift + i/4;
					return ret_value;
				}
			}

			if( (((index + *shift) - i) < buflen) && (index - i)>=0 )
			{
				ret_value = dst[index + *shift - i];
				if( ret_value )
				{
					// The pulse is soon 
					*shift = *shift - i/4;
					return ret_value;
				}
			}
		}
	}

	return 0;
}

unsigned long compare_block_timebased(HXCFLOPPYEMULATOR* floppycontext,s_track_dump * td,pulsesblock * prev_block,pulsesblock * src_block,pulsesblock * next_block, unsigned long * overlap_tab,long *in_shift)
{
	unsigned long i;
	unsigned long src_start_index,src_last_index;
	unsigned long dst_start_index,dst_last_index;
	unsigned long total_tick_source,total_tick_source2;
	unsigned long total_tick_destination;
	unsigned long time_index;

	unsigned long * time_pulse_array_src;
	unsigned long * time_pulse_array_dst;

	unsigned long time_buffer_len,t;

	unsigned long bad_pulses,good_pulses;

	double timefactor;

	long shift;


	shift = 0;
	if(in_shift)
		shift = *in_shift;

	good_pulses = 0;
	bad_pulses = 0;

	// total time of the source block
	src_start_index = prev_block->start_index + prev_block->number_of_pulses;
	src_last_index  = next_block->start_index;

	//if(src_last_index >= time_buffer_len)
	//	src_last_index = time_buffer_len - 1;

	i = src_start_index;
	total_tick_source = 0;
	do
	{
		overlap_tab[i] = 0;
		total_tick_source = total_tick_source + td->track_dump[i];
		i++;
	}while(i< src_last_index);


	// total time of the other block
	dst_start_index = prev_block->overlap_offset + prev_block->overlap_size;
	dst_last_index  = next_block->overlap_offset;

	//if(dst_last_index >= time_buffer_len)
	//	dst_last_index = time_buffer_len - 1;

	i = dst_start_index;
	total_tick_destination = 0;
	do
	{
		total_tick_destination = total_tick_destination + td->track_dump[i];
		i++;
	}while(i< dst_last_index && i < time_buffer_len);

	

	time_buffer_len = ((tick_to_time(total_tick_source)/10) + 1024);

	time_pulse_array_src = malloc( time_buffer_len * sizeof(unsigned long));
	memset(time_pulse_array_src, 0 , time_buffer_len * sizeof(unsigned long));

	time_pulse_array_dst = malloc( time_buffer_len * sizeof(unsigned long));
	memset(time_pulse_array_dst, 0 ,time_buffer_len * sizeof(unsigned long));


	timefactor = (double)total_tick_source / (double)total_tick_destination ;

	i = src_start_index;
	total_tick_source2 = 0;
	
	while(i<= src_last_index && i < time_buffer_len)
	{
		total_tick_source2 = total_tick_source2 + td->track_dump[i];

		time_index = tick_to_time(total_tick_source2) / 10;

		if(time_index<time_buffer_len)
			time_pulse_array_src[time_index] = i;
		i++;
	};

	i = dst_start_index;
	total_tick_destination = 0;
	
	while(i<= dst_last_index && i < time_buffer_len)
	{
		total_tick_destination = total_tick_destination + td->track_dump[i];

		time_index = tick_to_time((unsigned long)((double)total_tick_destination*timefactor)) / 10;

		if(time_index<time_buffer_len)
			time_pulse_array_dst[time_index] = i;

		i++;
	};


	i = 0;
	do
	{
		if(time_pulse_array_src[i])
		{
			t = getnearestbit(time_pulse_array_src,i,time_pulse_array_dst,time_buffer_len,&shift);
			
			overlap_tab[time_pulse_array_src[i]] = t;

			if(t)
				good_pulses++;
			else
				bad_pulses++;
		}
		i++;
	}while(i< time_buffer_len);

	src_block->start_index = src_start_index;
	src_block->number_of_pulses = src_last_index - src_start_index;

	src_block->overlap_offset = dst_start_index;
	src_block->overlap_size = next_block->overlap_offset - dst_start_index;

	src_block->locked = 1;

	free(time_pulse_array_src);
	free(time_pulse_array_dst);

#ifdef KFSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"Source block tick len : %d (%d us) Destination block tick len :%d (%d us), good:%d,bad:%d",total_tick_source,tick_to_time(total_tick_source)/100,total_tick_destination,tick_to_time(total_tick_destination)/100,good_pulses,bad_pulses);
#endif

	return 0;
}



enum
{
	UNDEF_STATE = 0,
	ONEMATCH_STATE,
	MULTIMATCH_STATE,
	NOMATCH_STATE,
	MATCH_STATE,
	PARTIALMATCH_STATE
};

const char *  getStateStr(unsigned long state)
{
	switch(state)
	{
		case UNDEF_STATE:
			return (const char*)"UNDEF_STATE       ";
		break;
		case ONEMATCH_STATE:
			return (const char*)"ONEMATCH_STATE    ";
		break;
		case MULTIMATCH_STATE:
			return (const char*)"MULTIMATCH_STATE  ";
		break;
		case NOMATCH_STATE:
			return (const char*)"NOMATCH_STATE     ";
		break;
		case MATCH_STATE:
			return (const char*)"MATCH_STATE       ";
		break;
		case PARTIALMATCH_STATE:
			return (const char*)"PARTIALMATCH_STATE";
		break;

		default:
			return (const char*)"                  ";
		break;
	}
}


unsigned long * ScanAndFindRepeatedBlocks(HXCFLOPPYEMULATOR* floppycontext,s_track_dump * track_dump,unsigned long indexperiod,unsigned long nbblock,pulsesblock * pb)
{
	unsigned long j,block_analysed;
	unsigned long cur_time_len;
	unsigned long index_tickpediod,block_tickpediod;
	unsigned long repeat_index;
	int firstblock,previous_block_matched;

	unsigned long good,bad;

	unsigned long tick_up;
	unsigned long tick_down;

	unsigned long tick_up_max;
	unsigned long tick_down_max;

	unsigned long i_down,i_up;

	unsigned long pourcent_error;

	unsigned long mt_i, block_num,t;

	unsigned char c;

	long shift;

	int partialmatch_cnt;

	int end_dump_reached;

	s_match * match_table;
	unsigned long * overlap_pulses_tab;

	unsigned long * margetab,i;

	partialmatch_cnt = 0;

	index_tickpediod = time_to_tick(indexperiod);
	block_tickpediod = time_to_tick(BLOCK_TIME*100);

	firstblock = 1;
	previous_block_matched = 0;
	
	end_dump_reached = 0;
	overlap_pulses_tab = 0;

	margetab = malloc(sizeof(unsigned long) * 65536);
	memset(margetab,0,sizeof(unsigned long) * 65536);
	
	pourcent_error = MAXPULSESKEW;
	for(i=0;i<65536;i++)
	{
		margetab[i] = ( ( i * pourcent_error ) / 100 );
	}

	if(track_dump->nb_of_pulses)
	{
		match_table = malloc(sizeof(s_match) * track_dump->nb_of_pulses);
		if(match_table)
		{
			memset(match_table , 0,sizeof(s_match) * track_dump->nb_of_pulses);

			overlap_pulses_tab = malloc(track_dump->nb_of_pulses * sizeof(unsigned long) );
			memset(overlap_pulses_tab,0,track_dump->nb_of_pulses * sizeof(unsigned long) );

			// First pass : Fast/partial search of twin blocks.
/*			
			block_num=0;
			do
			{
				// find the theorical index point.
				if(block_num && (pb[block_num-1].state == PARTIALMATCH_STATE) )
				{

					j = pb[block_num-1].overlap_offset;
						
					cur_time_len = 0;
					do
					{

						cur_time_len = cur_time_len + track_dump->track_dump[j];

						j++;

					}while( (cur_time_len < block_tickpediod) && (j < track_dump->nb_of_pulses) );

					tick_down_max = time_to_tick((unsigned long)((double)BLOCK_TIME*100 * (double)0.05));
					tick_up_max =   time_to_tick((unsigned long)((double)BLOCK_TIME*100 * (double)0.05));
				}
				else
				{
					j = pb[block_num].start_index;
						
					cur_time_len = 0;
					do
					{

						cur_time_len = cur_time_len + track_dump->track_dump[j];

						j++;

					}while( (cur_time_len < index_tickpediod) && (j < track_dump->nb_of_pulses) );

					tick_down_max = time_to_tick((unsigned long)((double)indexperiod * (double)0.05));
					tick_up_max =   time_to_tick((unsigned long)((double)indexperiod * (double)0.05));
				}


				if(j < track_dump->nb_of_pulses )
				{
					repeat_index  = j - 1;

					tick_up = 0;
					tick_down = 0;

					i_up = 0;
					i_down = 0;


					memset(match_table , 0,sizeof(s_match) * track_dump->nb_of_pulses);

					mt_i = 0;

					do
					{
						if(tick_up < tick_up_max)
						{
							if(repeat_index + i_up < track_dump->nb_of_pulses)
							{
								// compare the block
								compareblock(track_dump,&pb[block_num], repeat_index + i_up,&good,&bad,margetab,1);

								match_table[mt_i].yes = good;
								match_table[mt_i].no = bad;
								match_table[mt_i].offset = repeat_index + i_up;
									
								tick_up = tick_up + track_dump->track_dump[repeat_index + i_up];
								i_up++;

								if(mt_i < track_dump->nb_of_pulses ) mt_i++;
							}
							else
							{
								tick_up = tick_up_max;
							}				
						}

						if(tick_down < tick_down_max)
						{
							if(repeat_index - i_down < track_dump->nb_of_pulses)
							{
								// compare the block
								compareblock(track_dump,&pb[block_num], repeat_index - i_down,&good,&bad,margetab,1);

								match_table[mt_i].yes = good;
								match_table[mt_i].no = bad;
								match_table[mt_i].offset = repeat_index - i_down;

								tick_down = tick_down + track_dump->track_dump[repeat_index - i_down];
								i_down++;

								if(mt_i< track_dump->nb_of_pulses ) mt_i++;
							}
							else
							{
								tick_down = tick_down_max;
							}
						}
					}while( (tick_up < tick_up_max) || (tick_down < tick_down_max) );

					quickSort(match_table, 0, mt_i-1);

					if(match_table[mt_i-1].no == 0 && match_table[mt_i-2].no != 0 && match_table[mt_i-1].yes)
					{
						#ifdef KFSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) partial match with the offset %d.",block_num,pb[block_num].number_of_pulses,match_table[mt_i-1].offset);
						#endif
							
						partialmatch_cnt ++;

						pb[block_num].state = PARTIALMATCH_STATE;
						pb[block_num].overlap_offset = match_table[mt_i-1].offset;
						pb[block_num].overlap_size = pb[block_num].number_of_pulses;
					}
		
					block_num++;
				}
			}while( (block_num<nbblock) && (j < track_dump->nb_of_pulses));

			floppycontext->hxc_printf(MSG_DEBUG,"Partial match blocks found :%d",partialmatch_cnt);
*/
  
			// Full analysis...
			block_num=0;
			do
			{
				if(previous_block_matched || pb[block_num].state == PARTIALMATCH_STATE)
				{
					// compare the block
					compareblock(track_dump,&pb[block_num], pb[block_num-1].overlap_offset + pb[block_num-1].number_of_pulses ,&good,&bad,margetab,0);

					if(good && !bad)
					{
						#ifdef KFSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d. [F]",block_num,pb[block_num].number_of_pulses,pb[block_num-1].overlap_offset + pb[block_num-1].number_of_pulses);
						#endif

						pb[block_num].state = MATCH_STATE;

						pb[block_num].locked = 1;

						block_num++;
						previous_block_matched = 1;
					}
					else
					{
						if(!good && !bad)
						{
							#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"End of the track dump");
							#endif

							end_dump_reached = 1;
						}
						else
						{
							#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index - offset %d) : Bad pulses found... full analysis",block_num,pb[block_num].number_of_pulses,pb[block_num-1].overlap_offset + pb[block_num-1].number_of_pulses);
							#endif
							if(pb[block_num].state == PARTIALMATCH_STATE)
									pb[block_num].state = 0;


							previous_block_matched = 0;
						}
					}
				}
				else
				{

					
					if(block_num && (pb[block_num-1].locked) )
					{

						j = pb[block_num-1].overlap_offset;
							
						cur_time_len = 0;
						do
						{

							cur_time_len = cur_time_len + track_dump->track_dump[j];

							j++;

						}while( (cur_time_len < block_tickpediod) && (j < track_dump->nb_of_pulses) );

						tick_down_max = time_to_tick((unsigned long)((double)BLOCK_TIME*100 * (double)0.05));
						tick_up_max =   time_to_tick((unsigned long)((double)BLOCK_TIME*100 * (double)0.05));
					}
					else
					{

						// find the theorical index point.
						j = pb[block_num].start_index;
						
						cur_time_len = 0;
						do
						{

							cur_time_len = cur_time_len + track_dump->track_dump[j];

							j++;

						}while( (cur_time_len < index_tickpediod) && (j < track_dump->nb_of_pulses) );

						tick_down_max = time_to_tick((unsigned long)((double)indexperiod * (double)0.05));
						tick_up_max =   time_to_tick((unsigned long)((double)indexperiod * (double)0.05));

					}


					if(j < track_dump->nb_of_pulses )
					{
						repeat_index  = j - 1;

						tick_up = 0;
						tick_down = 0;

						i_up = 0;
						i_down = 0;


						memset(match_table , 0,sizeof(s_match) * track_dump->nb_of_pulses);

						mt_i = 0;

						do
						{
							if(tick_up < tick_up_max)
							{
								if(repeat_index + i_up < track_dump->nb_of_pulses)
								{
									// compare the block
									compareblock(track_dump,&pb[block_num], repeat_index + i_up,&good,&bad,margetab,0);

									match_table[mt_i].yes = good;
									match_table[mt_i].no = bad;
									match_table[mt_i].offset = repeat_index + i_up;
									
									tick_up = tick_up + track_dump->track_dump[repeat_index + i_up];
									i_up++;

									if(mt_i < track_dump->nb_of_pulses ) mt_i++;
								}
								else
								{
									tick_up = tick_up_max;
								}
								
							}

							if(tick_down < tick_down_max)
							{
								if(repeat_index - i_down < track_dump->nb_of_pulses)
								{
									// compare the block
									compareblock(track_dump,&pb[block_num], repeat_index - i_down,&good,&bad,margetab,0);

									match_table[mt_i].yes = good;
									match_table[mt_i].no = bad;
									match_table[mt_i].offset = repeat_index - i_down;

									tick_down = tick_down + track_dump->track_dump[repeat_index - i_down];
									i_down++;

									if(mt_i< track_dump->nb_of_pulses ) mt_i++;
								}
								else
								{
									tick_down = tick_down_max;
								}
							}
						}while( (tick_up < tick_up_max) || (tick_down < tick_down_max) );

						quickSort(match_table, 0, mt_i-1);

						// Here we can get different case

						// -> Only one offset match at 100% -> We have found the overlap
						if(match_table[mt_i-1].no == 0 && match_table[mt_i-2].no != 0 && match_table[mt_i-1].yes)
						{
							#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d.",block_num,pb[block_num].number_of_pulses,match_table[mt_i-1].offset);
							#endif
							
							pb[block_num].state = ONEMATCH_STATE;
							pb[block_num].overlap_offset = match_table[mt_i-1].offset;
							pb[block_num].overlap_size = pb[block_num].number_of_pulses;

							pb[block_num].locked = 1;

							previous_block_matched = 1;
						}

						// -> different offset match at 100%
						if(match_table[mt_i-1].no == 0 && match_table[mt_i-2].no == 0 && match_table[mt_i-1].yes)
						{
							pb[block_num].state = MULTIMATCH_STATE;

							j = 0;
							t = 0;
							while(j < mt_i)
							{
								if((match_table[(mt_i-j)-1].no == 0) && match_table[(mt_i-j)-1].yes)
								{
									t++;
								}
								j++;
							}

							if(block_num)
							{
								// check if one position match with the previous block offset
								if( pb[block_num-1].locked )
								{
									j = mt_i-1;
									do
									{
										j--;	
									}while(j && (match_table[j].offset != (pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size)));

									if(j && match_table[j].yes && !match_table[j].no && (match_table[j].offset == (pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size)))
									{
										#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d. (M:%d)",block_num,pb[block_num].number_of_pulses,match_table[mt_i-1].offset,t);
										#endif

										pb[block_num].overlap_offset = match_table[j].offset;
										pb[block_num].overlap_size = pb[block_num].number_of_pulses;

										previous_block_matched = 1;

										// Lock ?
									}
									else
									{
										#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (not aligned with prev block)!",block_num,pb[block_num].number_of_pulses);
										#endif
									}
								}
								else
								{
									#ifdef KFSTREAMDBG
										floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (prev block unlocked) !",block_num,pb[block_num].number_of_pulses);
									#endif
								}
							}
							else
							{
								#ifdef KFSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (first block!)!",block_num,pb[block_num].number_of_pulses);
								#endif
							}
						}

						// -> No offset match at 100% -> take the best one
						if(match_table[mt_i-1].no && match_table[mt_i-1].yes)
						{

							pb[block_num].state = NOMATCH_STATE;
							pb[block_num].overlap_offset = match_table[mt_i-1].offset;
							

							// if the offset match with the previous block we can lock it.
							c = 1;

							if(block_num)
							{
								if( pb[block_num-1].locked && (pb[block_num].overlap_offset == ( pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size ) ) )
								{
									pb[block_num].locked = 1;

									t = detectflakeybits(track_dump,&pb[block_num],pb[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF,margetab);
									pb[block_num].overlap_size = t - pb[block_num].overlap_offset;


									#ifdef KFSTREAMDBG
										floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match not found. Best position : %d - %d ok %d bad. - Match with the previous block !",block_num,pb[block_num].number_of_pulses,match_table[mt_i-1].offset,match_table[mt_i-1].yes,match_table[mt_i-1].no);
										c=0;
									#endif

								}
							}
							
							if(c)
							{
								#ifdef KFSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match not found. Best position : %d - %d ok %d bad. - DOESN'T Match with the previous block !",block_num,pb[block_num].number_of_pulses,match_table[mt_i-1].offset,match_table[mt_i-1].yes,match_table[mt_i-1].no);
									c=0;
								#endif
							/*	if(block_num)
								{
									if( pb[block_num-1].locked ) 
										pb[block_num].overlap_offset = pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size;
								}

								t = detectflakeybits(track_dump,&pb[block_num],pb[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF,margetab);
								pb[block_num].overlap_size = t - pb[block_num].overlap_offset;*/
							}
						}
					}

					block_num++;
				}
				
			}while( (block_num<nbblock) && (j < track_dump->nb_of_pulses)  && !end_dump_reached);

			/////////////////////////////////////////////////////////
#ifdef KFSTREAMDBG
			floppycontext->hxc_printf(MSG_DEBUG,"-------- Fisrt Loop state ---------");

			// print missing blocks.
			for(block_num=0;block_num<nbblock;block_num++)
			{
				c = ' '; 

				if(block_num)
				{
					if( (pb[block_num-1].overlap_offset+pb[block_num-1].overlap_size) == pb[block_num].overlap_offset)
					{
						c = ' ';
					}
					else
					{
						c = 'B';
					}
				}

				floppycontext->hxc_printf(MSG_DEBUG,"Block %.4d : %s state |%d| - [%d <-> %d] %c",block_num,getStateStr(pb[block_num].state),pb[block_num].locked,pb[block_num].overlap_offset,pb[block_num].overlap_offset+pb[block_num].overlap_size,c);
			}
#endif
			/////////////////////////////////////////////////////////

			block_analysed=0;
			do
			{
				block_analysed = 0; 

				for(block_num = 0 ; block_num < nbblock ;block_num++)
				{

					if(!pb[block_num].locked)
					{
						switch(pb[block_num].state)
						{
							//
							case MATCH_STATE:
							case ONEMATCH_STATE:
								if(block_num)
								{
									if(pb[block_num-1].locked)
									{
										if( ( pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size ) == pb[block_num].overlap_offset )
										{
											pb[block_num].locked = 1;
											block_analysed++;
										}	
									}
								}

								if(block_num < nbblock-1)
								{
									if(pb[block_num+1].locked)
									{
										if( ( pb[block_num].overlap_offset + pb[block_num].overlap_size ) == pb[block_num + 1].overlap_offset )
										{
											pb[block_num].locked = 1;
											block_analysed++;
										}
									}
								}

							break;

							case MULTIMATCH_STATE:
								c = 1;
								if(block_num)
								{
									
									if(pb[block_num-1].locked)
									{
										compareblock(track_dump,&pb[block_num], pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size ,&good,&bad,margetab,0);
										if(!bad && good)
										{
											pb[block_num].overlap_offset = pb[block_num-1].overlap_offset + pb[block_num-1].overlap_size;
											pb[block_num].locked = 1;
											block_analysed++;
											c = 0;
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											pb[block_num].locked = 0;
										}
									}
								}

								if(block_num < nbblock-1 && c)
								{
									if(pb[block_num+1].locked)
									{
										compareblock(track_dump,&pb[block_num], pb[block_num+1].overlap_offset - pb[block_num].overlap_size ,&good,&bad,margetab,0);
										if(!bad && good)
										{
											pb[block_num].overlap_offset = pb[block_num+1].overlap_offset - pb[block_num].overlap_size;
											pb[block_num].locked = 1;
											block_analysed++;
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											pb[block_num].locked = 0;
										}
									}
								}
							break;

							case NOMATCH_STATE:

								if(block_num)
								{
									if(pb[block_num-1].locked)
									{
										t = detectflakeybits(track_dump,&pb[block_num],pb[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF,margetab);

										if(pb[block_num-1].overlap_offset + pb[block_num-1].number_of_pulses == pb[block_num].overlap_offset)
										{
											pb[block_num].locked = 1;
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : pre aligned with the previous block",block_num,pb[block_num].number_of_pulses);
#endif
										}
										else
										{
											pb[block_num].locked = 0;
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : not pre aligned with the previous block! : %d != %d",block_num,pb[block_num].number_of_pulses,pb[block_num-1].overlap_offset + pb[block_num-1].number_of_pulses,pb[block_num].overlap_offset);
#endif
										}
									}
								}

								if(block_num < nbblock-1)
								{
									if(pb[block_num+1].locked) 
									{
										t = detectflakeybits(track_dump,&pb[block_num],pb[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF,margetab);

										if(pb[block_num+1].overlap_offset == t)
										{
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : post aligned with the next block - %d",block_num,pb[block_num].number_of_pulses,t);
#endif
											
											pb[block_num].locked = 1;
										}
										else
										{
#ifdef KFSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : not post aligned with the next block ! %d != %d   - %d",block_num,pb[block_num].number_of_pulses,(pb[block_num].overlap_offset + pb[block_num].number_of_pulses),pb[block_num+1].overlap_offset,t);
#endif
											pb[block_num].locked = 0;
										}
									}
								}
							break;

							default:
							break;
						}
					}
				}

			}while(block_analysed);


			memset(overlap_pulses_tab,0,track_dump->nb_of_pulses * sizeof(unsigned long) );

			for(block_num=0;block_num<nbblock;block_num++)
			{
				if(pb[block_num].locked)
				{
					detectflakeybits(track_dump,&pb[block_num],pb[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF,margetab);
				}
			}

			// Scan unlocked blocks...
			shift = 0;
			for(block_num=0;block_num<nbblock;block_num++)
			{
				if(!pb[block_num].locked)
				{
					if(block_num<nbblock-1)
					{
						if(block_num)
						{
							if(pb[block_num-1].locked)
							{
								
								i=1;
								while(block_num+i<nbblock-1 && !pb[block_num+i].locked)
								{
									i++;
								}
					
								if(block_num+i<nbblock-1)
								{					
#ifdef KFSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"scan unlocked flakey bits block %d-%d...",block_num,block_num+i);
#endif
									compare_block_timebased(floppycontext,track_dump,&pb[block_num-1],&pb[block_num],&pb[block_num+i], overlap_pulses_tab,&shift);
								}
							}
						}
					}
				}
			}

			shift = 0;
			for(block_num=0;block_num<nbblock;block_num++)
			{
				if(pb[block_num].locked)
				{
					if(block_num<nbblock-1)
					{
						if(block_num)
						{
							if(pb[block_num-1].locked && pb[block_num+1].locked && pb[block_num].state == NOMATCH_STATE)
							{
								i=1;
								while(block_num+i<nbblock-1 && !pb[block_num+i].locked)
								{
									i++;
								}

								if(block_num+i<nbblock-1)
								{
#ifdef KFSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"Rescan flakey bits block %d...",block_num);
#endif
									compare_block_timebased(floppycontext,track_dump,&pb[block_num-1],&pb[block_num],&pb[block_num+i], overlap_pulses_tab,&shift);
								}
							}
						}
					}
				}
			}

			// print missing blocks.
			for(block_num=0;block_num<nbblock;block_num++)
			{
				c = ' '; 
				if(block_num)
				{
					if( (pb[block_num-1].overlap_offset+pb[block_num-1].overlap_size) == pb[block_num].overlap_offset)
					{
						c = ' ';
					}
					else
					{
						c = 'B';
					}
				}

				j = 0;
				for(i=pb[block_num].start_index;i<pb[block_num].start_index+pb[block_num].number_of_pulses;i++)
				{
					if(!overlap_pulses_tab[i]) j++;
				}	
#ifdef KFSTREAMDBG
				floppycontext->hxc_printf(MSG_DEBUG,"Block %.4d : %s state |%d| - [%d <-> %d] %c s:%d l:%d size:%d bad bit:%d",block_num,getStateStr(pb[block_num].state),pb[block_num].locked,pb[block_num].overlap_offset,pb[block_num].overlap_offset+pb[block_num].overlap_size,c,pb[block_num].start_index,pb[block_num].number_of_pulses,pb[block_num].start_index+pb[block_num].number_of_pulses,j);
#endif 
			}

			/*for(i=0;i<track_dump->nb_of_pulses;i++)
			{
				//if(track_dump->track_dump[i] > 0x100)
				{
					floppycontext->hxc_printf(MSG_DEBUG,"offset:%d , val :%d",i,track_dump->track_dump[i]);
				}
			}*/

			free(match_table);
		}
	}

	free(margetab);

	return overlap_pulses_tab;
}


unsigned long getbestindex(s_track_dump *track_dump,unsigned long * overlap_tab)
{
	unsigned long nb_index_compare,i,j;
	unsigned long first_index,last_index,bad_pulses;

	unsigned long bad_pulses_array[32],min_val;
	unsigned long bestval;

	bestval = 0;

	if(track_dump->nb_of_index >= 4)
	{
		nb_index_compare = track_dump->nb_of_index - 3;
		if(nb_index_compare > 32) nb_index_compare = 32;

		memset(bad_pulses_array,0,sizeof(unsigned long) * 32);

		for(i=0;i<nb_index_compare;i++)
		{

			first_index = track_dump->index_evt_tab[i].dump_offset;

			if( first_index < track_dump->nb_of_pulses)
			{
				while( (first_index < track_dump->nb_of_pulses) && !overlap_tab[first_index])
				{
					first_index++;
				}
			}

			if( first_index >= track_dump->nb_of_pulses)
			{
				first_index = track_dump->nb_of_pulses - 1;
			}


			bad_pulses = 0;
			last_index = overlap_tab[first_index];

			for(j=first_index;j<last_index;j++)
			{
				if(!overlap_tab[j])
					bad_pulses++;
			}
			
			bad_pulses_array[i] = bad_pulses;
		}

		min_val = bad_pulses_array[0];
		for(i=0;i<nb_index_compare;i++)
		{
			if(min_val >= bad_pulses_array[i])
			{
				min_val = bad_pulses_array[i];
				bestval = i;
			}
		}
	}

	return bestval;
}


SIDE* decodestream(HXCFLOPPYEMULATOR* floppycontext,char * file,short * rpm,float timecoef)
{
	double mck;
	double sck;
	double ick;
	int bitrate;
	unsigned long totallen, nbblock,indexperiod;
	unsigned long * histo;
	SIDE* currentside;
	
	unsigned long * overlap_tab;
	unsigned long first_index,index;
	unsigned long track_len;
	unsigned long i;

	s_track_dump *track_dump;
	pulsesblock * pb;

	int index_pos;

	index_pos=BASEINDEX;
	
	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading %s...",hxc_getfilenamebase(file,0));

	track_dump=DecodeKFStreamFile(floppycontext,file,timecoef);
	if(track_dump)
	{
	
		mck=((18432000 * 73) / 14) / 2;
		sck=mck/2;
		ick=mck/16;

		if(track_dump->nb_of_index>2)
		{
			// Get the total track dump time length. (in 10th of nano seconds)
			totallen = GetDumpTimelength(track_dump);

			// Number of block of 10ms.
			nbblock = totallen / ( BLOCK_TIME * 100 );

			floppycontext->hxc_printf(MSG_DEBUG,"Track dump lenght : %d us, number of block : %d (block time : %d us)",totallen/100,nbblock,BLOCK_TIME);

			// Scan and find block index boundaries.
			pb = ScanAndFindBoundaries(track_dump, BLOCK_TIME, nbblock);

			// Get the index period.
			indexperiod = ScanAndGetIndexPeriod(track_dump);

			floppycontext->hxc_printf(MSG_DEBUG,"Index period : %d ms",indexperiod / (1000*100) );

			floppycontext->hxc_printf(MSG_DEBUG,"Block analysing...");
			
			// Find the blocks overlap.
			overlap_tab = ScanAndFindRepeatedBlocks(floppycontext,track_dump,indexperiod,nbblock,pb);
			
			floppycontext->hxc_printf(MSG_DEBUG,"...done");

			if(overlap_tab)
			{

				index = getbestindex(track_dump,overlap_tab);

				first_index = track_dump->index_evt_tab[index].dump_offset;
		
				if( first_index < track_dump->nb_of_pulses)
				{
					while((first_index < track_dump->nb_of_pulses) && !overlap_tab[first_index])
					{
						first_index++;
					}
				}

				if( first_index >= track_dump->nb_of_pulses)
				{
					first_index = track_dump->nb_of_pulses - 1;
				}

				track_len = 0;
				i = first_index;
				if( i < track_dump->nb_of_pulses)
				{
					do
					{
						track_len = track_len + track_dump->track_dump[i];
						i++;
					}while( ( i < track_dump->nb_of_pulses) && (i<overlap_tab[first_index]) && (i<track_dump->nb_of_pulses));
				}

				if(track_len)
				{
					*rpm = (short)((double)(1 * 60 * 1000) / (double)( (double)tick_to_time(track_len) / (double)100000));

					if(*rpm <= 0 || *rpm > 800 || tick_to_time(track_len) < 5000000 )
					{
						*rpm = 300;

						first_index = track_dump->index_evt_tab[0].dump_offset;

						i = 0;
						track_len = 0;
						do
						{
							track_len = track_len + track_dump->track_dump[first_index + i];
							i++;
						}while(i < track_dump->nb_of_pulses && track_len<time_to_tick(indexperiod));
						overlap_tab[first_index] = first_index + i;
					}

					histo=(unsigned long*)malloc(65536* sizeof(unsigned long));

					computehistogram(&track_dump->track_dump[first_index],overlap_tab[first_index] - first_index,histo);

					bitrate=detectpeaks(floppycontext,histo);

					floppycontext->hxc_printf(MSG_DEBUG,"%d RPM, Bitrate: %d",*rpm,(int)(24027428/bitrate) );

					floppycontext->hxc_printf(MSG_DEBUG,"Cells analysing...");
					currentside=ScanAndDecodeStream(bitrate,track_dump,overlap_tab,first_index,*rpm);
					floppycontext->hxc_printf(MSG_DEBUG,"...done");

					free(histo);
				}

				free(overlap_tab);

			}

			free(pb);

			/*j=currentside->tracklen/8;
			if(currentside->tracklen&7) j++;
			for(i=0;i<j;i++)
			{
				floppycontext->hxc_printf(MSG_DEBUG,"D:%.2X\tM:%.2X\tBR:%.8d\tI:%.2X",
						currentside->databuffer[i],
						currentside->flakybitsbuffer[i],
						currentside->timingbuffer[i],
						currentside->indexbuffer[i]);
			}*/
			
		}

		FreeStream(track_dump);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

double GetTrackPeriod(HXCFLOPPYEMULATOR* floppycontext,SIDE * curside)
{
	int tracklen,i;
	double total_period;

	tracklen = curside->tracklen /8;
	if(curside->tracklen & 7)
		tracklen++;

	total_period = 0;
	for(i=0;i<tracklen;i++)
	{
		total_period = total_period + (double)((double)1/(double)curside->timingbuffer[i]);
	}

	return total_period;
}

void AdjustTrackPeriod(HXCFLOPPYEMULATOR* floppycontext,SIDE * curside_S0,SIDE * curside_S1)
{
	int tracklen,i;
	double period_s0,period_s1;

	tracklen = curside_S1->tracklen /8;
	if(curside_S1->tracklen & 7)
		tracklen++;

	period_s0 = GetTrackPeriod(floppycontext,curside_S0);
	period_s1 = GetTrackPeriod(floppycontext,curside_S1);

	for(i=0;i<tracklen;i++)
	{
		curside_S1->timingbuffer[i] = (unsigned long)((double)curside_S1->timingbuffer[i] * (period_s1/period_s0));
	}
}


int KryoFluxStream_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{	
	FILE * f;
	char * filepath;
	char * folder;
	char fname[512];
	int mintrack,maxtrack;
	int minside,maxside,singleside;
	short rpm;
	unsigned short i,j;
	int k,l;
	int doublestep;
	CYLINDER* currentcylinder;
	int len;
	int found,track,side;
	struct stat staterep;
	s_oob_header oob;
	SIDE * curside;
	int nbtrack,nbside;
	float timecoef;
	int tracklen;

	int previous_bit;
		
	floppycont = floppycontext;

	
	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libLoad_DiskFile");
	
	if(imgfile)
	{
		if(!hxc_stat(imgfile,&staterep))
		{
			
			len=hxc_getpathfolder(imgfile,0);
			folder=(char*)malloc(len+1);
			hxc_getpathfolder(imgfile,folder);

			if(staterep.st_mode&S_IFDIR)
			{
				sprintf(fname,"track");
			}
			else
			{
				hxc_getfilenamebase(imgfile,(char*)&fname);
				if(!strstr(fname,".0.raw") && !strstr(fname,".1.raw") )
				{
					free(folder);
					return HXCFE_BADFILE;
				}

				fname[strlen(fname)-8]=0;

			}

			filepath = malloc( strlen(imgfile) + 32 );

			doublestep=1;///
			sprintf(filepath,"%s%s",folder,"doublestep");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				doublestep=2;
				hxc_fclose(f);
			}

			singleside=0;
			sprintf(filepath,"%s%s",folder,"singleside");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				singleside=1;
				hxc_fclose(f);
			}

			timecoef=1;//2;////
			sprintf(filepath,"%s%s",folder,"rpm360rpm300");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)1.2;
				hxc_fclose(f);
			}

			sprintf(filepath,"%s%s",folder,"rpm300rpm360");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)0.833;
				hxc_fclose(f);
			}

			track=0;
			side=0;
			found=0;

			mintrack=0;
			maxtrack=0;
			minside=0;
			maxside=0;

			do
			{
				sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,track,side);
				f=hxc_fopen(filepath,"rb");
				if(f)
				{
					fread(&oob,sizeof(s_oob_header),1,f);
					if(oob.Sign==OOB_SIGN)
					{
						if(mintrack>track) mintrack = track;
						if(maxtrack<track) maxtrack = track;
						if(minside>side) minside = side;
						if(maxside<side) maxside = side;
						found=1;
					}
					hxc_fclose(f);
				}
				side++;
				if(side>1) 
				{
					side = 0;
					track=track+doublestep;
				}
			}while(track<84);

			if(!found)
			{
				free( folder );
				free( filepath );
				return HXCFE_BADFILE;
			}

			nbside=(maxside-minside)+1;
			if(singleside)
				nbside = 1;
			nbtrack=(maxtrack-mintrack)+1;
			nbtrack=nbtrack/doublestep;

			floppycontext->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate=VARIABLEBITRATE;
			floppydisk->floppyNumberOfTrack=nbtrack;
			floppydisk->floppyNumberOfSide=nbside;
			floppydisk->floppySectorPerTrack=-1;

			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*doublestep;j=j+doublestep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{					
					sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,j,i);
					
					curside=decodestream(floppycontext,filepath,&rpm,timecoef);

					if(!floppydisk->tracks[j/doublestep])
					{
						floppydisk->tracks[j/doublestep]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					}

					if(curside)
					{
						tracklen = curside->tracklen /8;
						if(curside->tracklen & 7)
							tracklen++;

						// Remove sticked data bits ...
						previous_bit = 0;
						for(k=0;k<tracklen;k++)
						{
							if(previous_bit)
							{
								if(curside->databuffer[k] & 0x80)
								{
									//curside->databuffer[k-1] = curside->databuffer[k-1] ^ 0x01;
									//curside->flakybitsbuffer[k-1] =  curside->flakybitsbuffer[k-1] | (0x01);
									curside->databuffer[k] = curside->databuffer[k] ^ 0x80;
									curside->flakybitsbuffer[k] =  curside->flakybitsbuffer[k] | (0x80);
								}
							}

							for(l=0;l<7;l++)
							{
								if((curside->databuffer[k] & (0xC0>>l)) == (0xC0>>l))
								{
									curside->databuffer[k] = curside->databuffer[k] ^ (0x40>>l);
									curside->flakybitsbuffer[k] =  curside->flakybitsbuffer[k] | (0x40>>l);
								}
							}

							previous_bit = curside->databuffer[k] & 1;
						}

					}

					currentcylinder=floppydisk->tracks[j/doublestep];
					currentcylinder->sides[i]=curside;

				}
			}


			// Adjust track timings. 
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{					
					curside = floppydisk->tracks[j]->sides[i];
					if(curside && floppydisk->tracks[0]->sides[0])
					{
						AdjustTrackPeriod(floppycontext,floppydisk->tracks[0]->sides[0],curside);
					}
				}
			}

			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			free( folder );
			free( filepath );

			hxcfe_sanityCheck(floppycontext,floppydisk);

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}

int KryoFluxStream_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="KRYOFLUXSTREAM";
	static const char plug_desc[]="KryoFlux Stream Loader";
	static const char plug_ext[]="raw";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	KryoFluxStream_libIsValidDiskFile,
		(LOADDISKFILE)		KryoFluxStream_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	KryoFluxStream_libGetPluginInfo
	};

	return libGetPluginInfo(
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
