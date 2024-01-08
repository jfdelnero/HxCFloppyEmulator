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
// File : fluxStreamAnalyzer.c
// Contains: Flux (pulses) Stream analyzer
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"
#include "tracks/trackutils.h"

#include "fluxStreamAnalyzer.h"

#include "libhxcadaptor.h"

#include "misc/env.h"

//#include "asm_mmx_routines.h"

#define FLUXSTREAMDBG 1

#define DEFAULT_MAXPULSESKEW 25   // in per-256
#define DEFAULT_BLOCK_TIME 1000   // in uS
#define DEFAULT_SEARCHDEPTH 0.025

#define SECONDPASSANALYSIS 1

//#define EXPAND_MATCHING_BLOCK 1

HXCFE* floppycont;

int victor_9k_bands_def[]=
{
	0,  1,2142, 3,3600, 5,5200, 0,
	4,  1,2492, 3,3800, 5,5312, 0,
	16, 1,2550, 3,3966, 5,5552, 0,
	27, 1,2723, 3,4225, 5,5852, 0,
	38, 1,2950, 3,4500, 5,6450, 0,
	48, 1,3150, 3,4836, 5,6800, 0,
	60, 1,3400, 3,5250, 5,7500, 0,
	71, 1,3800, 3,5600, 5,8000, 0,

	-1, 0,   0, 0,   0, 0,   0, 0
};

static void settrackbit(uint8_t * dstbuffer,int dstsize,uint8_t byte,int bitoffset,int size)
{
	int i,j;

	i = bitoffset;
	for(j = 0; j < size;j++)
	{
		if( i>=0 && ( (i>>3) < dstsize) )
		{
			if(byte&((0x80)>>(j&7)))
				dstbuffer[(i>>3)] = (uint8_t)(dstbuffer[(i>>3)]|( (0x80>>(i&7))));
			else
				dstbuffer[(i>>3)] = (uint8_t)(dstbuffer[(i>>3)]&(~(0x80>>(i&7))));
		}

		i++;
	}
}

static int gettrackbit(uint8_t * dstbuffer,int dstsize,int bitoffset)
{
	int i;

	i = bitoffset;

	if((uint8_t)(dstbuffer[(i>>3)] & ( (0x80>>(i&7)))))
		return 1;
	else
		return 0;
}

void computehistogram(uint32_t *indata,int size,uint32_t *outdata)
{
	int i;

	if( !indata || !outdata )
		return;

	memset(outdata,0,sizeof(uint32_t) * (65536) );
	for(i=0;i<size;i++)
	{
		if(indata[i]<0x10000)
		{
			outdata[indata[i]]++;
		}
	}
}

int detectpeaks(HXCFE* floppycontext, pll_stat *pll, uint32_t *histogram)
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
	if(!stattab)
		return 0;

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

#ifdef FLUXSTREAMDBG
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

		if(ret<(pll->tick_freq/224618) && ret>=(pll->tick_freq/276243))
		{
			total250k=total250k+stattab[i].occurence;
			pourcent250k = pourcent250k + stattab[i].pourcent;
		}

		if(ret<(pll->tick_freq/276243) && ret>=(pll->tick_freq/353606))
		{
			total300k=total300k+stattab[i].occurence;
			pourcent300k = pourcent300k + stattab[i].pourcent;
		}

		if(ret<(pll->tick_freq/437062) && ret>(pll->tick_freq/572082))
		{
			total500k=total500k+stattab[i].occurence;
			pourcent500k = pourcent500k + stattab[i].pourcent;
		}

		i++;
	}

	free(stattab);

	if(pourcent500k > 2)
	{
		return pll->tick_freq / 500000;
	}

	if((pourcent300k > 2) && (pourcent300k>pourcent250k))
	{
		return pll->tick_freq / 300000;
	}

	if((pourcent250k > 2) && (pourcent250k>pourcent300k))
	{
		return pll->tick_freq / 250000;
	}

	if((pourcent300k>pourcent250k))
	{
		return pll->tick_freq / 300000;
	}
	else
	{
		return pll->tick_freq / 250000;
	}
}

static int getBandCells(pll_stat * pll,int current_pulsevalue)
{
	int i,ret;

	ret = 1;

	i = 0;
	while( ( i < 16 ) && ( (pll->bands_separators[i] == -1) || (pll->bands_separators[i] < current_pulsevalue) ) )
	{
		//printf(">>%d\n",pll->bands_separators[i]);
		i++;
	}

	if(i<16)
		ret = i + 1 ;

	//printf(">%d  -  %d\n",current_pulsevalue,ret);
	return ret;
}

static int getCellTiming(pll_stat * pll,int current_pulsevalue,int * badpulse,int overlapval,int phasecorrection)
{
	int blankcell;
	int cur_pll_error,left_boundary,right_boundary,center;
	int current_pulse_position;

	blankcell = 0;

	if(pll->band_mode)
	{
		return getBandCells(pll, current_pulsevalue);
	}

	current_pulsevalue = current_pulsevalue * 16;

	// very long tracks analysis overflow fix.
	if(pll->phase > (512*1024*1014))
	{
		pll->phase -= (256*1024*1014);
		pll->lastpulsephase -= (256*1024*1014);
	}

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
		pll->lastpulsephase = pll->lastpulsephase + current_pulsevalue;

		if(badpulse)
			*badpulse = 1;

		//floppycont->hxc_printf(MSG_DEBUG,"ValIn:%d Nbcell:%d Pumpcharche:%d Window Phase:%d LastPulsePhase:%d Err:%d",current_pulsevalue,blankcell,pll->pump_charge,pll->phase,pll->lastpulsephase,current_pulse_position - center);
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

	if( pll->inter_band_rejection )
	{
		switch(pll->inter_band_rejection)
		{
			case 1:
				// GCR
				if(blankcell == 3)
				{
					if((right_boundary - current_pulse_position) > pll->pump_charge / 2)
						blankcell = 2;
					else
						blankcell = 4;
				}

				if(blankcell == 5)
				{
					if((right_boundary - current_pulse_position) > pll->pump_charge / 2)
						blankcell = 4;
					else
						blankcell = 6;
				}
			break;

			case 2:
				// FM
				if(blankcell == 1)
				{
					blankcell = 2;
				}

				if(blankcell == 3)
				{
					if((right_boundary - current_pulse_position) > pll->pump_charge / 2)
						blankcell = 2;
					else
						blankcell = 4;
				}

				if(blankcell > 4)
				{
					blankcell = 4;
				}
			break;

		}
	}

	// Is the pulse before or after the middle of the window ?
	// negative value : clock too slow - positive value : clock too fast
	cur_pll_error = current_pulse_position - center;

	if(overlapval)
	{
		if(pll->pump_charge < ( pll->pivot / 2 ) )
		{
			if(cur_pll_error<0)
				pll->pump_charge = ( (pll->pump_charge * pll->slow_correction_ratio_n) + ( pll->pump_charge + cur_pll_error ) ) / pll->slow_correction_ratio_d;
			else
				pll->pump_charge = ( (pll->pump_charge * pll->fast_correction_ratio_n) + ( pll->pump_charge + cur_pll_error ) ) / pll->fast_correction_ratio_d;
		}
		else
		{
			if(cur_pll_error<0)
				pll->pump_charge = ( (pll->pump_charge * pll->fast_correction_ratio_n) + ( pll->pump_charge + cur_pll_error ) ) / pll->fast_correction_ratio_d;
			else
				pll->pump_charge = ( (pll->pump_charge * pll->slow_correction_ratio_n) + ( pll->pump_charge + cur_pll_error ) ) / pll->slow_correction_ratio_d;
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
		pll->phase = pll->phase + ( cur_pll_error / phasecorrection );
	}

	// next window
	pll->phase = pll->phase + pll->pump_charge;
	pll->lastpulsephase = pll->lastpulsephase + current_pulsevalue;

	pll->last_error = cur_pll_error;

	//floppycont->hxc_printf(MSG_DEBUG,"ValIn:%d Nbcell:%d Pumpcharge:%d Window Phase:%d left_boundary:%d center:%d right_boundary:%d CurPulsePhase:%d Err:%d",current_pulsevalue,blankcell,pll->pump_charge/16,pll->phase/16,left_boundary,center,right_boundary,pll->lastpulsephase,cur_pll_error/16);

	return blankcell;
}

static void exchange(s_match *  table, int a, int b)
{
	s_match temp;
	temp = table[a];
	table[a] = table[b];
	table[b] = temp;
}

static void quickSort(s_match * table, int start, int end)
{
	int left = start-1;
	int right = end+1;
	const int pivot = table[start].yes;

	if(start >= end)
		return;

	for(;;)
	{
		do right--; while(table[right].yes > pivot);
		do left++; while(table[left].yes < pivot);

		if(left < right)
			exchange(table, left, right);
		else
			break;
	}

	quickSort(table, start, right);
	quickSort(table, right+1, end);
}

//#define USE_PLL_BITRATE 1

HXCFE_SIDE* ScanAndDecodeStream(HXCFE* floppycontext,HXCFE_FXSA * fxs, int initialvalue,HXCFE_TRKSTREAM * track,pulses_link * pl,uint32_t start_index, short rpm,int phasecorrection,int flags)
{
	#define TEMPBUFSIZE 512*1024

	int i,j,k,size;
	uint32_t value;
	int cellcode;
	int centralvalue;
	int bitrate;

	int bitoffset;
	int old_bitoffset;
	int tracksize;
	int lastpulsebitoffset;

	unsigned char *outtrack;
	unsigned char *flakeytrack;
	unsigned char *indextrack;
	uint32_t *trackbitrate;
	uint32_t *tickposition;
#ifndef USE_PLL_BITRATE
	float byteduration;
	float partialduration;
	float cumul_value;
	uint32_t cumul_code;

	uint32_t code_array[32];
	float    value_array[32];
	uint32_t in_ptr;
	uint32_t out_ptr;
	uint32_t fifolevel;
	uint32_t leftbit;
#endif
	uint32_t nextindex_pos;
	uint32_t nextindex;
	uint32_t tickoffset;
	int lastbitisflakey;

	HXCFE_SIDE* hxcfe_track;

	centralvalue = hxcfe_getEnvVarValue( floppycontext, "FLUXSTREAM_PLL_INITIAL_BITRATE");
	if(!centralvalue)
	{
		centralvalue = initialvalue;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"ScanAndDecodeStream : Measured bitrate : %d, Forced value : %d",(int)(fxs->pll.tick_freq/initialvalue), centralvalue);
		centralvalue = (int)(fxs->pll.tick_freq/centralvalue);
	}

	fxs->pll.pump_charge = ( centralvalue * 16 ) / 2;
	fxs->pll.phase = 0;
	fxs->pll.pivot = centralvalue * 16;
	fxs->pll.pll_max = fxs->pll.pivot + ( ( fxs->pll.pivot * fxs->pll.pll_min_max_percent ) / 100 );
	fxs->pll.pll_min = fxs->pll.pivot - ( ( fxs->pll.pivot * fxs->pll.pll_min_max_percent ) / 100 );

	fxs->pll.lastpulsephase = 0;

	hxcfe_track = 0;

	tickposition = NULL;

	if(start_index < track->channels[0].nb_of_pulses)
	{
		// Work buffer allocation.

		outtrack = (unsigned char*)calloc(1,TEMPBUFSIZE);
		flakeytrack = (unsigned char*)calloc(1,TEMPBUFSIZE);
		indextrack = (unsigned char*)calloc(1,TEMPBUFSIZE);
		trackbitrate = (uint32_t*)calloc(1,TEMPBUFSIZE*sizeof(uint32_t));

		if( !outtrack || !flakeytrack || !indextrack || !trackbitrate )
		{
			free(outtrack);
			free(flakeytrack);
			free(indextrack);
			free(trackbitrate);

			return NULL;
		}

		for(i=0;i<TEMPBUFSIZE;i++)
		{
			trackbitrate[i] = (int)(fxs->pll.tick_freq/centralvalue);
		}

		// Sync the "PLL"

		bitoffset=0;
		if(!pl)
		{
			size = track->channels[0].nb_of_pulses - start_index;
		}
		else
		{
			if( ( start_index + (pl->forward_link[start_index] - start_index ) ) < track->channels[0].nb_of_pulses )
			{
				size = ( pl->forward_link[start_index] - (start_index) );
			}
			else
			{
				floppycontext->hxc_printf(MSG_ERROR,"ScanAndDecodeStream : End of the stream flux passed ! Bad Stream flux ?");
				size = track->channels[0].nb_of_pulses - start_index;
			}
		}

		if(flags & 1)
			tickposition = (uint32_t*)calloc( 1, TEMPBUFSIZE*sizeof(uint32_t));

		bitoffset=0;
		if(start_index>2000)
			i = start_index - 2000;
		else
			i = 0;

		if( !hxcfe_getEnvVarValue( floppycontext, "FLUXSTREAM_PLL_NO_PRESYNC" ) )
		{
			if(!pl)
			{
				while(i<(int)start_index)
				{
					value = track->channels[0].stream[i];
					cellcode = getCellTiming(&fxs->pll,value,0,1,phasecorrection);
					i++;
				};
			}
			else
			{
				while(i<(int)start_index)
				{
					value = track->channels[0].stream[i];
					cellcode = getCellTiming(&fxs->pll,value,0,pl->forward_link[i],phasecorrection);
					i++;
				};
			}
		}

		//
		// "Decode" the stream...
		//
		outtrack[0] = 0x80;
		old_bitoffset = 0;

#ifndef USE_PLL_BITRATE
		cumul_code = 0;
		cumul_value = 0;
		in_ptr = 0;
		out_ptr = 0;
		fifolevel = 0;
#endif

		i = 0;

		if(pl)
		{
			while ( ( i < (int)track->nb_of_index ) && ( track->index_evt_tab[i].dump_offset < start_index ) )
			{
				i++;
			}
		}

		nextindex = i;
		nextindex_pos = track->index_evt_tab[i].dump_offset;

		lastbitisflakey = 0;
		tickoffset = 0;
		bitoffset = 0;
		for(i=0;i<size;i++)
		{

			value = track->channels[0].stream[start_index + i];

			tickoffset += value;

			if(!pl)
				cellcode = getCellTiming(&fxs->pll,value,0,1,phasecorrection);
			else
				cellcode = getCellTiming(&fxs->pll,value,0,pl->forward_link[start_index + i],phasecorrection);

			// If bit timing is above the standard spacing, set all cells above the standard as flakey bits
			if( fxs->weak_cell_threshold > 0)
			{
				if(cellcode > fxs->weak_cell_threshold )
				{
					if(lastbitisflakey)
					{
						for(j=0;j<cellcode;j++)
						{
							settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset+j,1);
						}
					}
					else
					{
						for(j=fxs->weak_cell_threshold;j<cellcode;j++)
						{
							settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset+j,1);
						}
					}
				}
			}

			bitoffset = bitoffset + cellcode;

			settrackbit(outtrack,TEMPBUFSIZE,0xFF,bitoffset,1);

			if( nextindex_pos == (start_index + i) )
			{
				#ifdef FLUXSTREAMDBG
				floppycontext->hxc_printf(MSG_DEBUG,"Index reached : Position %d",nextindex_pos);
				#endif

				// Generate index signal
				settrackbit(indextrack,TEMPBUFSIZE,0xFF,bitoffset,1);

				nextindex++;
				if( nextindex < track->nb_of_index )
				{
					nextindex_pos = track->index_evt_tab[nextindex].dump_offset;
				}
			}

			if(!pl)
			{
				if( ( fxs->pll.last_error > (fxs->pll.max_pll_error_ticks * 16) ) || ( fxs->pll.last_error < -( fxs->pll.max_pll_error_ticks * 16) ) )
				{   // flakey bits or invalid bit...
					settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset,1);
					lastbitisflakey = 1;
				}
				else
				{
					lastbitisflakey = 0;
				}
			}
			else
			{
				if((pl->forward_link[start_index + i]<0 && pl->backward_link[start_index + i]<0) || ( ( fxs->pll.last_error > ( fxs->pll.max_pll_error_ticks *16) ) || ( fxs->pll.last_error < -( fxs->pll.max_pll_error_ticks * 16) )))
				{   // flakey bits or invalid bit...
					settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset,1);
					lastbitisflakey = 1;
				}
				else
				{
					lastbitisflakey = 0;
				}
			}

