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
// File : kryofluxstream.c
// Contains: KryoFlux Stream file loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "os_api.h"

s_track_dump* DecodeKFStreamFile(HXCFLOPPYEMULATOR* floppycontext,char * file,float timecoef)
{
	unsigned long i;
	s_oob_header		* oob;
	s_oob_StreamRead	* streamRead;
	s_oob_StreamEnd		* streamEnd;
	s_oob_DiskIndex		* diskIndex;
	s_track_dump* track_dump;
	
	s_oob_DiskIndex  tabindex[16];

	FILE* f;
#ifdef KFSTREAMDBG	
	char * tempstr;
#endif
	unsigned char * kfstreambuffer;
	unsigned long * cellstream;

	unsigned long cell_value;
	unsigned long nbindex;
	unsigned long offset;
	unsigned long cellpos;
	unsigned long streamend;

	int inc0B;

	unsigned long filesize;

	track_dump=malloc(sizeof(s_track_dump));

	if(track_dump)
	{
		memset(track_dump,0,sizeof(s_track_dump));

		f=fopen(file,"rb");
		if(f)
		{
			fseek(f,0,SEEK_END);
			filesize=ftell(f);
			fseek(f,0,SEEK_SET);

			kfstreambuffer=malloc(filesize);

			fread(kfstreambuffer,filesize,1,f);

			fclose(f);

			cellstream=(unsigned long*)malloc(filesize*sizeof(unsigned long));
			memset(cellstream,0,filesize*sizeof(unsigned long));

			cell_value=0;
			offset=0;
			nbindex=0;
			cellpos=0;
			streamend=0;
			do
			{
				switch(kfstreambuffer[offset])
				{
					case 0x00:
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x05:
					case 0x06:
					case 0x07:
						cell_value = kfstreambuffer[offset] << 8;
						offset++;
						cell_value = cell_value | kfstreambuffer[offset];
						offset++;

						if(inc0B)
						{
							cell_value=cell_value+ 0x10000;
							inc0B=0;
						}
						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
	
					break;
				
					// Nop
					case 0x0A:
						offset++;
					case 0x09:
						offset++;
					case 0x08:
						offset++;
					break;
						//
						
					//0x0B 	Overflow16 	1 	Next cell value is increased by 0×10000 (16-bits). Decoding of *this* cell should continue at next stream position
					case 0x0B:
						inc0B=1;
						offset++;
					break;

					//0x0C 	Value16 	3 	New cell value: Upper 8 bits are offset+1 in the stream, lower 8-bits are offset+2 
					case 0x0C:
						offset++;
						cell_value = kfstreambuffer[offset] << 8;
						offset++;
						cell_value = cell_value | kfstreambuffer[offset];
						offset++;

						if(inc0B)
						{
							cell_value=cell_value+ 0x10000;
							inc0B=0;
						}
						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
					break;

					case 0x0D:
						oob=(s_oob_header*)&kfstreambuffer[offset];

		#ifdef KFSTREAMDBG
						floppycontext->hxc_printf(MSG_DEBUG,"OOB 0x%.2x 0x%.2x Size:0x%.4x",oob->Sign,oob->Type,oob->Size);
		#endif

						switch(oob->Type)
						{
							case 0x01:
		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"---Stream Read---");
		#endif
								streamRead=	(s_oob_StreamRead*) &kfstreambuffer[offset + sizeof(s_oob_header) ];

		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X TrTime: 0x%.8X",streamRead->StreamPosition,streamRead->TrTime);
		#endif
								break;

							case 0x02:
		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"---Index--- : %d sp:%d",nbindex,cellpos);
		#endif

								diskIndex=	(s_oob_DiskIndex*) &kfstreambuffer[offset + sizeof(s_oob_header) ];

		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X SysClk: 0x%.8X Timer: 0x%.8X",diskIndex->StreamPosition,diskIndex->SysClk,diskIndex->Timer);
		#endif

								tabindex[nbindex].StreamPosition=diskIndex->StreamPosition;
								tabindex[nbindex].SysClk=diskIndex->SysClk;
								tabindex[nbindex].Timer=diskIndex->Timer;
								if(nbindex)
								{
		#ifdef KFSTREAMDBG
									floppycontext->hxc_printf(MSG_DEBUG,"Delta : %d Rpm : %f ",tabindex[nbindex].SysClk-tabindex[nbindex-1].SysClk,(float)(ick*(float)60)/(float)(tabindex[nbindex].SysClk-tabindex[nbindex-1].SysClk));
		#endif
								}
								nbindex++;
								break;

							case 0x03:
		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"---Stream End---");
		#endif
								streamEnd=	(s_oob_StreamEnd*) &kfstreambuffer[offset + sizeof(s_oob_header) ];
		#ifdef KFSTREAMDBG							
								floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X Result: 0x%.8X",streamEnd->StreamPosition,streamEnd->Result);
		#endif
							break;

							case 0x04:
		#ifdef KFSTREAMDBG
								floppycontext->hxc_printf(MSG_DEBUG,"---String---");
		#endif
		#ifdef KFSTREAMDBG
								tempstr=malloc(oob->Size+1);
								memset(tempstr,0,oob->Size+1);
								memcpy(tempstr,&kfstreambuffer[offset + sizeof(s_oob_header)],oob->Size);
		
								floppycontext->hxc_printf(MSG_DEBUG,"String : %s",tempstr);		
								free(tempstr);	
		#endif
							break;

							case 0x0D:
								streamend=1;
							break;

							default:
								floppycontext->hxc_printf(MSG_DEBUG,"Unknown OOB : 0x%.2x 0x%.2x Size:0x%.4x",oob->Sign,oob->Type,oob->Size);
								break;

						}
						offset=offset+oob->Size + sizeof(s_oob_header);
					break;
				
					default:
						cell_value = kfstreambuffer[offset];

						if(inc0B)
						{
							cell_value=cell_value+ 0x10000;
							inc0B=0;
						}
						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
						offset++;
					break;
				}

			}while( (offset<filesize) && !streamend);

			track_dump->nb_of_pulses=cellpos;
			if(track_dump->nb_of_pulses)
			{
				track_dump->track_dump=malloc( track_dump->nb_of_pulses * sizeof(unsigned long) ); 
				memcpy(track_dump->track_dump, cellstream, track_dump->nb_of_pulses * sizeof(unsigned long) );
			}
			free(cellstream);
			free(kfstreambuffer);

			if(nbindex)
			{
				track_dump->index_evt_tab=malloc(sizeof(s_index_evt)*nbindex);
				memset(track_dump->index_evt_tab,0,sizeof(s_index_evt)*nbindex);
				for(i=0;i<nbindex;i++)
				{
					track_dump->index_evt_tab[i].dump_offset=tabindex[i].StreamPosition;
					track_dump->index_evt_tab[i].clk=(unsigned long)((float)tabindex[i].SysClk * timecoef);
				}
				track_dump->nb_of_index=nbindex;
			}
		}
		else
		{
			free(track_dump);
			track_dump=0;
		}
	}

	return track_dump;
}

void FreeStream(s_track_dump* trackdump)
{
	if(trackdump)
	{
		if(trackdump->track_dump)
			free(trackdump->track_dump);

		if(trackdump->index_evt_tab)
			free(trackdump->index_evt_tab);

		free(trackdump);
	}
}