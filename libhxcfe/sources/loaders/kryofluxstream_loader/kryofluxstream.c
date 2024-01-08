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
// File : kryofluxstream.c
// Contains: KryoFlux Stream file loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "libhxcadaptor.h"

/*
#define KF_MCLOCK 48054857,14 //(((18432000 * 73) / 14) / 2)
#define KF_SCLOCK ((float)KF_MCLOCK / (float)2)
#define KF_ICLOCK (KF_MCLOCK / 16)
*/

#define KF_NS_PER_TICK 41619 // 41,619 ns per tick

#define MAX_INDEX 128

//#define KFSTREAMDBG 1

typedef struct Index_
{
	uint32_t 	StreamPosition;
	uint32_t 	Timer;
	uint32_t 	SysClk;
	uint32_t 	CellPos;
	uint32_t 	IndexTime;
	uint32_t 	PreIcTime;
	uint32_t 	PostIcTime;

	uint32_t 	Prev_Index_Tick;
	uint32_t 	Next_Index_Tick;
	uint32_t 	type;
}Index;

static uint32_t get_tick_from_reversal(uint32_t* buffer,uint32_t reversal)
{
	uint32_t i;
	uint32_t tick;

	tick = 0;

	for(i = 0;i < reversal; i++ )
	{
		tick += buffer[i];
	}

	return tick;
}