#ifdef USE_PLL_BITRATE
			if( (bitoffset>>3) < TEMPBUFSIZE )
			{
				trackbitrate[(bitoffset>>3)] = (int)((float)(fxs->pll.tick_freq)/(float)((float)((fxs->pll.pump_charge*2)/16)));
				if(tickposition)
					tickposition[(old_bitoffset>>3)] = tickoffset;
			}
#else
			code_array[in_ptr&31] = cellcode;
			value_array[in_ptr&31] = (float)value;
			in_ptr = (in_ptr + 1)&31;
			/*if(fifolevel<32)
				fifolevel++;
			else
				out_ptr = (out_ptr + 1)&31;*/

			cumul_value += value;
			cumul_code += cellcode;

			while(cumul_code>=8)
			{
				byteduration = 0;
				leftbit = 8;
				do
				{
					if( leftbit >= code_array[out_ptr&31] )
					{
						byteduration = byteduration + value_array[out_ptr&31];
						leftbit = leftbit - code_array[out_ptr&31];

						out_ptr = (out_ptr + 1)&31;
						fifolevel--;
					}
					else
					{
						partialduration = value_array[out_ptr&31] * (float)((float)leftbit/(float)code_array[out_ptr&31]);
						byteduration = byteduration + partialduration;

						value_array[out_ptr&31] -= partialduration;
						code_array[out_ptr&31] -= leftbit;
						leftbit = 0;
					}
				}while(leftbit);

				cumul_value = cumul_value - byteduration;
				cumul_code = cumul_code - 8;

				if( (old_bitoffset>>3) < TEMPBUFSIZE )
				{
					trackbitrate[(old_bitoffset>>3)] = (uint32_t)((float)(fxs->pll.tick_freq*4)/(float)byteduration);
					if(tickposition)
						tickposition[(old_bitoffset>>3)] = tickoffset;
					old_bitoffset += 8;
				}
			};
#endif
		}

		// If two adjacent bits are flakey, set all in between cells flakey
		lastpulsebitoffset = -1;
		for(i=0;i<size;i++)
		{
			if(gettrackbit(outtrack,TEMPBUFSIZE,i))
			{
				if(gettrackbit(flakeytrack,TEMPBUFSIZE,i))
				{
					if(lastpulsebitoffset!=-1)
					{
						//set loop
						for(j=lastpulsebitoffset;j<i;j++)
						{
							settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,j,1);
						}

						lastpulsebitoffset = i;
					}
					else
					{
						lastpulsebitoffset = i;
					}
				}
				else
				{
					lastpulsebitoffset = -1;
				}
			}
		}

		// If two adjacent cells are flakey, set all in between cells flakey
		lastpulsebitoffset = -1;
		for(i=0;i<size;i++)
		{
			if(!gettrackbit(outtrack,TEMPBUFSIZE,i))
			{
				if(gettrackbit(flakeytrack,TEMPBUFSIZE,i))
				{
					if(lastpulsebitoffset!=-1)
					{
						//set loop
						for(j=lastpulsebitoffset;j<i;j++)
						{
							settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,j,1);
						}

						lastpulsebitoffset = i;
					}
					else
					{
						lastpulsebitoffset = i;
					}
				}
				else
				{
					lastpulsebitoffset = -1;
				}
			}
			else
			{
				lastpulsebitoffset = -1;
			}
		}

#ifndef USE_PLL_BITRATE
		if(cumul_code)
		{
			byteduration = (float)cumul_value * ((float)8 / (float)cumul_code);

			if( (old_bitoffset>>3) < TEMPBUFSIZE )
			{
				trackbitrate[(old_bitoffset>>3)] = (uint32_t)((float)(fxs->pll.tick_freq)/(float)(byteduration/4));
				if(tickposition)
					tickposition[(old_bitoffset>>3)] = tickoffset;
				old_bitoffset += 8;
			}

		}
#endif

		if(bitoffset&7)
		{
			tracksize = ( bitoffset >> 3 ) + 1;
		}
		else
		{
			tracksize = ( bitoffset >> 3 );
		}

		if( tracksize >= TEMPBUFSIZE )
			tracksize = TEMPBUFSIZE - 1;

		// Bitrate Filter
		if(tracksize)
		{
			if( !(flags & 1) )
			{
				if(fxs->filter)
				{
					for( k = 0; k < fxs->filterpasses; k++ )
					{
						i = 0;
						do
						{
							j = 0;
							bitrate = 0;
							while( ((i+j) < tracksize ) && j < fxs->filter)
							{
								bitrate = ( bitrate + trackbitrate[(i+j)] );
								j++;
							}

							bitrate = bitrate/j;

							j = 0;
							while( ((i+j)< tracksize ) && j < fxs->filter)
							{
								trackbitrate[(i+j)] = bitrate;
								j++;
							}

							if( k == 0 )
							{
								i += ((fxs->filter) / 3 ) * 2;
							}
							else
							{
								i += fxs->filter;
							}

						}while( i < tracksize );
					}
				}
			}

			bitrate=(int)( fxs->pll.tick_freq / (centralvalue) );

			hxcfe_track = tg_alloctrack(bitrate,ISOFORMAT_DD,rpm,bitoffset,0,0,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_ALLOCTIMIMGBUFFER);

			memcpy(hxcfe_track->databuffer,outtrack,tracksize);
			memcpy(hxcfe_track->flakybitsbuffer,flakeytrack,tracksize);
			memcpy(hxcfe_track->timingbuffer,trackbitrate, tracksize * sizeof(uint32_t));

			if(tickposition)
			{
				hxcfe_track->cell_to_tick = malloc(tracksize * sizeof(uint32_t));
				if(hxcfe_track->cell_to_tick)
				{
					memcpy(hxcfe_track->cell_to_tick,tickposition, tracksize * sizeof(uint32_t));
				}
			}
			else
			{
				hxcfe_track->cell_to_tick = NULL;
			}

			hxcfe_track->tick_freq = fxs->pll.tick_freq;

			memset(hxcfe_track->indexbuffer,0,tracksize);

			// add sector/track index
			for(i=0;i<bitoffset;i++)
			{
				if(getbit(indextrack,i))
				{
					us2index(i % bitoffset,hxcfe_track,fxs->out_index_length_us,1,0);
				}
			}

			hxcfe_track->bitrate = VARIABLEBITRATE;
		}

		free(tickposition);
		free(trackbitrate);
		free(indextrack);
		free(flakeytrack);
		free(outtrack);
	}

	return hxcfe_track;
}

static uint32_t tick_to_time(HXCFE_FXSA * fxs,uint32_t tick)
{
	return (uint32_t) (double)( (double)tick  * (double)( (double)( 1000 * 1000 * 100 ) / (double)fxs->pll.tick_freq ) );
}

static uint32_t time_to_tick(HXCFE_FXSA * fxs,uint32_t time)
{
	return (uint32_t) (double)(( (double)time * fxs->pll.tick_freq ) / (1000 * 1000 * 100 ) );
}

int cleanupTrack(HXCFE_SIDE *curside)
{
	uint32_t tracklen,k;
	unsigned char l;
	unsigned char previous_bit;
	int bitpatched;

	bitpatched = 0;

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
					curside->databuffer[k] = (uint8_t)(curside->databuffer[k] ^ 0x80);
					curside->flakybitsbuffer[k] =  (uint8_t)(curside->flakybitsbuffer[k] | (0x80));
					bitpatched++;
				}
			}

			for(l=0;l<7;l++)
			{
				if((curside->databuffer[k] & (0xC0>>l)) == (0xC0>>l))
				{
					curside->databuffer[k] = (uint8_t)(curside->databuffer[k] ^ (0x40>>l));
					curside->flakybitsbuffer[k] =  (uint8_t)(curside->flakybitsbuffer[k] | (0x40>>l));
					bitpatched++;
				}
			}

			previous_bit = (uint8_t)(curside->databuffer[k] & 1);
		}
	}
	return bitpatched;
}

static uint32_t GetDumpTimelength(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * track_dump)
{
	uint32_t len;

	uint32_t nb_pulses,i;

	len = 0;

	if(track_dump)
	{
		nb_pulses = track_dump->channels[0].nb_of_pulses;

		for(i=0;i<nb_pulses;i++)
		{
			len = len + track_dump->channels[0].stream[i];
		}
	}

	return tick_to_time(fxs,len);
}

static track_blocks * AllocateBlocks(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * track_dump, int blocktimelength)
{
	uint32_t i,j;
	uint32_t len;
	uint32_t blocklen;
	uint32_t number_of_blocks;
	int32_t timeoffset;
	int64_t tickoffset;
	track_blocks * trackb;

	trackb = malloc(sizeof(track_blocks));
	if(trackb)
	{
		// Count the needed block number.
		j = 0;
		number_of_blocks = 0;
		do
		{
			blocklen = time_to_tick( fxs, blocktimelength * 100 + ( ( number_of_blocks%7 ) * 75 ) );

			len = 0;
			while(len < blocklen && j < track_dump->channels[0].nb_of_pulses)
			{
				len = len + track_dump->channels[0].stream[j];
				j++;
			};
			number_of_blocks++;

		}while(j < track_dump->channels[0].nb_of_pulses);

		// Allocate and init the blocks.
		trackb->number_of_blocks = number_of_blocks;
		trackb->blocks = malloc(sizeof(pulsesblock) * number_of_blocks);
		if(trackb->blocks)
		{
			memset(trackb->blocks,0, sizeof(pulsesblock) * number_of_blocks);

			tickoffset = 0;
			timeoffset = 0;
			j = 0;
			i = 0;
			do
			{
				blocklen = time_to_tick( fxs, blocktimelength * 100 + ( (i%7) * 75 ) );

				trackb->blocks[i].start_index = j;

				len = 0;
				while(len < blocklen && j < track_dump->channels[0].nb_of_pulses)
				{
					len = len + track_dump->channels[0].stream[j];
					j++;
				};

				trackb->blocks[i].timeoffset = timeoffset;
				trackb->blocks[i].tickoffset = tickoffset;
				trackb->blocks[i].end_index = j;
				trackb->blocks[i].ticklength = len;
				trackb->blocks[i].timelength = tick_to_time(fxs,len);
				trackb->blocks[i].number_of_pulses = ( trackb->blocks[i].end_index - trackb->blocks[i].start_index );

				timeoffset = timeoffset + trackb->blocks[i].timelength;
				tickoffset = tickoffset + len;
				i++;

			}while(j < track_dump->channels[0].nb_of_pulses);

			return trackb;
		}

		free(trackb);
	}

	return 0;
}

static void compareblock(HXCFE_TRKSTREAM * td,pulsesblock * src_block, uint32_t dst_block_offset,uint32_t * pulses_ok,uint32_t * pulses_failed,int partial, int max_skew)
{
	int marge;
	int time1,time2;
	int bad_pulses,good_pulses;

	uint32_t *start_ptr;
	uint32_t *dst_ptr;
	uint32_t *last_dump_ptr;
	uint32_t *last_block_ptr;
	uint32_t src_number_of_pulses;

	bad_pulses = 0;
	good_pulses = 0;
	src_number_of_pulses = src_block->number_of_pulses;

	if( dst_block_offset >= td->channels[0].nb_of_pulses)
	{
		if(pulses_ok)
			*pulses_ok = 0;

		if(pulses_failed)
			*pulses_failed = 0;

		return;
	}

	if(dst_block_offset + src_number_of_pulses >= td->channels[0].nb_of_pulses)
	{
		src_number_of_pulses = td->channels[0].nb_of_pulses - dst_block_offset;
	}

	src_block->overlap_offset = dst_block_offset;

	start_ptr = &td->channels[0].stream[src_block->start_index];
	dst_ptr = &td->channels[0].stream[dst_block_offset];
	last_dump_ptr = &td->channels[0].stream[td->channels[0].nb_of_pulses - 2];

	if(partial)
		last_block_ptr = &td->channels[0].stream[src_block->start_index + (src_number_of_pulses/100)];
	else
		last_block_ptr = &td->channels[0].stream[src_block->start_index + (src_number_of_pulses)];

	if( dst_ptr < last_dump_ptr && start_ptr < last_block_ptr )
	{
		time1 = *(start_ptr);
		time2 = *(dst_ptr);

		marge = ( ( time1 * max_skew ) >> 8 );

		while( (dst_ptr + 8) < last_dump_ptr && (start_ptr+8) < last_block_ptr)
		{
			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);
				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );

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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

		}

		while( (dst_ptr) < last_dump_ptr && (start_ptr) < last_block_ptr)
		{

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				bad_pulses++;

				marge = ( ( time1 * max_skew ) >> 8 );
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

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}
		}
	}

	src_block->overlap_size = dst_ptr - &td->channels[0].stream[dst_block_offset];

	if(pulses_ok)
		*pulses_ok = good_pulses;

	if(pulses_failed)
		*pulses_failed = bad_pulses;

}

#ifdef EXPAND_MATCHING_BLOCK

static int expandblock(HXCFE_TRKSTREAM * td,pulsesblock * src_block, uint32_t dst_block_offset,uint32_t * pulses_ok,uint32_t * pulses_failed,int partial, int max_skew)
{
	int marge;
	int time1,time2;
	int bad_pulses,good_pulses;

	uint32_t *start_ptr;
	uint32_t *dst_ptr;
	uint32_t *last_dump_ptr;
	uint32_t *last_block_ptr;
	uint32_t src_number_of_pulses;

	bad_pulses = 0;
	good_pulses = 0;
	src_number_of_pulses = src_block->number_of_pulses;

	if( dst_block_offset >= td->channels[0].nb_of_pulses)
	{
		if(pulses_ok)
			*pulses_ok = 0;

		if(pulses_failed)
			*pulses_failed = 0;

		return 0;
	}

	if(dst_block_offset + src_number_of_pulses >= td->channels[0].nb_of_pulses)
	{
		src_number_of_pulses = td->channels[0].nb_of_pulses - dst_block_offset;
	}

	src_block->overlap_offset = dst_block_offset;

	start_ptr = &td->channels[0].stream[src_block->start_index];
	dst_ptr = &td->channels[0].stream[dst_block_offset];
	last_dump_ptr = &td->channels[0].stream[td->channels[0].nb_of_pulses - 2];

	last_block_ptr = last_dump_ptr;

	if( dst_ptr < last_dump_ptr && start_ptr < last_block_ptr )
	{
		time1 = *(start_ptr);
		time2 = *(dst_ptr);

		marge = ( ( time1 * max_skew ) >> 8 );

		while( (dst_ptr + 8) < last_dump_ptr && (start_ptr+8) < last_block_ptr)
		{
			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}


			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}

		}

		while( (dst_ptr) < last_dump_ptr && (start_ptr) < last_block_ptr)
		{

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				goto exit_func;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					goto exit_func;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;

					marge = ( ( time1 * max_skew ) >> 8 );
				}
			}
		}
	}

	src_block->overlap_size = dst_ptr - &td->channels[0].stream[dst_block_offset];

exit_func:
	if(pulses_ok)
		*pulses_ok = good_pulses;

	if(pulses_failed)
		*pulses_failed = bad_pulses;

	return good_pulses;

}
#endif

static int fastcompareblock(HXCFE_TRKSTREAM * td,pulsesblock * src_block, uint32_t dst_block_offset, int max_skew)
{
	int32_t marge;
	int time1,time2;

	uint32_t *start_ptr;
	uint32_t *dst_ptr;
	uint32_t *last_dump_ptr;
	uint32_t *last_block_ptr;

	int errorcntdown;

	errorcntdown = 16;

	if(dst_block_offset + src_block->number_of_pulses >= td->channels[0].nb_of_pulses)
	{
		return 0;
	}

	src_block->overlap_offset = dst_block_offset;

	start_ptr = &td->channels[0].stream[src_block->start_index];
	dst_ptr = &td->channels[0].stream[dst_block_offset];
	last_dump_ptr = &td->channels[0].stream[td->channels[0].nb_of_pulses - 2];

	last_block_ptr = &td->channels[0].stream[src_block->start_index + (src_block->number_of_pulses)];

	if( dst_ptr < last_dump_ptr && start_ptr < last_block_ptr )
	{
		time1 = *(start_ptr);
		time2 = *(dst_ptr);

		while( dst_ptr < last_dump_ptr && start_ptr < last_block_ptr)
		{
			marge = ( ( time1 * max_skew ) >> 8 );

			if( time2 > ( time1 + marge ) )
			{
				time1 = time1 + *(++start_ptr);

				errorcntdown--;
				if(!errorcntdown)
					return 0;
			}
			else
			{
				if( time2 < ( time1 - marge ) )
				{
					time2 = time2 + *(++dst_ptr);

					errorcntdown--;
					if(!errorcntdown)
						return 0;
				}
				else
				{
					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);
				}
			}
		}
	}

	return 1;
}

static uint32_t detectflakeybits(HXCFE_TRKSTREAM * td,pulsesblock * src_block, uint32_t dst_block_offset,uint32_t * pulses_ok,uint32_t * pulses_failed,pulses_link * pl,uint32_t maxptr, int max_skew)
{
	int32_t marge;
	int time1,time2;
	int bad_pulses,good_pulses;
	int32_t *forward_link;

	uint32_t *srcptr,*start_ptr, *dst_ptr, *last_dump_ptr,*last_block_ptr,*maxptr5;
	uint32_t src_number_of_pulses;

	forward_link = pl->forward_link;
	bad_pulses = 0;
	good_pulses = 0;
	src_number_of_pulses = src_block->number_of_pulses;

	if(dst_block_offset >= td->channels[0].nb_of_pulses)
	{
		if(pulses_ok)
			*pulses_ok = 0;

		if(pulses_failed)
			*pulses_failed = 0;

		return 0;
	}

	if(dst_block_offset + src_number_of_pulses >= td->channels[0].nb_of_pulses)
	{
		src_number_of_pulses = td->channels[0].nb_of_pulses - dst_block_offset;
	}

	srcptr        = td->channels[0].stream;

	start_ptr     = &td->channels[0].stream[src_block->start_index];
	dst_ptr       = &td->channels[0].stream[dst_block_offset];
	last_dump_ptr = &td->channels[0].stream[td->channels[0].nb_of_pulses - 2];
	last_block_ptr= &td->channels[0].stream[src_block->start_index + src_number_of_pulses];

	if(maxptr == 0xFFFFFFFF)
	{
		maxptr5 = &td->channels[0].stream[td->channels[0].nb_of_pulses-1];
	}
	else
	{
		maxptr5 = &td->channels[0].stream[maxptr];
	}

	if( dst_ptr < last_dump_ptr && start_ptr < last_block_ptr )
	{
		time1 = *(start_ptr);
		time2 = *(dst_ptr);

		while(dst_ptr < last_dump_ptr && start_ptr < last_block_ptr)
		{
			marge = ( ( time1 * max_skew ) >> 8 );

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
					if(dst_ptr < maxptr5)// && start_ptr < last_dump_ptr)
					{
						forward_link[(start_ptr) - srcptr] = ((dst_ptr) - srcptr);
					}

					time1 = *(++start_ptr);
					time2 = *(++dst_ptr);

					good_pulses++;
				}
			}
		}
	}

	if(pulses_ok)
		*pulses_ok = good_pulses;

	if(pulses_failed)
		*pulses_failed = bad_pulses;

	return (dst_ptr - srcptr);
}