HXCFE_TRKSTREAM* DecodeKFStreamFile(HXCFE* floppycontext,HXCFE_FXSA * fxs,char * file)
{
	uint32_t i;
	s_oob_header		* oob;
	s_oob_DiskIndex		* diskIndex;
	HXCFE_TRKSTREAM		* track_dump;

	Index index_events[MAX_INDEX];

	FILE* f;

	#ifdef KFSTREAMDBG

	char * tempstr;
	s_oob_StreamRead	* streamRead;
	s_oob_StreamEnd		* streamEnd;

	#endif

	unsigned char * kfstreambuffer;
	uint32_t * cellstream;
	uint32_t * cellstreampos;

	uint32_t nxt_index;
	uint32_t cell_accumulator;
	uint32_t nbindex;
	uint32_t stream_ofs;
	uint32_t stream_pos;
	uint32_t cellpos;
	uint32_t oobEnd;

	uint32_t filesize;

	uint32_t totalcell;
	uint32_t next_cell_pos;
	uint32_t index_cell_time;
	uint32_t next_index_stream_pos;
	uint32_t Prev_Max_Index_Tick;
	uint32_t Next_Max_Index_Tick;
	uint8_t  cur_op,cur_op_len;
	int      new_cell_available;

	track_dump = NULL;
	kfstreambuffer = NULL;

	if(fxs)
	{
		f = hxc_fopen(file,"rb");
		if(f)
		{
			filesize = hxc_fgetsize(f);

			kfstreambuffer = malloc(filesize);
			if( kfstreambuffer )
			{
				hxc_fread(kfstreambuffer,filesize,f);

				hxc_fclose(f);
			}
			else
			{
				hxc_fclose(f);

				return 0;
			}

			cellstream = (uint32_t*)malloc(filesize*sizeof(uint32_t));
			cellstreampos = (uint32_t*)malloc(filesize*sizeof(uint32_t));

			if( !cellstream || !cellstreampos )
			{
				free(cellstream);
				free(cellstreampos);
				free(kfstreambuffer);

				return NULL;
			}

			memset(cellstream,0,filesize*sizeof(uint32_t));
			memset(cellstreampos,0,filesize*sizeof(uint32_t));

			cell_accumulator = 0;
			stream_ofs = 0;
			stream_pos = 0;
			nbindex = 0;
			cellpos = 0;
			oobEnd = 0;
			while( ( stream_ofs < filesize ) && !oobEnd)
			{
				cur_op = kfstreambuffer[stream_ofs];
				cur_op_len = 0;

				// Set Operation / Data code lenght
				switch (cur_op)
				{
					case KF_STREAM_OP_NOP1:
						cur_op_len = 1;
					break;
					case KF_STREAM_OP_NOP2:
						cur_op_len = 2;
					break;
					case KF_STREAM_OP_NOP3:
						cur_op_len = 3;
					break;
					case KF_STREAM_OP_OVERFLOW:
						cur_op_len = 1;
					break;
					case KF_STREAM_OP_VALUE16:
						cur_op_len = 3;
					break;
					case KF_STREAM_OP_OOB:
						oob = (s_oob_header*)&kfstreambuffer[stream_ofs];

						cur_op_len = sizeof(s_oob_header);
						if (filesize - stream_ofs >= cur_op_len)
						{
							cur_op_len = sizeof(s_oob_header);
							if (oob->Type != OOBTYPE_End)
							{
								cur_op_len += oob->Size;
							}
						}
					break;

					default:
						if (cur_op >= KF_STREAM_DAT_BYTE)
						{
							cur_op_len = 1;
						}
						else
						{
							if (!(cur_op & ~KF_STREAM_DAT_MASK_SHORT))
								cur_op_len = 2;
							else
							{
								free(cellstream);
								free(cellstreampos);
								free(kfstreambuffer);

								return NULL;
							}
						}
					break;
				}

				// Bad operation/data size ?
				if (filesize - stream_ofs < cur_op_len)
				{
					free(cellstream);
					free(cellstreampos);
					free(kfstreambuffer);

					return NULL;
				}

				new_cell_available = 0;

				switch(cur_op)
				{
					//0x0B Overflow16 1 Next cell value is increased by 0×10000 (16-bits).
					//                  Decoding of *this* cell should continue at next stream position
					case KF_STREAM_OP_OVERFLOW:
						cell_accumulator +=  0x10000;
					break;

					//0x0C Value16 3 New cell value: Upper 8 bits are offset+1 in the stream, lower 8-bits are offset+2
					case KF_STREAM_OP_VALUE16:
						// 2 Bytes Flux (full)
						cell_accumulator += ( kfstreambuffer[stream_ofs + 1] << 8 ) | kfstreambuffer[stream_ofs + 2];
						new_cell_available = 1;
					break;

					default:
						if (cur_op >= KF_STREAM_DAT_BYTE)
						{
							cell_accumulator += cur_op;
							new_cell_available = 1;
						}
						else
						{
							if (!(cur_op & ~KF_STREAM_DAT_MASK_SHORT))
							{
								cell_accumulator += (((uint32_t)cur_op)<<8) | kfstreambuffer[stream_ofs + 1];
								new_cell_available = 1;
							}
						}
					break;
				}

				if( cur_op != KF_STREAM_OP_OOB )
				{
					if(new_cell_available)
					{
						cellstream[cellpos] = cell_accumulator;
						cellstreampos[cellpos] = stream_pos;
						cellpos++;
						cell_accumulator = 0;
					}

					stream_pos += cur_op_len;
				}
				else
				{
					oob = (s_oob_header*)&kfstreambuffer[stream_ofs];

					switch(oob->Type)
					{
						case OOBTYPE_Stream_Read:
	#ifdef KFSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"---Stream Read---");
	#endif

	#ifdef KFSTREAMDBG
							streamRead = (s_oob_StreamRead*) &kfstreambuffer[stream_ofs + sizeof(s_oob_header) ];
							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X TrTime: 0x%.8X",streamRead->StreamPosition,streamRead->TrTime);
	#endif
							break;

						case OOBTYPE_Index:
							floppycontext->hxc_printf(MSG_DEBUG,"---Index--- : %d sp:%d",nbindex,cellpos);

							diskIndex=	(s_oob_DiskIndex*) &kfstreambuffer[stream_ofs + sizeof(s_oob_header) ];

							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X SysClk: 0x%.8X Timer: 0x%.8X",diskIndex->StreamPosition,diskIndex->SysClk,diskIndex->Timer);

							if(nbindex < MAX_INDEX)
							{
								index_events[nbindex].StreamPosition = diskIndex->StreamPosition;
								index_events[nbindex].SysClk = diskIndex->SysClk;
								index_events[nbindex].Timer = diskIndex->Timer;
								index_events[nbindex].CellPos = 0;
								index_events[nbindex].IndexTime = 0;
								index_events[nbindex].PreIcTime = 0;
								index_events[nbindex].PostIcTime = 0;
								index_events[nbindex].Prev_Index_Tick = 0;
								index_events[nbindex].Next_Index_Tick = 0;
								nbindex++;
							}
							else
							{
								floppycontext->hxc_printf(MSG_ERROR,"DecodeKFStreamFile : nbindex >= MAX_INDEX (%d)!",MAX_INDEX);
							}
							break;

						case OOBTYPE_Stream_End:
	#ifdef KFSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"---Stream End---");
	#endif

	#ifdef KFSTREAMDBG
							streamEnd =	(s_oob_StreamEnd*) &kfstreambuffer[stream_ofs + sizeof(s_oob_header) ];
							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X Result: 0x%.8X",streamEnd->StreamPosition,streamEnd->Result);
	#endif
						break;

						case OOBTYPE_String:
	#ifdef KFSTREAMDBG
							floppycontext->hxc_printf(MSG_DEBUG,"---String---");
	#endif
	#ifdef KFSTREAMDBG
							tempstr = malloc(oob->Size+1);
							if( tempstr )
							{
								memset(tempstr,0,oob->Size+1);
								memcpy(tempstr,&kfstreambuffer[stream_ofs + sizeof(s_oob_header)],oob->Size);

								floppycontext->hxc_printf(MSG_DEBUG,"String : %s",tempstr);
								free(tempstr);
								tempstr = NULL;
							}
	#endif
						break;

						case OOBTYPE_End:
							oobEnd = 1;
						break;

						default:
							oob = (s_oob_header*)&kfstreambuffer[stream_ofs];
							floppycontext->hxc_printf(MSG_DEBUG,"Unknown OOB : 0x%.2x 0x%.2x Size:0x%.4x",oob->Sign,oob->Type,oob->Size);
						break;

					}
				}

				stream_ofs += cur_op_len;
			};

			cellstream[cellpos] = cell_accumulator;
			cellstreampos[cellpos] = stream_ofs;

			totalcell = cellpos;

			if(nbindex)
			{
				nxt_index = 0;
				next_cell_pos = 0;
				next_index_stream_pos = index_events[nxt_index].StreamPosition;

				for (cellpos=0; cellpos < totalcell; cellpos++)
				{
					next_cell_pos = cellpos + 1;

					if( nxt_index < nbindex )
					{
						if( next_index_stream_pos <= cellstreampos[next_cell_pos] )
						{
							// Index reached
							if(!cellpos)
							{
								// Index at cell 0 ?
								if (cellstreampos[0] >= next_index_stream_pos )
								{
									next_cell_pos = 0;
								}
							}

							index_events[nxt_index].CellPos = next_cell_pos;

							index_cell_time = cellstream[next_cell_pos];

							if( !index_events[nxt_index].Timer )
							{
								index_events[nxt_index].Timer = index_cell_time;
							}

							if( next_cell_pos >= totalcell )
							{
								if( cellstreampos[next_cell_pos] == next_index_stream_pos )
								{
									index_cell_time += index_events[nxt_index].Timer;
									cellstream[next_cell_pos] = index_cell_time;
								}
							}

							nxt_index++;

							if( nxt_index < nbindex )
							{
								next_index_stream_pos = index_events[nxt_index].StreamPosition;
							}
							else
							{
								next_index_stream_pos = 0;
							}
						}
					}
				}
			}

			hxcfe_FxStream_setResolution(fxs,KF_NS_PER_TICK);

			track_dump = hxcfe_FxStream_ImportStream(fxs,cellstream,32,cellpos, HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data",NULL);

			for(i=0;i<nbindex;i++)
			{
				index_events[i].type = FXSTRM_INDEX_MAININDEX;
			}

			if( hxcfe_getEnvVarValue( fxs->hxcfe, "FLUXSTREAM_ALL_REVOLUTIONS_IN_ONE" ) )
			{
				hxcfe_FxStream_AddIndex(fxs,track_dump,index_events[0].CellPos,index_events[0].Timer,index_events[0].type);
				hxcfe_FxStream_AddIndex(fxs,track_dump,index_events[nbindex-1].CellPos,index_events[nbindex-1].Timer,index_events[nbindex-1].type);
			}
			else
			{
				// Hard sector indexes detection
				if(nbindex > 1)
				{
					for( i = 0; i < nbindex ; i++ )
					{
						if(i)
						{
							index_events[i].Prev_Index_Tick = get_tick_from_reversal(cellstream,index_events[i].CellPos) - \
															  get_tick_from_reversal(cellstream,index_events[i - 1].CellPos);
						}

						if( i < (nbindex - 1))
						{
							index_events[i].Next_Index_Tick = get_tick_from_reversal(cellstream,index_events[i + 1].CellPos) - \
															  get_tick_from_reversal(cellstream,index_events[i].CellPos);
						}
					}
				}

				Prev_Max_Index_Tick = 0;
				Next_Max_Index_Tick = 0;
				for( i = 0; i < nbindex ; i++ )
				{
					if( Prev_Max_Index_Tick < index_events[i].Prev_Index_Tick )
					{
						Prev_Max_Index_Tick = index_events[i].Prev_Index_Tick;
					}

					if( Next_Max_Index_Tick < index_events[i].Next_Index_Tick )
					{
						Next_Max_Index_Tick = index_events[i].Next_Index_Tick;
					}
				}

				// Max index delta is < to 50 ms, load this track as an hard sectored track.
				if ( (Next_Max_Index_Tick * ((float)KF_NS_PER_TICK * 10E-10)) < 50 && (Prev_Max_Index_Tick * ((float)KF_NS_PER_TICK * 10E-10)) < 50 )
				{
					for( i = 0; i < nbindex ; i++ )
					{
						if ( ( index_events[i].Prev_Index_Tick > ( Prev_Max_Index_Tick / 4 ) * 3 ) &&
							 ( index_events[i].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ) )
						{
							index_events[i].type = FXSTRM_INDEX_SECTORINDEX;
						}
						else
						{
							if ( ( index_events[i].Prev_Index_Tick > ( Prev_Max_Index_Tick / 4 ) * 3 ) ||
								 ( index_events[i].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ) )
							{
								index_events[i].type = FXSTRM_INDEX_SECTORINDEX;
							}
							else
							{
								if( index_events[i].Prev_Index_Tick && index_events[i].Next_Index_Tick )
								{
									index_events[i].type = FXSTRM_INDEX_MAININDEX;
								}
							}
						}
					}

					if( ( index_events[0].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ))
					{
						index_events[0].type = FXSTRM_INDEX_SECTORINDEX;
					}
					else
					{
						if( ( index_events[1].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ))
						{
							index_events[0].type = FXSTRM_INDEX_MAININDEX;
						}
						else
						{
							index_events[0].type = FXSTRM_INDEX_SECTORINDEX;
						}
					}

					if(nbindex)
					{
						if( ( index_events[nbindex-1].Prev_Index_Tick < ( Next_Max_Index_Tick / 4 ) * 3 ))
						{
							if ( ( index_events[nbindex-2].Prev_Index_Tick < ( Prev_Max_Index_Tick / 4 ) * 3 ) &&
								 ( index_events[nbindex-2].Next_Index_Tick < ( Next_Max_Index_Tick / 4 ) * 3 ) )
							{
								index_events[nbindex-1].type = FXSTRM_INDEX_SECTORINDEX;
							}
							else
							{
								index_events[nbindex-1].type = FXSTRM_INDEX_MAININDEX;
							}
						}
						else
						{
							index_events[nbindex-1].type = FXSTRM_INDEX_SECTORINDEX;
						}
					}
				}

				for( i = 0; i < nbindex ; i++ )
				{
					floppycontext->hxc_printf(MSG_DEBUG,"Index %d : Prev delta %f ms, Next delta %f ms, Type : 0x%x",i, \
								(float)index_events[i].Prev_Index_Tick * ((float)KF_NS_PER_TICK * 10E-10), \
								(float)index_events[i].Next_Index_Tick * ((float)KF_NS_PER_TICK * 10E-10), \
								index_events[i].type);

					hxcfe_FxStream_AddIndex(fxs,track_dump,index_events[i].CellPos,index_events[i].Timer,index_events[i].type);
				}
			}

			free(cellstream);
			free(cellstreampos);
			free(kfstreambuffer);
		}
	}

	return track_dump;
}