static uint32_t getnearestbit(uint32_t * src,uint32_t index,uint32_t * dst,uint32_t buflen,int32_t * shift)
{
	uint32_t i,ret_value;

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

			if( ((index + *shift) - i) < buflen )
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

static uint32_t compare_block_timebased(HXCFE* floppycontext,HXCFE_TRKSTREAM * td,HXCFE_FXSA * fxs,pulsesblock * prev_block,pulsesblock * src_block,pulsesblock * next_block, pulses_link * pl,int32_t *in_shift)
{
	uint32_t i;
	uint32_t src_start_index,src_last_index;
	uint32_t dst_start_index,dst_last_index;
	uint32_t total_tick_source,total_tick_source2;
	uint32_t total_tick_destination;
	uint32_t time_index;

	uint32_t * time_pulse_array_src;
	uint32_t * time_pulse_array_dst;

	uint32_t time_buffer_len,t;

	uint32_t bad_pulses,good_pulses;

	int32_t * forward_link;

	double timefactor;

	int32_t shift;

	if(!floppycontext)
		return 0;

	shift = 0;
	if(in_shift)
		shift = *in_shift;

	good_pulses = 0;
	bad_pulses = 0;

	// total time of the source block
	src_start_index = prev_block->start_index + prev_block->number_of_pulses;
	src_last_index  = next_block->start_index;

	forward_link = pl->forward_link;

	i = src_start_index;
	forward_link = &pl->forward_link[i];
	total_tick_source = 0;
	while( ( i < src_last_index ) && ( i < td->channels[0].nb_of_pulses ) )
	{
		*(forward_link++) = -1;
		total_tick_source = total_tick_source + td->channels[0].stream[i];
		i++;
	};

	// total time of the other block
	dst_start_index = prev_block->overlap_offset + prev_block->overlap_size;
	dst_last_index  = next_block->overlap_offset;

	i = dst_start_index;
	total_tick_destination = 0;
	while(i< dst_last_index && i < td->channels[0].nb_of_pulses)
	{
		total_tick_destination = total_tick_destination + td->channels[0].stream[i];
		i++;
	};

	time_buffer_len = ((tick_to_time(fxs,total_tick_source)/10) + 1024);

	time_pulse_array_src = calloc( 1, time_buffer_len * sizeof(uint32_t) );
	time_pulse_array_dst = calloc( 1, time_buffer_len * sizeof(uint32_t) );

	if( time_buffer_len && (!time_pulse_array_src || !time_pulse_array_dst))
	{
		free(time_pulse_array_src);
		free(time_pulse_array_dst);
		return 0;
	}

	timefactor = (double)total_tick_source / (double)total_tick_destination ;

	i = src_start_index;
	total_tick_source2 = 0;
	while(i <= src_last_index)
	{
		total_tick_source2 = total_tick_source2 + td->channels[0].stream[i];

		time_index = tick_to_time(fxs,total_tick_source2) / 10;

		if(time_index<time_buffer_len)
			time_pulse_array_src[time_index] = i;
		i++;
	};

	i = dst_start_index;
	total_tick_destination = 0;
	while(i <= dst_last_index)
	{
		total_tick_destination = total_tick_destination + td->channels[0].stream[i];

		time_index = tick_to_time(fxs,(uint32_t)((double)total_tick_destination*timefactor)) / 10;

		if(time_index<time_buffer_len)
			time_pulse_array_dst[time_index] = i;

		i++;
	};


	forward_link = pl->forward_link;
	i = 0;
	while(i< time_buffer_len)
	{
		if(time_pulse_array_src[i])
		{
			t = getnearestbit(time_pulse_array_src,i,time_pulse_array_dst,time_buffer_len,&shift);

			forward_link[time_pulse_array_src[i]] = t;

			if(t)
				good_pulses++;
			else
				bad_pulses++;
		}
		i++;
	};


	src_block->start_index = src_start_index;
	src_block->number_of_pulses = src_last_index - src_start_index;

	src_block->overlap_offset = dst_start_index;
	src_block->overlap_size = next_block->overlap_offset - dst_start_index;

	src_block->locked = 1;

	free(time_pulse_array_src);
	free(time_pulse_array_dst);

#ifdef FLUXSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"Source block tick len : %d (%d us) Destination block tick len :%d (%d us), good:%d,bad:%d",total_tick_source,tick_to_time(fxs,total_tick_source)/100,total_tick_destination,tick_to_time(fxs,total_tick_destination)/100,good_pulses,bad_pulses);
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

const char *  getStateStr(uint32_t state)
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

static int getNearestMatchedBlock(pulsesblock * pb, int dir, int currentblock,int nbblock)
{
	int cntblock;

	cntblock = 0;
	if(!dir)
	{
		while(currentblock)
		{
			currentblock--;

			if(pb[currentblock].locked  &&
				(   ( pb[currentblock].state == ONEMATCH_STATE ) ||
					( pb[currentblock].state == MATCH_STATE ) ) )
			{
				cntblock++;
				if( cntblock == 3 )
					return currentblock + 2;
			}
			else
			{
				cntblock = 0;
			}
		}

		return -1;
	}
	else
	{
		while(currentblock < nbblock-1)
		{
			currentblock++;

			if(pb[currentblock].locked  &&
				(   ( pb[currentblock].state == ONEMATCH_STATE ) ||
					( pb[currentblock].state == MATCH_STATE ) ) )
			{
				return currentblock;
			}
		}

		return -1;
	}
}

uint32_t getNearestValidIndex(pulses_link * pl,uint32_t center,uint32_t limit)
{
	int32_t i;
	int32_t offset_max;

	i = 0;
	do
	{
		//offset_min = (int32_t)center - i;
		offset_max = (int32_t)center + i;

		if(offset_max < (int32_t)pl->number_of_pulses)
		{
			if( pl->forward_link[offset_max] >= 0 )
			{
				return (uint32_t)offset_max;
			}
		}

	/*  if(offset_min >= 0)
		{
			if(pl->forward_link[offset_min]>=0)
				return (uint32_t)offset_min;
		}*/

		i++;
	}while(i<(int32_t)limit);

	return pl->number_of_pulses - 1;
}

uint32_t searchBestOverlap(HXCFE* floppycontext,HXCFE_TRKSTREAM * track_dump, pulsesblock * src_block, uint32_t repeat_index, uint32_t tick_down_max, uint32_t tick_up_max, s_match * match_table,int * searchstate,int fullcompare, int max_jitter)
{
	uint32_t i_down,i_up;
	uint32_t tick_up;
	uint32_t tick_down;
	uint32_t mt_i;
	uint32_t good,bad;
	int goodblockfound;

	tick_up = 0;
	tick_down = 0;

	i_up = 0;
	i_down = 0;

	memset(match_table , 0,sizeof(s_match) * track_dump->channels[0].nb_of_pulses);

	mt_i = 0;

	if(fullcompare)
	{
		do
		{
			if(tick_up < tick_up_max)
			{
				if(repeat_index + i_up < track_dump->channels[0].nb_of_pulses)
				{
					// compare the block
					//mmx_blk_compare (uint32_t * src_ptr,uint32_t * dst_ptr,uint32_t * last_src_ptr,uint32_t * last_dst_ptr, int * bad_pulses, int * good_pulses);
					compareblock(track_dump,src_block, repeat_index + i_up,&good,&bad,0,max_jitter);

					match_table[mt_i].yes = good;
					match_table[mt_i].no = bad;
					match_table[mt_i].offset = repeat_index + i_up;

					tick_up = tick_up + track_dump->channels[0].stream[repeat_index + i_up];
					i_up++;

					if(mt_i < track_dump->channels[0].nb_of_pulses ) mt_i++;
				}
				else
				{
					tick_up = tick_up_max;
				}
			}

		}while( tick_up < tick_up_max);

		// i_down = 0 case already tested in the previous loop !
		i_down = 1;

		do
		{
			if(tick_down < tick_down_max)
			{
				if(repeat_index - i_down < track_dump->channels[0].nb_of_pulses)
				{
					// compare the block
					compareblock(track_dump,src_block, repeat_index - i_down,&good,&bad,0,max_jitter);

					match_table[mt_i].yes = good;
					match_table[mt_i].no = bad;
					match_table[mt_i].offset = repeat_index - i_down;

					tick_down = tick_down + track_dump->channels[0].stream[repeat_index - i_down];
					i_down++;

					if(mt_i< track_dump->channels[0].nb_of_pulses ) mt_i++;
				}
				else
				{
					tick_down = tick_down_max;
				}
			}

		}while((tick_down < tick_down_max) );

		quickSort(match_table, 0, mt_i-1);
	}
	else
	{
		goodblockfound = 0;

		do
		{

			if(tick_up < tick_up_max)
			{
				if(repeat_index + i_up < track_dump->channels[0].nb_of_pulses)
				{
					// compare the block
					if(fastcompareblock(track_dump,src_block, repeat_index + i_up,max_jitter))
					{
						goodblockfound = 1;
					}

					tick_up = tick_up + track_dump->channels[0].stream[repeat_index + i_up];
					i_up++;
				}
				else
				{
					tick_up = tick_up_max;
				}
			}

			if(tick_down < tick_down_max)
			{
				if(repeat_index - i_down < track_dump->channels[0].nb_of_pulses)
				{
					// compare the block
					if(fastcompareblock(track_dump,src_block, repeat_index - i_down,max_jitter))
					{
						goodblockfound = 1;
					}

					tick_down = tick_down + track_dump->channels[0].stream[repeat_index - i_down];
					i_down++;
				}
				else
				{
					tick_down = tick_down_max;
				}
			}

		}while( !goodblockfound && ( (tick_up < tick_up_max) || (tick_down < tick_down_max) ) );

		*searchstate = 0;
		if(goodblockfound)
		{
			*searchstate = 2;
		}

		return 0;
	}

	// -> No offset match at 100% -> take the best one
	*searchstate = 0;

	// -> Only one offset match at 100% -> We have found the overlap
	if( mt_i >= 2 )
	{
		if( match_table[mt_i-1].no == 0 && match_table[mt_i-2].no != 0 && match_table[mt_i-1].yes )
		{
			*searchstate = 2;
		}

		// -> different offset match at 100%
		if( match_table[mt_i-1].no == 0 && match_table[mt_i-2].no == 0 && match_table[mt_i-1].yes )
		{
	#ifdef FLUXSTREAMDBG
			int i,c;

			c = 0;
			for(i=0;i<mt_i;i++)
			{
				if(!match_table[i].no)
				{
					c++;
				}
			}
			floppycontext->hxc_printf(MSG_DEBUG,"searchBestOverlap : MULTI_MATCH : %d / %d",c,tick_down_max+tick_up_max);
	#endif

			*searchstate = 1;
		}
	}
	return mt_i;
}

#ifdef FLUXSTREAMDBG
static int GetTickCnt(HXCFE_TRKSTREAM * track_dump,unsigned int start, unsigned int end)
{
	unsigned int i,ticknum;

	ticknum = 0;
	i = start;
	while( (i< track_dump->channels[0].nb_of_pulses) && ( i < end ) )
	{
		ticknum = ticknum + track_dump->channels[0].stream[i];
		i++;
	}

	return ticknum;
}
#endif

pulses_link * alloc_pulses_link_array(int numberofpulses)
{
	int32_t i;
	pulses_link * pl;

	pl = malloc(sizeof(pulses_link));
	if(pl)
	{
		memset(pl,0,sizeof(pulses_link));

		pl->number_of_pulses = numberofpulses;

		pl->backward_link = (int32_t*)malloc( (numberofpulses+1)*sizeof(int32_t) );
		pl->forward_link = (int32_t*)malloc( (numberofpulses+1)*sizeof(int32_t) );

		if(!pl->backward_link || !pl->forward_link)
		{
			free(pl->backward_link);
			free(pl->forward_link);
			free(pl);

			return NULL;
		}

		// -1 Uninitialized
		// -2 No link
		// -3 No link- End of dump

		for(i=0;i<(pl->number_of_pulses+1);i++)
		{
			pl->backward_link[i] = -1;
			pl->forward_link[i] = -1;
		}
	}

	return pl;
}

void free_pulses_link_array(pulses_link * pl)
{
	if(pl)
	{
		free(pl->backward_link);
		free(pl->forward_link);
		free(pl);
	}
}

static pulses_link * ScanAndFindRepeatedBlocks(HXCFE* floppycontext,HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * track_dump,uint32_t indexperiod,track_blocks * tb)
{
#ifdef SECONDPASSANALYSIS
	uint32_t block_analysed;
#endif
	uint32_t j;
	uint32_t cur_time_len;
	uint32_t index_tickpediod;
	int previous_block_matched;

	uint32_t good,bad;

	uint32_t tick_up_max;
	uint32_t tick_down_max;

	uint32_t mt_i, block_num,nb_of_multimatch;

	uint32_t pass_loop;

	unsigned char c;

	int32_t t;

	int k;

	int searchstate;

	int previousmatchedblock;

	int32_t shift;

	int end_dump_reached;

	int next_locked_block;
	int previous_locked_block;

	double search_depth;
	s_match * match_table;
	char * conv_error;
	char * tmp_str;

	int32_t i;

	pulses_link * pl;

	floppycont = floppycontext;

	index_tickpediod = time_to_tick(fxs, indexperiod);

	previous_block_matched = 0;

	end_dump_reached = 0;

	pl = NULL;

	conv_error = NULL;
	tmp_str = hxcfe_getEnvVar( fxs->hxcfe, "FLUXSTREAM_OVERLAPSEARCHDEPTH", NULL);
	search_depth = strtod( tmp_str, &conv_error);
	if(conv_error == tmp_str)
	{
		search_depth = (double)DEFAULT_SEARCHDEPTH;
	}

	if(track_dump->channels[0].nb_of_pulses)
	{
		// Only one revolution -> No flakey bits detection : return a dummy buffer.
		if( ( hxcfe_FxStream_GetNumberOfRevolution(fxs,track_dump) == 1 ) || hxcfe_getEnvVarValue( fxs->hxcfe, "FLUXSTREAM_SKIPBLOCKSDETECTION") )
		{

#ifdef FLUXSTREAMDBG
			floppycontext->hxc_printf(MSG_DEBUG,"ScanAndFindRepeatedBlocks : No flakey bits detection : return a dummy buffer.");
#endif

			pl = alloc_pulses_link_array(track_dump->channels[0].nb_of_pulses);
			if(pl)
			{
				for(i=0;i<pl->number_of_pulses;i++)
				{
					pl->forward_link[i] = i + 1;
				}

				if( hxcfe_FxStream_GetNumberOfRevolution(fxs,track_dump) == 1 )
				{
					int start_index_pos,end_index_pos;

					start_index_pos = hxcfe_FxStream_GetRevolutionIndex( fxs, track_dump, 0 );
					end_index_pos = hxcfe_FxStream_GetRevolutionIndex( fxs, track_dump, 1 );

					if(start_index_pos < end_index_pos)
					{
						// fix index forward link.
						if(
							( track_dump->index_evt_tab[start_index_pos].dump_offset < track_dump->index_evt_tab[end_index_pos].dump_offset ) &&
							( ( track_dump->index_evt_tab[start_index_pos].dump_offset >= 0 ) &&  ( track_dump->index_evt_tab[end_index_pos].dump_offset >= 0 ) )
						)
						{
							pl->forward_link[ track_dump->index_evt_tab[start_index_pos].dump_offset ] =  track_dump->index_evt_tab[end_index_pos].dump_offset;
						}
					}
				}

				for(i=0;i<pl->number_of_pulses;i++)
				{
					if( ( pl->forward_link[i] >= 0 ) && ( pl->forward_link[i] < (int32_t)pl->number_of_pulses ) )
					{
						pl->backward_link[ pl->forward_link[i] ] = i;
					}
				}
			}

			return pl;
		}

		match_table = malloc(sizeof(s_match) * track_dump->channels[0].nb_of_pulses);
		if(match_table)
		{
			memset(match_table , 0,sizeof(s_match) * track_dump->channels[0].nb_of_pulses);

			pl = alloc_pulses_link_array(track_dump->channels[0].nb_of_pulses);
			if(!pl)
			{
				free(match_table);
				return NULL;
			}

#ifdef FLUXSTREAMDBG
			floppycontext->hxc_printf(MSG_DEBUG,"Number of pulses : %d",pl->number_of_pulses);
#endif

			/////////////////////////////////////////////////////////////////////////////////
			// Partial analysis...
			// Unformated track detection.
			block_num=0;
			do
			{
				// find index point.
				j = tb->blocks[block_num].start_index;

				cur_time_len = 0;
				do
				{

					cur_time_len = cur_time_len + track_dump->channels[0].stream[j];

					j++;

				}while( (cur_time_len < index_tickpediod) && (j < track_dump->channels[0].nb_of_pulses) );

				tick_down_max = time_to_tick( fxs, (uint32_t)((double)indexperiod * search_depth));
				tick_up_max =   time_to_tick( fxs, (uint32_t)((double)indexperiod * search_depth));

				searchstate = 0;

				if(j < track_dump->channels[0].nb_of_pulses )
				{
					searchBestOverlap(floppycontext,track_dump, &tb->blocks[block_num], j - 1, tick_down_max, tick_up_max, match_table,&searchstate,0,fxs->analysis_rev2rev_max_pulses_jitter);
				}

				block_num++;

			}while( !searchstate && ( (block_num<tb->number_of_blocks) && (j < track_dump->channels[0].nb_of_pulses)  && !end_dump_reached));


			/////////////////////////////////////////////////////////////////////////////////
			// Full analysis...

			if(searchstate)
			{
#ifdef FLUXSTREAMDBG
				floppycontext->hxc_printf(MSG_DEBUG,"-----------------------------");
				floppycontext->hxc_printf(MSG_DEBUG,"--   First pass analysis ! --");
				floppycontext->hxc_printf(MSG_DEBUG,"-----------------------------");
#endif
				block_num=0;
				do
				{
					if( previous_block_matched )
					{
						// compare the blocks
						compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);

						if(good && !bad)
						{
							#ifdef FLUXSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d. [F]",block_num,tb->blocks[block_num].number_of_pulses,tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses);
							#endif

							tb->blocks[block_num].state = MATCH_STATE;

							tb->blocks[block_num].locked = 1;

							block_num++;
							previous_block_matched++;
						}
						else
						{
							if(!good && !bad)
							{
								#ifdef FLUXSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"End of the track dump");
								#endif

								end_dump_reached = 1;
							}
							else
							{
								#ifdef FLUXSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index - offset %d) : Bad pulses found... full analysis",block_num,tb->blocks[block_num].number_of_pulses,tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses);
								#endif

								previous_block_matched = 0;
							}
						}
					}
					else
					{

#ifdef FLUXSTREAMDBG
						floppycontext->hxc_printf(MSG_DEBUG,"Block %d : The previous block doesn't match",block_num);
#endif

						previousmatchedblock = getNearestMatchedBlock( tb->blocks, 0, block_num,tb->number_of_blocks);

						if(previousmatchedblock >= 0)
						{

							j = tb->blocks[previousmatchedblock].overlap_offset;

							cur_time_len = 0;
							do
							{

								cur_time_len = cur_time_len + track_dump->channels[0].stream[j];

								j++;
							}while( (cur_time_len < (tb->blocks[block_num].tickoffset - tb->blocks[previousmatchedblock].tickoffset) ) && (j < track_dump->channels[0].nb_of_pulses) );

							tick_down_max = (uint32_t)((double)(tb->blocks[block_num].tickoffset - tb->blocks[previousmatchedblock].tickoffset) * (double)search_depth*2);
							tick_up_max =   tick_down_max;
#ifdef FLUXSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"Block %d : Match block distance : %d",block_num,previousmatchedblock);
#endif
						}
						else
						{

							// find index point.
							j = tb->blocks[block_num].start_index;

							cur_time_len = 0;
							do
							{

								cur_time_len = cur_time_len + track_dump->channels[0].stream[j];

								j++;

							}while( (cur_time_len < index_tickpediod) && (j < track_dump->channels[0].nb_of_pulses) );

							tick_down_max = time_to_tick( fxs, (uint32_t)((double)indexperiod * (double)0.0125));
							tick_up_max =   time_to_tick( fxs, (uint32_t)((double)indexperiod * (double)0.0125));

#ifdef FLUXSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"Block %d : index distance (period : %d) tick_down_max : %d, tick_up_max : %d",block_num,indexperiod,tick_to_time(fxs,tick_down_max),tick_to_time(fxs,tick_up_max));
#endif

						}

						if(j < track_dump->channels[0].nb_of_pulses )
						{

							mt_i = searchBestOverlap(floppycontext,track_dump, &tb->blocks[block_num], j - 1, tick_down_max, tick_up_max, match_table,&searchstate,1,fxs->analysis_rev2rev_max_pulses_jitter);

							switch(searchstate)
							{
								// -> Only one offset match at 100% -> We have found the overlap
								case 2:
									#ifdef FLUXSTREAMDBG
										floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d (delta : %d).",block_num,tb->blocks[block_num].number_of_pulses,match_table[mt_i-1].offset,tick_to_time(fxs,GetTickCnt(track_dump,tb->blocks[block_num].start_index,match_table[mt_i-1].offset)));
									#endif

									tb->blocks[block_num].state = ONEMATCH_STATE;
									tb->blocks[block_num].overlap_offset = match_table[mt_i-1].offset;
									tb->blocks[block_num].overlap_size = tb->blocks[block_num].number_of_pulses;

									tb->blocks[block_num].locked = 1;

#ifdef EXPAND_MATCHING_BLOCK
									// Try to expand this block
									int tmp_ret;
									tmp_ret = expandblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num].overlap_offset,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
									if(good && !bad)
									{
#ifdef FLUXSTREAMDBG
										floppycontext->hxc_printf(MSG_DEBUG,"Match block resized from %d pulses to %d pulses",tb->blocks[block_num].overlap_size,tmp_ret);
#endif

										tb->blocks[block_num].overlap_size = tmp_ret;
										tb->blocks[block_num].number_of_pulses = tmp_ret;

										int blk_o_i = 0;
										for( int blk_i = 0; blk_i < tb->number_of_blocks; blk_i++)
										{
											if( blk_i != block_num)
											{
												if( ( tb->blocks[blk_i].start_index >= tb->blocks[block_num].start_index ) &&
													( tb->blocks[blk_i].start_index < ( tb->blocks[block_num].start_index + tb->blocks[block_num].overlap_size ) ) )
												{
													// starting inside the matching one
													if(
														( ( tb->blocks[blk_i].start_index + tb->blocks[blk_i].number_of_pulses ) <=
														  ( tb->blocks[block_num].start_index + tb->blocks[block_num].number_of_pulses ) )
														)
													{
														// Completly covered - remove it
													}
													else
													{
														// Partial - reajust start point
														tb->blocks[blk_i].number_of_pulses -= ( ( tb->blocks[block_num].start_index + tb->blocks[block_num].number_of_pulses ) - tb->blocks[blk_i].start_index);
														tb->blocks[blk_i].start_index = tb->blocks[block_num].start_index + tb->blocks[block_num].number_of_pulses;
														if( tb->blocks[blk_i].number_of_pulses > 0)
														{
															memcpy(&tb->blocks[blk_o_i++],&tb->blocks[blk_i],sizeof(pulsesblock));
														}
													}
												}
												else
												{
													memcpy(&tb->blocks[blk_o_i++],&tb->blocks[blk_i],sizeof(pulsesblock));
												}
											}
											else
											{
												memcpy(&tb->blocks[blk_o_i++],&tb->blocks[blk_i],sizeof(pulsesblock));
											}
										}

#ifdef FLUXSTREAMDBG
										floppycontext->hxc_printf(MSG_DEBUG,"Blocks count updated : %d -> %d",tb->number_of_blocks,blk_o_i);
#endif

										tb->number_of_blocks = blk_o_i;
									}
#endif

									previous_block_matched++;
								break;

								// -> different offset match at 100%
								case 1:
									tb->blocks[block_num].state = MULTIMATCH_STATE;

									j = 0;
									nb_of_multimatch = 0;

									while(j < mt_i)
									{
										if((match_table[(mt_i-j)-1].no == 0) && match_table[(mt_i-j)-1].yes)
										{
											nb_of_multimatch++;
										}
										j++;
									}

									if(block_num)
									{
										// check if one position match with the previous block offset
										if( tb->blocks[block_num-1].locked )
										{
											j = mt_i-1;
											do
											{
												j--;
											}while(j && (match_table[j].offset != (tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size)));

											if(j && match_table[j].yes && !match_table[j].no && (match_table[j].offset == (tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size)))
											{
												#ifdef FLUXSTREAMDBG
													floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match with the offset %d. (M:%d)",block_num,tb->blocks[block_num].number_of_pulses,match_table[mt_i-1].offset,nb_of_multimatch);
												#endif

												tb->blocks[block_num].overlap_offset = match_table[j].offset;
												tb->blocks[block_num].overlap_size = tb->blocks[block_num].number_of_pulses;

												previous_block_matched++;

												// Lock ?
											}
											else
											{
												#ifdef FLUXSTREAMDBG
													floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (not aligned with prev block)!",block_num,tb->blocks[block_num].number_of_pulses);
												#endif
											}
										}
										else
										{
											#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (prev block unlocked) !",block_num,tb->blocks[block_num].number_of_pulses);
											#endif
										}
									}
									else
									{
										#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi block match with the Block %d (%d index) (first block!)!",block_num,tb->blocks[block_num].number_of_pulses);
										#endif
									}

								break;

								// -> No offset match at 100% -> take the best one
								case 0:
									tb->blocks[block_num].state = NOMATCH_STATE;
									tb->blocks[block_num].overlap_offset = match_table[mt_i-1].offset;

									// if the offset match with the previous block we can lock it.
									c = 1;

									if(block_num)
									{
										if( tb->blocks[block_num-1].locked && (tb->blocks[block_num].overlap_offset == ( tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size ) ) )
										{

											//tb->blocks[block_num].locked = 1;

											t = detectflakeybits(track_dump,&tb->blocks[block_num],tb->blocks[block_num].overlap_offset,0,0,pl,0xFFFFFFFF,fxs->analysis_rev2rev_max_pulses_jitter);

											tb->blocks[block_num].overlap_size = t - tb->blocks[block_num].overlap_offset;

											#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match not found. Best position : %d - %d ok %d bad. - Match with the previous block !",block_num,tb->blocks[block_num].number_of_pulses,match_table[mt_i-1].offset,match_table[mt_i-1].yes,match_table[mt_i-1].no);
												c=0;
											#endif

										}
									}

									if(c)
									{
										#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Block %d (%d index) full match not found. Best position : %d - %d ok %d bad. - DOESN'T Match with the previous block !",block_num,tb->blocks[block_num].number_of_pulses,match_table[mt_i-1].offset,match_table[mt_i-1].yes,match_table[mt_i-1].no);
											c=0;
										#endif
									/*  if(block_num)
										{
											if( tb->blocks[block_num-1].locked )
												tb->blocks[block_num].overlap_offset = tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size;
										}

										t = detectflakeybits(track_dump,&tb->blocks[block_num],tb->blocks[block_num].overlap_offset,0,0,overlap_pulses_tab,0xFFFFFFFF);
										tb->blocks[block_num].overlap_size = t - tb->blocks[block_num].overlap_offset;*/
									}
								break;

								default:
								break;
							}
						}

						block_num++;
					}

				}while( (block_num<tb->number_of_blocks) && (j < track_dump->channels[0].nb_of_pulses)  && !end_dump_reached);
			}

			/////////////////////////////////////////////////////////
#ifdef FLUXSTREAMDBG
			floppycontext->hxc_printf(MSG_DEBUG,"-------- Fisrt Loop state ---------");

			// print missing blocks.
			for(block_num=0;block_num<tb->number_of_blocks;block_num++)
			{
				c = ' ';

				if(block_num)
				{
					if( (tb->blocks[block_num-1].overlap_offset+tb->blocks[block_num-1].overlap_size) == tb->blocks[block_num].overlap_offset)
					{
						c = ' ';
					}
					else
					{
						c = 'B';
					}
				}

				floppycontext->hxc_printf(MSG_DEBUG,"Block %.4d : %s state |%d| - [%d <-> %d] %c",block_num,getStateStr(tb->blocks[block_num].state),tb->blocks[block_num].locked,tb->blocks[block_num].overlap_offset,tb->blocks[block_num].overlap_offset+tb->blocks[block_num].overlap_size,c);
			}
#endif
			/////////////////////////////////////////////////////////
#ifdef SECONDPASSANALYSIS
			floppycontext->hxc_printf(MSG_DEBUG,"------------------------------");
			floppycontext->hxc_printf(MSG_DEBUG,"--   Second pass analysis ! --");
			floppycontext->hxc_printf(MSG_DEBUG,"------------------------------");

			pass_loop = 0;

			do
			{
				block_analysed = 0;

				for(block_num = 0 ; block_num < tb->number_of_blocks ;block_num++)
				{

					if(!tb->blocks[block_num].locked)
					{
						switch(tb->blocks[block_num].state)
						{

							case MATCH_STATE:
							case ONEMATCH_STATE:
								if(block_num)
								{
									if(tb->blocks[block_num-1].locked)
									{
										if( ( tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size ) == tb->blocks[block_num].overlap_offset )
										{
											tb->blocks[block_num].locked = 1;
											block_analysed++;
										}
									}
								}

								if(block_num < tb->number_of_blocks - 1)
								{
									if(tb->blocks[block_num+1].locked)
									{
										if( ( tb->blocks[block_num].overlap_offset + tb->blocks[block_num].overlap_size ) == tb->blocks[block_num + 1].overlap_offset )
										{
											tb->blocks[block_num].locked = 1;
											block_analysed++;
										}
									}
								}

							break;

							case MULTIMATCH_STATE:
								c = 1;
								if(block_num)
								{
									if(tb->blocks[block_num-1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
											c = 0;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											tb->blocks[block_num].locked = 0;
										}
									}
								}

								if(block_num < tb->number_of_blocks - 1 && c)
								{
									if(tb->blocks[block_num+1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].overlap_size ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].overlap_size;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											tb->blocks[block_num].locked = 0;
										}
									}
								}
							break;

							case NOMATCH_STATE:

								if(block_num)
								{
									if(tb->blocks[block_num-1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].overlap_size;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
											c = 0;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"NOMATCH Block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											t = detectflakeybits(track_dump,&tb->blocks[block_num],tb->blocks[block_num].overlap_offset,0,0,pl,0xFFFFFFFF,fxs->analysis_rev2rev_max_pulses_jitter);

											if(tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses == tb->blocks[block_num].overlap_offset)
											{
												tb->blocks[block_num].locked = 1;
												//block_analysed++;
	#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : pre aligned with the previous block",block_num,tb->blocks[block_num].number_of_pulses);
	#endif
											}
											else
											{
												tb->blocks[block_num].locked = 0;
	#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : not pre aligned with the previous block! : %d != %d",block_num,tb->blocks[block_num].number_of_pulses,tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses,tb->blocks[block_num].overlap_offset);
	#endif
											}
										}
									}
								}

								if(block_num < tb->number_of_blocks-1)
								{
									if(tb->blocks[block_num+1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].overlap_size ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].overlap_size;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"NOMATCH_STATE block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{

											t = detectflakeybits(track_dump,&tb->blocks[block_num],tb->blocks[block_num].overlap_offset,0,0,pl,0xFFFFFFFF,fxs->analysis_rev2rev_max_pulses_jitter);

											if(tb->blocks[block_num+1].overlap_offset == t)
											{
	#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : post aligned with the next block - %d",block_num,tb->blocks[block_num].number_of_pulses,t);
	#endif
												//block_analysed++;

												tb->blocks[block_num].locked = 1;
											}
											else
											{
	#ifdef FLUXSTREAMDBG
												floppycontext->hxc_printf(MSG_DEBUG,"Flakey Block %d (%d index) analysis : not post aligned with the next block ! %d != %d   - %d",block_num,tb->blocks[block_num].number_of_pulses,(tb->blocks[block_num].overlap_offset + tb->blocks[block_num].number_of_pulses),tb->blocks[block_num+1].overlap_offset,t);
	#endif
												tb->blocks[block_num].locked = 0;
											}

										}
									}
								}
							break;

							default:
							break;
						}
					}
				}

				pass_loop++;

			}while(block_analysed && ( pass_loop < ( tb->number_of_blocks * 2 ) ));

			if(pass_loop >= ( tb->number_of_blocks * 2 ) )
			{
				floppycontext->hxc_printf(MSG_DEBUG,"Infinite loop... Second pass analysis aborted!");
			}

			floppycontext->hxc_printf(MSG_DEBUG,"Second pass done in %d loop(s)",pass_loop);
#endif


#ifdef SECONDPASSANALYSIS
			floppycontext->hxc_printf(MSG_DEBUG,"------------------------------");
			floppycontext->hxc_printf(MSG_DEBUG,"--   Third pass analysis ! --");
			floppycontext->hxc_printf(MSG_DEBUG,"------------------------------");

			pass_loop = 0;

			do
			{
				block_analysed = 0;

				for(block_num = 0 ; block_num < tb->number_of_blocks ;block_num++)
				{

					if(!tb->blocks[block_num].locked)
					{
						switch(tb->blocks[block_num].state)
						{
							case MULTIMATCH_STATE:
								c = 1;
								if(block_num)
								{
									if(tb->blocks[block_num-1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num-1].overlap_offset + tb->blocks[block_num-1].number_of_pulses;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
											c = 0;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											tb->blocks[block_num].locked = 0;
										}
									}
								}

								if(block_num < tb->number_of_blocks-1 && c)
								{
									if(tb->blocks[block_num+1].locked)
									{
										compareblock(track_dump,&tb->blocks[block_num], tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].number_of_pulses ,&good,&bad,0,fxs->analysis_rev2rev_max_pulses_jitter);
										if(!bad && good)
										{
											tb->blocks[block_num].overlap_offset = tb->blocks[block_num+1].overlap_offset - tb->blocks[block_num].number_of_pulses;
											tb->blocks[block_num].locked = 1;
											block_analysed++;
#ifdef FLUXSTREAMDBG
											floppycontext->hxc_printf(MSG_DEBUG,"Multi match block %d position corrected with the next block alignement",block_num);
#endif
										}
										else
										{
											tb->blocks[block_num].locked = 0;
										}
									}
								}
							break;

							default:
							break;
						}
					}
				}

				pass_loop++;

			}while(block_analysed && ( pass_loop < ( tb->number_of_blocks * 4 ) ));

			if(pass_loop >= ( tb->number_of_blocks * 4 ) )
			{
				floppycontext->hxc_printf(MSG_DEBUG,"Infinite loop... Third pass analysis aborted!");
			}

			floppycontext->hxc_printf(MSG_DEBUG,"Third pass done in %d loop(s)",pass_loop);

#endif

			memset(pl->forward_link,0xFF,pl->number_of_pulses * sizeof(uint32_t) );

			for(block_num=0;block_num<tb->number_of_blocks;block_num++)
			{
				if(tb->blocks[block_num].locked)
				{
					detectflakeybits(track_dump,&tb->blocks[block_num],tb->blocks[block_num].overlap_offset,0,0,pl,0xFFFFFFFF,fxs->analysis_rev2rev_max_pulses_jitter);
				}
			}

			// Scan unlocked blocks...
			shift = 0;
			for(block_num=0;block_num<tb->number_of_blocks;block_num++)
			{
				if(!tb->blocks[block_num].locked)
				{

					next_locked_block =     getNearestMatchedBlock( tb->blocks, 1, block_num,tb->number_of_blocks);
					previous_locked_block = getNearestMatchedBlock( tb->blocks, 0, block_num,tb->number_of_blocks);

					if( ( next_locked_block >= 0 ) && ( previous_locked_block >= 0 ) )
					{

#ifdef FLUXSTREAMDBG
						floppycontext->hxc_printf(MSG_DEBUG,"scan unlocked flakey bits block %d-%d-%d...",previous_locked_block,block_num,next_locked_block);
#endif
						compare_block_timebased(floppycontext,track_dump,fxs,&tb->blocks[previous_locked_block],&tb->blocks[block_num],&tb->blocks[next_locked_block], pl,&shift);

						for( k = previous_locked_block ; k < next_locked_block ; k++)
						{
							tb->blocks[block_num].locked = 1;
						}

						block_num = next_locked_block;
					}
				}
			}


#ifdef FLUXSTREAMDBG
			// print missing blocks.
			for(block_num=0;block_num<tb->number_of_blocks;block_num++)
			{
				c = ' ';
				if(block_num)
				{
					if( (tb->blocks[block_num-1].overlap_offset+tb->blocks[block_num-1].overlap_size) == tb->blocks[block_num].overlap_offset)
					{
						c = ' ';
					}
					else
					{
						c = 'B';
					}
				}

				j = 0;
				for(i=tb->blocks[block_num].start_index;i<tb->blocks[block_num].start_index+tb->blocks[block_num].number_of_pulses;i++)
				{
					if(pl->forward_link[i]==-1) j++;
				}

				floppycontext->hxc_printf(MSG_DEBUG,"Block %.4d : %s state |%d| - [%d <-> %d] %c s:%d l:%d size:%d bad bit:%d",block_num,getStateStr(tb->blocks[block_num].state),tb->blocks[block_num].locked,tb->blocks[block_num].overlap_offset,tb->blocks[block_num].overlap_offset+tb->blocks[block_num].overlap_size,c,tb->blocks[block_num].start_index,tb->blocks[block_num].number_of_pulses,tb->blocks[block_num].start_index+tb->blocks[block_num].number_of_pulses,j);
			}
#endif

			for(block_num=0;block_num<tb->number_of_blocks;block_num++)
			{
				if(tb->blocks[block_num].locked)
				{
					for(i=tb->blocks[block_num].start_index;i<tb->blocks[block_num].number_of_pulses;i++)
					{
						if((i<(int32_t)track_dump->channels[0].nb_of_pulses) && pl->forward_link[i]>=0)
						{
							if(pl->forward_link[i]<(int32_t)track_dump->channels[0].nb_of_pulses)
							{
								if(!pl->forward_link[pl->forward_link[i]])
								{
									pl->forward_link[pl->forward_link[i]] = pl->forward_link[i] + (pl->forward_link[i] - i);
								}
							}
						}
					}
				}
			}

			block_num = 0;
			while(block_num<tb->number_of_blocks && !tb->blocks[block_num].locked)
			{
				switch(tb->blocks[block_num].state)
				{
					case MULTIMATCH_STATE:

						for(i=tb->blocks[block_num].start_index;i<tb->blocks[block_num].number_of_pulses;i++)
						{
							if((i<(int32_t)track_dump->channels[0].nb_of_pulses) && pl->forward_link[i]<0)
							{
								pl->forward_link[i] = 1 + i;
							}
						}

					break;
				}

				block_num++;
			}

			for(i=0;i<pl->number_of_pulses;i++)
			{
				if( ( pl->forward_link[i] >= 0 ) && ( pl->forward_link[i] < pl->number_of_pulses ) )
				{
					pl->backward_link[ pl->forward_link[i] ] = i;
				}
			}

			/*for(i=0;i<track_dump->channels[0].nb_of_pulses;i++)
			{
				//if(track_dump->channels[0].stream[i] > 0x100)
				{
					floppycontext->hxc_printf(MSG_DEBUG,"offset:%d , val :%d",i,track_dump->channels[0].stream[i]);
				}
			}*/

			free(match_table);
		}
	}

	return pl;
}

static uint32_t getbestindex(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM *track_dump,pulses_link * pl,int bestindex,uint32_t * score,uint32_t nb_revolution)
{
	int32_t i,j;
	int32_t first_index,last_index,bad_pulses;

	uint32_t bad_pulses_array[MAX_NB_OF_INDEX],min_val;
	uint32_t bestval;

	uint32_t bestscore,revnb;

	bestval = 0;

	if(fxs && track_dump)
	{
		bestscore = score[bestindex];

		memset(bad_pulses_array,0xFF,sizeof(uint32_t) * MAX_NB_OF_INDEX);

		for(revnb = 0; revnb < nb_revolution ; revnb++)
		{
			if ( score[revnb] == bestscore )
			{
				first_index = getNearestValidIndex(pl,track_dump->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, track_dump, revnb )].dump_offset,pl->number_of_pulses);

				bad_pulses = 0;
				last_index = pl->forward_link[first_index];

				if(last_index>=0)
				{
					for(j=first_index;j<last_index;j++)
					{
						if( pl->forward_link[j] < 0 && pl->backward_link[j] < 0 )
							bad_pulses++;
					}
				}
				bad_pulses_array[revnb] = bad_pulses;
			}
		}

	#ifdef FLUXSTREAMDBG
		for(revnb = 0; revnb < nb_revolution ; revnb++)
		{
			fxs->hxcfe->hxc_printf(MSG_DEBUG,"getbestindex : Index %d, %d bad pulse(s)",revnb,bad_pulses_array[revnb]);
		}
	#endif

		min_val = bad_pulses_array[0];
		for(i=0;i<(int32_t)nb_revolution;i++)
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

void AdjustTrackPeriod(HXCFE* floppycontext,HXCFE_SIDE * curside_S0,HXCFE_SIDE * curside_S1)
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
		curside_S1->timingbuffer[i] = (uint32_t)((double)curside_S1->timingbuffer[i] * (period_s1/period_s0));
	}
}

int set_pll_cfg(HXCFE * hxcfe, pll_stat *pll, int * cfg_table, int current_track, HXCFE_TRKSTREAM * std)
{
	int track_index,i,j;
	uint32_t * histo;
	int val,total;

	track_index = 0;

	for(i=0;i<16;i++)
	{
		pll->bands_separators[i] = -1;
	}

	if( cfg_table[track_index*8] == -1 )
		return 0;

	while( (cfg_table[track_index*8] != -1) && (cfg_table[track_index*8] <= current_track) )
	{
		track_index++;
	}

	if(track_index)
		track_index--;

	histo = (uint32_t*)malloc(65536* sizeof(uint32_t));
	if(!histo)
		goto error;

	if(std)
	{
		computehistogram(&std->channels[0].stream[0],std->channels[0].nb_of_pulses,histo);
	}

	i = 0;
	while( cfg_table[(track_index*8) + 1 + i] )
	{
		val = ( (double)cfg_table[(track_index*8) + 1 + i + 1]  / (double)( (double)(((double)(1000*1000*1000)/TICKFREQ)) ) );

		if(!i)
		{
			if(std)
			{
				if(val < 65536)
				{
					#define FILTERLEN 10
					do
					{
						total = 0;
						for(j=0;j<FILTERLEN;j++)
						{
							total += histo[val + j];

							if(total > 10)
							{
								break;
							}
						}

						if(j<FILTERLEN)
							val += FILTERLEN/2;

					}while(j<FILTERLEN);
				}
			}
		}

		pll->bands_separators[ cfg_table[ (track_index*8) + 1 + i] ] = val;

#ifdef FLUXSTREAMDBG
		hxcfe->hxc_printf(MSG_DEBUG,">>>>>> %d %d: %d-%d <<<<<<<\n",current_track, track_index , cfg_table[(track_index*8) + 1 + i] ,pll->bands_separators[ cfg_table[ (track_index*8) + 1 + i] ]);
#endif

		i = i + 2;
	}

	pll->bands_separators[15] = ( (double)20000  / (double)( (double)(((double)(1000*1000*1000)/TICKFREQ)) ) );

	free(histo);

	return 1;
error:
	free(histo);

	return 0;
}

HXCFE_FXSA * hxcfe_initFxStream(HXCFE * hxcfe)
{
	HXCFE_FXSA * fxs;
	int v;

	if(hxcfe)
	{
		fxs = malloc(sizeof(HXCFE_FXSA));
		if(fxs)
		{
			memset(fxs,0,sizeof(HXCFE_FXSA));

			// Default low pass filter setting
			fxs->filterpasses = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" );
			fxs->filter = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" );

			fxs->hxcfe = hxcfe;
			fxs->phasecorrection = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" );

			fxs->analysis_window_size = DEFAULT_BLOCK_TIME;

			fxs->pll.tick_freq = TICKFREQ;
			fxs->pll.fast_correction_ratio_n = 15;
			fxs->pll.fast_correction_ratio_d = 16;
			fxs->pll.slow_correction_ratio_n = 31;
			fxs->pll.slow_correction_ratio_d = 32;
			fxs->pll.inter_band_rejection = 0;
			fxs->pll.pll_min_max_percent = 18;
			fxs->pll.max_pll_error_ticks = (float)((float)fxs->pll.tick_freq * (float)1E-9) * (float)680;
			fxs->analysis_rev2rev_max_pulses_jitter = DEFAULT_MAXPULSESKEW;
			fxs->out_index_length_us = 2000;
			fxs->weak_cell_threshold = -1;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_FAST_CORRECTION_RATIO_N" );
			if(v)
				fxs->pll.fast_correction_ratio_n = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_FAST_CORRECTION_RATIO_D" );
			if(v)
				fxs->pll.fast_correction_ratio_d = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_SLOW_CORRECTION_RATIO_N" );
			if(v)
				fxs->pll.slow_correction_ratio_n = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_SLOW_CORRECTION_RATIO_D" );
			if(v)
				fxs->pll.slow_correction_ratio_d = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_TICK_FREQUENCY" );
			if(v)
				fxs->pll.tick_freq = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_GCR_INTER_BANDS_REJECTION" );
			if(v)
				fxs->pll.inter_band_rejection = 1;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_FM_INTER_BANDS_REJECTION" );
			if(v)
				fxs->pll.inter_band_rejection = 2;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_MIN_MAX_PERCENT" );
			if(v)
				fxs->pll.pll_min_max_percent = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_MAX_ERROR_NS" );
			if(v)
				fxs->pll.max_pll_error_ticks = (float)((float)fxs->pll.tick_freq * (float)1E-9) * (float)v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_ANALYSIS_WINDOW_SIZE_US" );
			if(v)
				fxs->analysis_window_size = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_ANALYSIS_REV2REV_MAX_PULSES_JITTER" );
			if(v)
				fxs->analysis_rev2rev_max_pulses_jitter = ((float)256/(float)100) * (float)v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_ANALYSIS_USE_VICTOR_TIMING" );
			if(v)
			{
				if(set_pll_cfg(hxcfe,&fxs->pll,(int*)&victor_9k_bands_def, 0,NULL))
				{
					fxs->pll.band_mode = 1;
				}
			}

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_ANALYSIS_OUT_INDEX_LENGTH" );
			if(v > 0)
				fxs->out_index_length_us = v;

			v = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_PLL_WEAKBITS_CELLCNT_THRESHOLD" );
			if(v)
				fxs->weak_cell_threshold = v;

			fxs->sector_recovery = hxcfe_getEnvVarValue( hxcfe, "FLUXSTREAM_SECTORS_RECOVERY" );

			return fxs;
		}
	}

	return 0;
}

void hxcfe_FxStream_setResolution( HXCFE_FXSA * fxs, int32_t step )
{
	if(fxs)
	{
		fxs->steptime = step;
	}
}

void hxcfe_FxStream_setBitrate( HXCFE_FXSA * fxs, int32_t bitrate )
{
	if(fxs)
	{
		fxs->defaultbitrate = bitrate;
	}
}

void hxcfe_FxStream_setPhaseCorrectionFactor( HXCFE_FXSA * fxs, int32_t phasefactor )
{
	if(fxs)
	{
		fxs->phasecorrection = phasefactor;
	}
}

void hxcfe_FxStream_setFilterParameters( HXCFE_FXSA * fxs, int32_t number_of_passes, int32_t step )
{
	if(fxs)
	{
		fxs->filterpasses = number_of_passes;
		fxs->filter = step;
	}
}

HXCFE_TRKSTREAM * hxcfe_FxStream_ImportStream( HXCFE_FXSA * fxs, void * stream, int32_t wordsize, uint32_t nbword, uint32_t type, char * name, HXCFE_TRKSTREAM * trk_stream)
{
	uint32_t total_tick;
	uint32_t total_tick_transposed;
	uint32_t total_tick_computed;
	unsigned int i,channel;

	if(!fxs)
		return NULL;

#ifdef FLUXSTREAMDBG
	fxs->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_FxStream_ImportStream : in buffer : %p, wordsize : %d, number of words : %d",stream,wordsize,nbword);
#endif

	if(!stream)
		return NULL;

	if(!trk_stream)
	{
		trk_stream = calloc( 1, sizeof(HXCFE_TRKSTREAM));
		if(!trk_stream)
			return NULL;
	}

	trk_stream->tick_freq = fxs->pll.tick_freq;

	channel = 0;
	while(channel<MAX_NB_OF_STREAMCHANNEL && trk_stream->channels[channel].stream)
	{
		channel++;
	}

	if(trk_stream && (channel<MAX_NB_OF_STREAMCHANNEL) )
	{
		trk_stream->channels[channel].stream_name[64-1] = 0;
		strncpy(trk_stream->channels[channel].stream_name,name,64 - 1);
		trk_stream->channels[channel].type = type;

		trk_stream->channels[channel].stream = malloc( nbword * sizeof(uint32_t) );
		if( trk_stream->channels[channel].stream )
		{
			total_tick = 0;
			total_tick_transposed = 0;
			memset( trk_stream->channels[channel].stream, 0, nbword * sizeof(uint32_t) );
			for( i = 0 ; i < nbword ; i++ )
			{
				switch(wordsize)
				{
					case 8:
						trk_stream->channels[channel].stream[i] = *(((uint8_t*)stream) + i);
					break;
					case 16:
						trk_stream->channels[channel].stream[i] = *(((uint16_t*)stream) + i);
					break;
					case 32:
						trk_stream->channels[channel].stream[i] = *(((uint32_t*)stream) + i);
					break;
					default:
					break;
				}

				total_tick += trk_stream->channels[channel].stream[i];

				trk_stream->channels[channel].stream[i] = (uint32_t)((float)trk_stream->channels[channel].stream[i] * (float)((float)fxs->steptime/(float)(1E12 / fxs->pll.tick_freq)));

				total_tick_transposed += trk_stream->channels[channel].stream[i];

				total_tick_computed = (uint32_t)((float)total_tick * (float)((float)fxs->steptime/(float)(1E12 / fxs->pll.tick_freq)));

				// Possible cumulative error check.
				if( total_tick_transposed != total_tick_computed )
				{
					// Fix it.
					if( total_tick_transposed < total_tick_computed )
					{
						total_tick_transposed++;
						trk_stream->channels[channel].stream[i]++;
					}
					else
					{
						if( trk_stream->channels[channel].stream[i] > 1 )
						{
							total_tick_transposed--;
							trk_stream->channels[channel].stream[i]--;
						}
					}
				}
			}

			trk_stream->channels[channel].nb_of_pulses = nbword;
		}
		else
		{
			free(trk_stream);
			return NULL;
		}
/*
		if(trk_stream)
		{
			hxcfe_FxStream_JitterFilter(fxs,trk_stream);
		}
*/
		return trk_stream;
	}

	return NULL;
}

void hxcfe_FxStream_AddIndex( HXCFE_FXSA * fxs, HXCFE_TRKSTREAM * std, uint32_t streamposition, int32_t tickoffset, uint32_t flags )
{
	uint32_t cellpos,i;

	if(fxs)
	{
		if(std)
		{
			if((streamposition <= std->channels[0].nb_of_pulses) && (std->channels[0].nb_of_pulses > 0) )
			{
#ifdef FLUXSTREAMDBG
				fxs->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_FxStream_AddIndex : streamposition %d (%d us) - tickoffset %d - flags : 0x%.8X",streamposition,tick_to_time(fxs,GetTickCnt(std,0, streamposition)),tickoffset,flags);
#endif

				if( (streamposition == std->channels[0].nb_of_pulses) && std->channels[0].nb_of_pulses)
				{
					fxs->hxcfe->hxc_printf(MSG_WARNING,"hxcfe_FxStream_AddIndex : FIXME ! -> streamposition beyond of stream limit by ONE sample ! (%d >= %d). Moving it inside the stream...",streamposition,std->channels[0].nb_of_pulses);

					streamposition--;
				}

				if(std->nb_of_index<MAX_NB_OF_INDEX)
				{
					std->index_evt_tab[std->nb_of_index].flags = flags;
					std->index_evt_tab[std->nb_of_index].dump_offset = streamposition;

					cellpos = 0;

					for(i = 0;i < streamposition;i++)
					{
						cellpos = cellpos + std->channels[0].stream[i];
					}

					std->index_evt_tab[std->nb_of_index].cellpos = cellpos;
					std->index_evt_tab[std->nb_of_index].tick_offset = (int)( tickoffset * (float)((float)fxs->steptime/(float)(1E12 / fxs->pll.tick_freq)));

					std->nb_of_index++;
				}
			}
			else
			{
				fxs->hxcfe->hxc_printf(MSG_ERROR,"hxcfe_FxStream_AddIndex : streamposition beyond of stream limit ! (%d > %d)",streamposition,std->channels[0].nb_of_pulses);
			}
		}
	}
}

void hxcfe_FxStream_ChangeSpeed( HXCFE_FXSA * fxs, HXCFE_TRKSTREAM * std, float speedchange )
{
	uint32_t cellpos,streamposition,i,j;
	if(fxs)
	{
		if(std)
		{
			if(speedchange && (speedchange != 1.0))
			{
				for(i=0;i<std->channels[0].nb_of_pulses;i++)
				{
					std->channels[0].stream[i] = (uint32_t)(((float)std->channels[0].stream[i] ) * speedchange);
				}

				for(i=0;i<std->nb_of_index;i++)
				{
					streamposition = std->index_evt_tab[i].dump_offset;

					cellpos = 0;

					for(j=0;j < streamposition;j++)
					{
						cellpos = cellpos + std->channels[0].stream[j];
					}

					std->index_evt_tab[i].cellpos = cellpos;
					std->index_evt_tab[i].tick_offset = (int)(((float)std->index_evt_tab[i].tick_offset ) * speedchange);
				}
			}
		}
	}
}

int32_t hxcfe_FxStream_GetNumberOfRevolution( HXCFE_FXSA * fxs, HXCFE_TRKSTREAM * std )
{
	int count_rev;
	unsigned int i;

	if(fxs)
	{
		if(std)
		{
			if(std->nb_of_index)
			{
				count_rev = 0;

				for(i=0;i<std->nb_of_index;i++)
				{
					if( std->index_evt_tab[i].flags & FXSTRM_INDEX_MAININDEX )
					{
						count_rev++;
					}
				}

#ifdef FLUXSTREAMDBG
				fxs->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_FxStream_GetNumberOfRevolution:  %d",count_rev - 1);
#endif

				return count_rev - 1;
			}

			return 0;
		}
	}

	return 0;
}

uint32_t hxcfe_FxStream_GetRevolutionIndex( HXCFE_FXSA * fxs, HXCFE_TRKSTREAM * std, int32_t revolution )
{
	int count_rev;
	unsigned int i;

	if(fxs)
	{
		if(std)
		{
			if(std->nb_of_index)
			{
				count_rev = 0;

				for(i=0;i<std->nb_of_index;i++)
				{
					if( std->index_evt_tab[i].flags & FXSTRM_INDEX_MAININDEX )
					{
						if( count_rev == revolution )
						{
							return i;
						}

						count_rev++;
					}
				}

				return 0;
			}

			return 0;
		}
	}

	return 0;
}

uint32_t hxcfe_FxStream_GetRevolutionPeriod( HXCFE_FXSA * fxs, HXCFE_TRKSTREAM * std, int32_t revolution )
{
	uint32_t period;

	if(fxs)
	{
		if(std)
		{
			if(revolution < hxcfe_FxStream_GetNumberOfRevolution(fxs,std))
			{

				period = std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution+1 )].cellpos - \
						std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution )].cellpos;

				return period;
			}

			return 0;
		}
	}

	return 0;
}

uint32_t hxcfe_FxStream_GetMeanRevolutionPeriod(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std)
{
	uint32_t nb_rotation;

	uint32_t i;
	uint32_t currentperiod,globalperiod;

	globalperiod = 0;

	if(fxs)
	{
		if(std)
		{
			nb_rotation = hxcfe_FxStream_GetNumberOfRevolution(fxs,std);

			if( nb_rotation )
			{
				globalperiod = 0;
				for(i=0;i<(nb_rotation);i++)
				{
					currentperiod = hxcfe_FxStream_GetRevolutionPeriod(fxs,std,i);
					globalperiod += currentperiod;
				}

				globalperiod = (uint32_t)((double)((double)globalperiod / ((double)nb_rotation)));
			}
			else
			{
				globalperiod = 0;
			}
		}
	}

	return tick_to_time(fxs,globalperiod);
}

HXCFE_FLOPPY * makefloppyfromtrack(HXCFE_SIDE * side)
{
	HXCFE_FLOPPY * newfloppy;

	newfloppy = calloc( 1, sizeof(HXCFE_FLOPPY) );
	if(newfloppy)
	{
		newfloppy->floppyBitRate = 250000;
		newfloppy->floppyNumberOfSide = 1;
		newfloppy->floppyNumberOfTrack = 1;
		newfloppy->floppySectorPerTrack = -1;

		newfloppy->tracks=(HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*newfloppy->floppyNumberOfTrack );
		if(!newfloppy->tracks)
		{
			free(newfloppy);
			return NULL;
		}

		newfloppy->tracks[0] = allocCylinderEntry(0,newfloppy->floppyNumberOfSide);
		if(!newfloppy->tracks[0])
		{
			free(newfloppy->tracks);
			free(newfloppy);
			return NULL;
		}

		newfloppy->tracks[0]->sides[0] = side;
	}

	return newfloppy;
}

void freefloppy(HXCFE_FLOPPY * fp)
{
	if(fp)
	{
		if(fp->tracks)
			free(fp->tracks[0]->sides);

		free(fp->tracks);

		free(fp);
	}
}

uint32_t getbestrevolution(uint32_t * score,uint32_t nb_revolution)
{
	uint32_t maxval,lastindex,i;

	lastindex = 0;
	maxval = score[0];

	i = 0xFFFFFFFF;

	for(i=0;i<nb_revolution;i++)
	{
		if(maxval < score[i])
		{
			maxval = score[i];
			lastindex = i;
		}
	}


	return lastindex;

}

int tracktypelist[]=
{
	ISOIBM_MFM_ENCODING,
	ISOIBM_FM_ENCODING,
	AMIGA_MFM_ENCODING,
	EMU_FM_ENCODING,
	ARBURGSYS_ENCODING,
	ARBURGDAT_ENCODING,
	NORTHSTAR_HS_MFM_ENCODING,
	HEATHKIT_HS_FM_ENCODING,
	MICRALN_HS_FM_ENCODING,
	APPLEII_GCR1_ENCODING,
	APPLEII_GCR2_ENCODING,
	APPLEMAC_GCR_ENCODING,
	UNKNOWN_ENCODING
};

HXCFE_TRKSTREAM * duplicate_track_stream(HXCFE_TRKSTREAM * trks)
{
	HXCFE_TRKSTREAM * new_trks;

	new_trks = 0;

	if(trks)
	{
		new_trks = malloc(sizeof(HXCFE_TRKSTREAM));
		if(new_trks)
		{
			memcpy(new_trks,trks,sizeof(HXCFE_TRKSTREAM));
			if(trks->channels[0].nb_of_pulses && trks->channels[0].stream)
			{
				new_trks->channels[0].stream = malloc(trks->channels[0].nb_of_pulses * sizeof(uint32_t));
				if(new_trks->channels[0].stream)
				{
					memcpy(new_trks->channels[0].stream,trks->channels[0].stream,trks->channels[0].nb_of_pulses * sizeof(uint32_t));
				}
			}
		}
	}

	return new_trks;
}

HXCFE_TRKSTREAM * reverse_track_stream(HXCFE_TRKSTREAM * trks)
{
	uint32_t i;
	uint32_t * reversed_track;

	if(trks)
	{
		if(trks->channels[0].stream && trks->channels[0].nb_of_pulses)
		{
			reversed_track = malloc(trks->channels[0].nb_of_pulses * sizeof(uint32_t));
			if(reversed_track)
			{
				for(i=0;i<trks->channels[0].nb_of_pulses;i++)
				{
					reversed_track[i] = trks->channels[0].stream[(trks->channels[0].nb_of_pulses - 1) - i];
				}

				free(trks->channels[0].stream);

				trks->channels[0].stream = reversed_track;
			}
		}
	}

	return trks;
}

void printsidestat(HXCFE_FXSA * fxs,HXCFE_SIDE * side)
{
#ifdef FLUXSTREAMDBG
	int nbbadbit;
	int nbbit;
	uint32_t i;

	fxs->hxcfe->hxc_printf(MSG_DEBUG,"Side stats: ");
	fxs->hxcfe->hxc_printf(MSG_DEBUG,"Bitrate: %d",side->bitrate);
	fxs->hxcfe->hxc_printf(MSG_DEBUG,"Number of sector: %d",side->number_of_sector);
	fxs->hxcfe->hxc_printf(MSG_DEBUG,"Track encoding: %d",side->track_encoding);
	fxs->hxcfe->hxc_printf(MSG_DEBUG,"Track lenght: %d cells",side->tracklen);

	nbbadbit = 0;

	if(side->flakybitsbuffer)
	{
		for(i=0;i<side->tracklen;i++)
		{
			if( side->flakybitsbuffer[i>>3] & (0x80>>(i&7)) )
			{
				nbbadbit++;
			}
		}

		fxs->hxcfe->hxc_printf(MSG_DEBUG,"%d flakey cells",nbbadbit);
	}

	nbbit = 0;
	if(side->databuffer)
	{
		for(i=0;i<side->tracklen;i++)
		{
			if( side->databuffer[i>>3] & (0x80>>(i&7)) )
			{
				nbbit++;
			}
		}

		fxs->hxcfe->hxc_printf(MSG_DEBUG,"%d bits",nbbit);
	}
#endif
}

void hxcfe_FxStream_SetIndexLength( HXCFE_FXSA * fxs, int us )
{
	int v;

	if(fxs)
	{
		v = hxcfe_getEnvVarValue( fxs->hxcfe, "FLUXSTREAM_ANALYSIS_OUT_INDEX_LENGTH" );
		if(v > 0)
		{
			fxs->out_index_length_us = v;
		}
		else
		{
			fxs->out_index_length_us = us;
		}
	}
}

HXCFE_SIDE * hxcfe_FxStream_AnalyzeAndGetTrack(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std)
{
	HXCFE * hxcfe;
	int bitrate;
	uint32_t totallen,indexperiod;
	uint32_t * histo;
	HXCFE_SIDE * currentside, * tmp_side;
	HXCFE_SIDE * revolutionside[MAX_NB_OF_INDEX];

	pulses_link * pl;
	pulses_link * pl_reversed;

	int32_t first_index,next_index;
	uint32_t track_len;
	uint32_t nb_of_revolutions,revolution;
	int32_t i;

	uint32_t qualitylevel[MAX_NB_OF_INDEX];

	int32_t first_track_encoding;

	track_blocks * tb;
	track_blocks * tb_reversed;

	int16_t rpm;
	int32_t nb_sectorfound,sectnum;

	HXCFE_SECTORACCESS* ss;
	HXCFE_FLOPPY *fp;
	HXCFE_SECTCFG** scl;
	HXCFE_TRKSTREAM * reversed_std;

	int32_t * backward_link;
	int32_t * forward_link;
	int number_of_pulses;
#ifdef FLUXSTREAMDBG
	int patchedbits;
#endif
	currentside = NULL;
	track_len = 0;
	rpm = 0;
	bitrate = 0;

	hxcfe = fxs->hxcfe;

	if(hxcfe_FxStream_GetNumberOfRevolution(fxs,std) >= 1)
	{
		if(fxs->pll.band_mode)
		{
			set_pll_cfg(hxcfe,&fxs->pll,(int*)&victor_9k_bands_def, fxs->pll.track, std);
		}

		// Get the total track dump time length. (in 10th of nano seconds)
		totallen = GetDumpTimelength(fxs,std);

		// Allocate the blocks.
		tb = AllocateBlocks(fxs, std, fxs->analysis_window_size);
		if(!tb)
		{
			return NULL;
		}

		hxcfe->hxc_printf(MSG_DEBUG,"Track dump length : %d us, number of block : %d, Number of pulses : %d",totallen/100,tb->number_of_blocks,std->channels[0].nb_of_pulses);

		// Get the index period.
		indexperiod = hxcfe_FxStream_GetMeanRevolutionPeriod(fxs,std);

		//indexperiod = 166 *1000*100;

		hxcfe->hxc_printf(MSG_DEBUG,"Index period : %d ms",indexperiod / (1000*100) );

		if( ( (indexperiod / (1000*100)) < 80 ) || ( (indexperiod / (1000*100)) >= 400 ) )
		{
			fxs->hxcfe->hxc_printf(MSG_WARNING,"Non-conventionnal index period ! (%d ms)",(indexperiod / (1000*100)));
		}

		if(indexperiod > 0)
		{

			hxcfe->hxc_printf(MSG_DEBUG,"Block analysing...");

			// Find the blocks overlap.
			pl = ScanAndFindRepeatedBlocks(hxcfe,fxs,std,indexperiod,tb);

			hxcfe->hxc_printf(MSG_DEBUG,"...done");
			if(pl)
			{
				///////////////////////////////////////////////////////////////////////////

				reversed_std = duplicate_track_stream(std);
				if(reversed_std)
				{
					reverse_track_stream(reversed_std);

					// Allocate the blocks.
					tb_reversed = AllocateBlocks(fxs, reversed_std, fxs->analysis_window_size);

					pl_reversed = ScanAndFindRepeatedBlocks(hxcfe,fxs,reversed_std,indexperiod,tb_reversed);

					number_of_pulses = pl->number_of_pulses;
					backward_link = pl->backward_link;
					forward_link = &pl_reversed->forward_link[number_of_pulses - 1];
					for(i=0;i<number_of_pulses;i++)
					{
						*(backward_link++) = *(forward_link--);
					}

					backward_link = pl->backward_link;
					forward_link = pl->forward_link;
					number_of_pulses = pl->number_of_pulses;

					for(i=0;i<number_of_pulses;i++)
					{
						if(*backward_link>=0)
						{
							*backward_link = (number_of_pulses - 1) - *backward_link;
						}

						backward_link++;
					}

					number_of_pulses = pl->number_of_pulses;
					backward_link = pl->backward_link;
					forward_link = pl->forward_link;

					for(i=0;i<number_of_pulses;i++)
					{
						if(*backward_link>=0)
						{
							if(forward_link[*backward_link] < 0)
							{
								forward_link[*backward_link] = i;
							}
						}

						backward_link++;
					}

					free_pulses_link_array(pl_reversed);

					hxcfe_FxStream_FreeStream(fxs,reversed_std);

					free(tb_reversed->blocks);
					free(tb_reversed);
				}

				///////////////////////////////////////////////////////////////////////////

#ifdef FLUXSTREAMDBG
				fxs->hxcfe->hxc_printf(MSG_DEBUG,"Revolutions checking...");
#endif

				memset(revolutionside,0,sizeof(revolutionside));

				nb_of_revolutions = hxcfe_FxStream_GetNumberOfRevolution( fxs, std );

				for(revolution = 0; revolution < nb_of_revolutions; revolution++)
				{
					first_index = getNearestValidIndex(pl,std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution )].dump_offset,pl->number_of_pulses);

#ifdef FLUXSTREAMDBG
					fxs->hxcfe->hxc_printf(MSG_INFO_1,"Revolution %d track generation... First valid index position : %d",revolution,first_index);
#endif

					track_len = 0;
					i = first_index;

					if( i < (int32_t)std->channels[0].nb_of_pulses)
					{
						if( pl->forward_link[first_index] == (first_index+1) )
						{   // Dummy buffer case
							int next_index,offset;
							next_index = getNearestValidIndex(pl,std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution ) + 1].dump_offset,pl->number_of_pulses);

							offset = i;
							while(offset < next_index)
							{
								track_len += std->channels[0].stream[offset++];
							}
						}
						else
						{
							do
							{
								track_len = track_len + std->channels[0].stream[i];
								i++;
							}while( ( i < (int32_t)std->channels[0].nb_of_pulses ) && ( i < pl->forward_link[first_index] ) );
						}
					}

#ifdef FLUXSTREAMDBG
					fxs->hxcfe->hxc_printf(MSG_DEBUG,"First valid index : %d [max : %d] - track length : %d - overlap : %d",first_index,std->channels[0].nb_of_pulses,track_len,pl->forward_link[first_index]);
#endif

					if(!track_len)
					{
						int start_offset, end_offset, offset, index;

						index = hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution );
						if(index + 1 < std->nb_of_index)
						{
							start_offset = std->index_evt_tab[index].dump_offset;
							end_offset = std->index_evt_tab[index + 1].dump_offset;
							track_len = 0;
							if( start_offset >= 0 && start_offset < end_offset && end_offset < std->channels[0].nb_of_pulses)
							{
								offset = start_offset;
								while(offset < end_offset)
								{
									track_len += std->channels[0].stream[offset++];
								}

								i = 0;
								while(i<std->channels[0].nb_of_pulses)
								{
									pl->forward_link[i] = 2;
									i++;
								}

							#ifdef FLUXSTREAMDBG
								fxs->hxcfe->hxc_printf(MSG_INFO_1,"Revolution %d Null track len : Recomputed size = %d, start index %d, start offset : %d, end offset : %d",revolution,track_len,index,start_offset,end_offset);
							#endif
							}

						}
					}

#if 1
					if(pl->forward_link[first_index] == 1)
					{
#ifdef FLUXSTREAMDBG
						fxs->hxcfe->hxc_printf(MSG_DEBUG,"Dummy overlap - Use the index to index track lenght instead");
#endif
						first_index = std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution )].dump_offset;

						if( hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution ) < std->nb_of_index + 1)
						{
							next_index = std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution ) + 1 ].dump_offset;
						}
						else
						{
							next_index = std->channels[0].nb_of_pulses;
#ifdef FLUXSTREAMDBG
							fxs->hxcfe->hxc_printf(MSG_DEBUG,"No second index... Use the remaining track...");
#endif
						}

						track_len = 0;
						i = first_index;
						if( i < (int32_t)std->channels[0].nb_of_pulses)
						{
							do
							{
								pl->forward_link[ i ] = next_index + i;
								track_len = track_len + std->channels[0].stream[i];
								i++;
							}while( ( i < (int32_t)std->channels[0].nb_of_pulses ) && ( i < next_index ) );
						}
					}
#endif
					if(track_len)
					{
						rpm = (short)((double)(1 * 60 * 1000) / (double)( (double)tick_to_time(fxs,track_len) / (double)100000));

#ifdef FLUXSTREAMDBG
						fxs->hxcfe->hxc_printf(MSG_DEBUG,"RPM : %d - time : %d",rpm,tick_to_time(fxs,track_len));
#endif

						if(rpm <= 0 || rpm > 800 || tick_to_time(fxs,track_len) < 5000000 )
						{

							fxs->hxcfe->hxc_printf(MSG_ERROR,"Invalid rpm or tracklen (%d RPM, %d)...",rpm,tick_to_time(fxs, track_len));

							rpm = 300;

							first_index = std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, 0 )].dump_offset;

							i = 0;
							track_len = 0;
							do
							{
								track_len = track_len + std->channels[0].stream[first_index + i];
								i++;
							}while(i < (int32_t)std->channels[0].nb_of_pulses && track_len<time_to_tick(fxs, indexperiod));
							pl->forward_link[first_index] = first_index + i;
						}

						if( !fxs->defaultbitrate )
						{
							histo = (uint32_t*)malloc(65536* sizeof(uint32_t));
							if(histo)
							{
								if((first_index + (pl->forward_link[first_index] - first_index))<(int32_t)std->channels[0].nb_of_pulses)
								{
									hxcfe->hxc_printf(MSG_DEBUG,"Compute histogram from offset %d to offset %d... (pl->forward_link[first_index] = %d)",first_index, first_index + (pl->forward_link[first_index] - first_index), pl->forward_link[first_index] );
									computehistogram(&std->channels[0].stream[first_index],pl->forward_link[first_index] - first_index,histo);
								}
								else
								{
									hxcfe->hxc_printf(MSG_DEBUG,"Compute histogram from offset %d to offset %d (end buffer)...",first_index, first_index + (std->channels[0].nb_of_pulses - first_index) );
									computehistogram(&std->channels[0].stream[first_index],std->channels[0].nb_of_pulses - first_index,histo);
									hxcfe->hxc_printf(MSG_ERROR,"hxcfe_FxStream_AnalyzeAndGetTrack : End of the stream flux passed ! Bad Stream flux ?");
								}

								bitrate = detectpeaks(hxcfe, &fxs->pll, histo);

								hxcfe->hxc_printf(MSG_DEBUG,"%d RPM, Bitrate: %d",rpm,(int)(fxs->pll.tick_freq/bitrate) );

								hxcfe->hxc_printf(MSG_DEBUG,"Cells analysing...");

								free(histo);
							}
							else
							{
								bitrate = 0;
								hxcfe->hxc_printf(MSG_ERROR,"hxcfe_FxStream_AnalyzeAndGetTrack : Alloc error...");
							}
						}
						else
						{
							bitrate = fxs->pll.tick_freq / ( fxs->defaultbitrate * 1000 );
						}

						currentside = ScanAndDecodeStream(hxcfe,fxs,bitrate,std,pl,first_index,rpm,fxs->phasecorrection,0);

#ifndef FLUXSTREAMDBG
						cleanupTrack(currentside);
#else
						patchedbits = cleanupTrack(currentside);
						fxs->hxcfe->hxc_printf(MSG_DEBUG,"%d bits patched",patchedbits);
#endif
						revolutionside[revolution] = currentside;

						hxcfe->hxc_printf(MSG_DEBUG,"...done");
					}
					else
					{
						revolutionside[revolution] = 0;
					}
				}

				memset(qualitylevel,0,sizeof(qualitylevel));

				first_track_encoding = UNKNOWN_ENCODING;

				for(revolution = 0; revolution < nb_of_revolutions; revolution++)
				{
					currentside = revolutionside[revolution];
#ifdef FLUXSTREAMDBG
					fxs->hxcfe->hxc_printf(MSG_DEBUG,"Scanning revolution %d [0x%X]...",revolution,revolutionside[revolution]);
#endif
					if(currentside)
					{
						currentside->track_encoding = UNKNOWN_ENCODING;

						fp = makefloppyfromtrack(revolutionside[revolution]);

						i = 0;
						while( tracktypelist[i] != UNKNOWN_ENCODING )
						{
							ss = hxcfe_initSectorAccess(fxs->hxcfe,fp);

							scl = hxcfe_getAllTrackSectors(ss,0,0,tracktypelist[i],&nb_sectorfound);

							if(scl)
							{
								for(sectnum=0;sectnum<nb_sectorfound;sectnum++)
								{
									if(!scl[sectnum]->use_alternate_header_crc)
										qualitylevel[revolution] += 0x010000;
									if(!scl[sectnum]->use_alternate_data_crc && scl[sectnum]->input_data)
										qualitylevel[revolution] += 0x000001;

									if(first_track_encoding == UNKNOWN_ENCODING)
										first_track_encoding = tracktypelist[i];

									hxcfe_freeSectorConfig  (ss,scl[sectnum]);
								}
								free(scl);
							}

							hxcfe_deinitSectorAccess(ss);

							if(first_track_encoding != UNKNOWN_ENCODING)
								currentside->track_encoding = first_track_encoding;

							i++;
						}

						freefloppy(fp);

						printsidestat(fxs,currentside);
					}

				}

#ifdef FLUXSTREAMDBG
				for(revolution = 0; revolution < nb_of_revolutions; revolution++)
				{
					fxs->hxcfe->hxc_printf(MSG_DEBUG,"revolution %d : 0x%.8X",revolution,qualitylevel[revolution]);
				}
#endif

				revolution = getbestrevolution(qualitylevel,nb_of_revolutions);
#ifdef FLUXSTREAMDBG
				fxs->hxcfe->hxc_printf(MSG_DEBUG,"getbestrevolution : %d",revolution);
#endif
				revolution = getbestindex(fxs,std,pl,revolution,qualitylevel,nb_of_revolutions - 1);
#ifdef FLUXSTREAMDBG
				fxs->hxcfe->hxc_printf(MSG_DEBUG,"getbestindex : %d",revolution);
#endif

				if(revolutionside[revolution])
				{
					revolutionside[revolution]->track_encoding = first_track_encoding;

					currentside = revolutionside[revolution];
					revolutionside[revolution] = 0;
				}
				else
				{
					fxs->hxcfe->hxc_printf(MSG_ERROR,"NULL revolutionside !!! (%d)",revolution);

					revolution = 0;
					while(!revolutionside[revolution] && (revolution < nb_of_revolutions))
					{
						revolution++;
					}

					if(revolution != (nb_of_revolutions) )
					{
						revolutionside[revolution]->track_encoding = first_track_encoding;

						currentside = revolutionside[revolution];
						revolutionside[revolution] = 0;
					}
				}

				// Try here to recover the left bad sectors from others revolutions.
				if(currentside && bitrate && fxs->sector_recovery)
				{
					tmp_side = NULL;

#ifdef FLUXSTREAMDBG
					fxs->hxcfe->hxc_printf(MSG_DEBUG,"Sectors recovering ... :");
#endif

					fp = makefloppyfromtrack(currentside);

					i = 0;
					while( tracktypelist[i] != UNKNOWN_ENCODING )
					{
						ss = hxcfe_initSectorAccess(fxs->hxcfe,fp);

						scl = hxcfe_getAllTrackSectors(ss,0,0,tracktypelist[i],&nb_sectorfound);

						if(scl)
						{
							if(!tmp_side)
							{
								tmp_side = ScanAndDecodeStream(hxcfe,fxs,bitrate,std,NULL,0,rpm,fxs->phasecorrection,0);
								cleanupTrack(tmp_side);
							}

							for(sectnum=0;sectnum<nb_sectorfound && tmp_side;sectnum++)
							{
								if( scl[sectnum]->use_alternate_data_crc && scl[sectnum]->input_data)
								{
									// Bad sector data...
#ifdef FLUXSTREAMDBG
									fxs->hxcfe->hxc_printf(MSG_DEBUG,"Bad sector -> head:%d sector:%d sectorsleft:%d cylinder:%d startsectorindex:%d startdataindex:%d endsectorindex:%d\n",scl[sectnum]->head,scl[sectnum]->sector,scl[sectnum]->sectorsleft,scl[sectnum]->cylinder,scl[sectnum]->startsectorindex,scl[sectnum]->startdataindex,scl[sectnum]->endsectorindex);
#endif
									if( scl[sectnum]->startsectorindex < scl[sectnum]->endsectorindex )
									{

										HXCFE_SECTCFG* tmp_scfg;

										HXCFE_FLOPPY * tmp_fp = makefloppyfromtrack(tmp_side);
										HXCFE_SECTORACCESS* tmp_ss = hxcfe_initSectorAccess( fxs->hxcfe, tmp_fp );
										if( tmp_ss )
										{
											do
											{
												tmp_scfg = hxcfe_getNextSector(tmp_ss,0,0,tracktypelist[i]);
												if( tmp_scfg )
												{
													if( (tmp_scfg->sector == scl[sectnum]->sector ) &&
														(tmp_scfg->head == scl[sectnum]->head ) &&
														(tmp_scfg->cylinder == scl[sectnum]->cylinder ) &&
														(tmp_scfg->trackencoding == scl[sectnum]->trackencoding ) &&
														(tmp_scfg->header_crc == scl[sectnum]->header_crc )
													)
													{
														if( !tmp_scfg->use_alternate_data_crc && tmp_scfg->input_data)
														{
#ifdef FLUXSTREAMDBG
															fxs->hxcfe->hxc_printf(MSG_DEBUG,"Recover sector s%d:c%d:h%d",tmp_scfg->sector,tmp_scfg->cylinder,tmp_scfg->head);
#endif
															hxcfe_removeCell( fxs->hxcfe, currentside, scl[sectnum]->startsectorindex,  scl[sectnum]->endsectorindex - scl[sectnum]->startsectorindex );
															hxcfe_insertCell( fxs->hxcfe, currentside, scl[sectnum]->startsectorindex, 0,  tmp_scfg->endsectorindex - tmp_scfg->startsectorindex );

															for(int k=0;k<tmp_scfg->endsectorindex - tmp_scfg->startsectorindex;k++)
															{
																hxcfe_setCellState( fxs->hxcfe, currentside, scl[sectnum]->startsectorindex + k,
																		hxcfe_getCellState( fxs->hxcfe, tmp_side, tmp_scfg->startsectorindex + k) );
															}

															hxcfe_freeSectorConfig(tmp_ss, tmp_scfg);
															tmp_scfg = NULL;
														}
													}

													hxcfe_freeSectorConfig(tmp_ss, tmp_scfg);
												}

											}while( tmp_scfg );

											hxcfe_deinitSectorAccess(tmp_ss);
										}

										freefloppy(tmp_fp);
									}
								}

								hxcfe_freeSectorConfig  (ss,scl[sectnum]);
							}
							free(scl);
						}

						hxcfe_deinitSectorAccess(ss);

						i++;
					}

					freefloppy(fp);

#ifdef FLUXSTREAMDBG
					fxs->hxcfe->hxc_printf(MSG_DEBUG,"... Sectors recovering done");
#endif
					hxcfe_freeSide(fxs->hxcfe,tmp_side);

				}

				if( currentside )
					hxcfe_shiftTrackData( fxs->hxcfe, currentside, us2index(0,currentside,(uint32_t)((std->index_evt_tab[hxcfe_FxStream_GetRevolutionIndex( fxs, std, revolution )].tick_offset)*(double)((double)1000000/(double)fxs->pll.tick_freq))& (~0x00000007),0,0) );

				for(revolution = 0; revolution < nb_of_revolutions; revolution++)
				{
					hxcfe_freeSide(fxs->hxcfe,revolutionside[revolution]);
				}

				free_pulses_link_array(pl);
			}
		}
		else
		{
			fxs->hxcfe->hxc_printf(MSG_ERROR,"Index Period ! (%d ms)",(indexperiod / (1000*100)));
		}

		free(tb->blocks);
		free(tb);

		/*j=currentside->tracklen/8;
		if(currentside->tracklen&7) j++;
		for(i=0;i<j;i++)
		{
			hxcfe->hxc_printf(MSG_DEBUG,"D:%.2X\tM:%.2X\tBR:%.8d\tI:%.2X",
					currentside->databuffer[i],
					currentside->flakybitsbuffer[i],
					currentside->timingbuffer[i],
					currentside->indexbuffer[i]);
		}*/

	}

	return currentside;
}

void hxcfe_FxStream_ExportToBmp(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * stream, char * filename)
{
	HXCFE_TD * td;
	uint32_t flags;

	td = hxcfe_td_init(fxs->hxcfe, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_DEFAULT_XSIZE" ), hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_DEFAULT_YSIZE" ) );
	if(td)
	{
		hxcfe_td_activate_analyzer(td, ISOIBM_MFM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_ISOIBM_MFM_ENCODING"));
		hxcfe_td_activate_analyzer(td, ISOIBM_FM_ENCODING,  hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_ISOIBM_FM_ENCODING"));
		hxcfe_td_activate_analyzer(td, AMIGA_MFM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_AMIGA_MFM_ENCODING"));
		hxcfe_td_activate_analyzer(td, EMU_FM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_EMU_FM_ENCODING"));
		hxcfe_td_activate_analyzer(td, MEMBRAIN_MFM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_MEMBRAIN_MFM_ENCODING"));
		hxcfe_td_activate_analyzer(td, TYCOM_FM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_TYCOM_FM_ENCODING"));
		hxcfe_td_activate_analyzer(td, APPLEII_GCR1_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_APPLEII_GCR1_ENCODING"));
		hxcfe_td_activate_analyzer(td, APPLEII_GCR2_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_APPLEII_GCR2_ENCODING"));
		hxcfe_td_activate_analyzer(td, APPLEMAC_GCR_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_APPLEMAC_GCR_ENCODING"));
		hxcfe_td_activate_analyzer(td, ARBURGDAT_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_ARBURGDAT_ENCODING"));
		hxcfe_td_activate_analyzer(td, ARBURGSYS_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_ARBURGSYS_ENCODING"));
		hxcfe_td_activate_analyzer(td, NORTHSTAR_HS_MFM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_NORTHSTAR_HS_MFM_ENCODING"));
		hxcfe_td_activate_analyzer(td, HEATHKIT_HS_FM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_HEATHKIT_HS_FM_ENCODING"));
		hxcfe_td_activate_analyzer(td, DEC_RX02_M2FM_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_DEC_RX02_M2FM_ENCODING"));
		hxcfe_td_activate_analyzer(td, QD_MO5_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_QD_MO5_ENCODING"));
		hxcfe_td_activate_analyzer(td, C64_GCR_ENCODING, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_ENABLE_C64_GCR_ENCODING"));

		flags = 0;

		if( hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_HIGHCONTRAST" ) )
		{
			flags |= TD_FLAG_HICONTRAST;
		}

		if( hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_BIG_DOTS" ) )
		{
			flags |= TD_FLAG_BIGDOT;
		}

		hxcfe_td_setparams(td, hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_DEFAULT_XTOTALTIME" ), hxcfe_getEnvVarValue( fxs->hxcfe, "BMPEXPORT_STREAM_DEFAULT_YTOTALTIME" ),0, flags);

		hxcfe_td_draw_trkstream( td, stream );

		hxcfe_td_exportToBMP( td, filename );

		hxcfe_td_deinit(td);
	}
}

void hxcfe_FxStream_FreeStream(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * stream)
{
	if(fxs)
	{
		if(stream)
		{
			free(stream->channels[0].stream);

			free(stream);
		}
	}
}

void hxcfe_deinitFxStream(HXCFE_FXSA * fxs)
{
	free(fxs);
}

#if 0
void hxcfe_FxStream_JitterFilter(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * stream)
{
	int i,o1,o2;
	int old,error;

	if(fxs)
	{
		if(stream)
		{
			if(stream->channels[0].stream)
			{
				old = (stream->channels[0].stream[i] - stream->channels[0].stream[i - 1] );
				for(i=1;i<stream->channels[0].nb_of_pulses;i++)
				{
					error = ( stream->channels[0].stream[i] - stream->channels[0].stream[i - 1] );

					if( 0 ) //abs(error) < 150 ) //stream->channels[0].stream[i]  / 8 )
					{
						o1 = stream->channels[0].stream[i - 1];
						o2 = stream->channels[0].stream[i + 0];

						// fix jitter

						stream->channels[0].stream[i - 1] += error/4;
						stream->channels[0].stream[i] -= error/4;

						//if( ( (error/2 + error/2) != error) && error > 1)
						  //  stream->channels[0].stream[i - 1]++;

						//printf("before : %d - %d | after : %d - %d\n",o1,o2,stream->channels[0].stream[i - 1],stream->channels[0].stream[i + 0]);

					}
					//stream->channels[0].stream[i] = (stream->channels[0].stream[i]/32)*32;
				}
			}
		}
	}
}

#endif
