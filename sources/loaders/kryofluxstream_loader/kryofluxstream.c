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
#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "libhxcadaptor.h"

/*#define KF_MCLOCK 48054857,14 //(((18432000 * 73) / 14) / 2)
#define KF_SCLOCK ((float)KF_MCLOCK / (float)2)
#define KF_ICLOCK (KF_MCLOCK / 16)
*/

//#define KFSTREAMDBG 1

HXCFE_TRKSTREAM* DecodeKFStreamFile(HXCFE* floppycontext,HXCFE_FXSA * fxs,char * file,float timecoef)
{
	unsigned long i;
	s_oob_header		* oob;
	s_oob_StreamRead	* streamRead;
	s_oob_StreamEnd		* streamEnd;
	s_oob_DiskIndex		* diskIndex;
	HXCFE_TRKSTREAM		* track_dump;

	s_oob_DiskIndex  tabindex[16];

	FILE* f;
#ifdef KFSTREAMDBG
	double mck;
	double sck;
	double ick;

	char * tempstr;
#endif
	unsigned char * kfstreambuffer;
	unsigned long * cellstream;

	unsigned long cell_value;
	unsigned long nbindex;
	unsigned long offset;
	unsigned long cellpos;
	unsigned long streamend;

	unsigned long overflowvalue;

	unsigned long filesize;

	unsigned long totalcell,totalstreampos;

	track_dump=0;
	overflowvalue=0;
	totalcell = 0;
	totalstreampos = 0;

	if(fxs)
	{
		f=hxc_fopen(file,"rb");
		if(f)
		{
			fseek(f,0,SEEK_END);
			filesize=ftell(f);
			fseek(f,0,SEEK_SET);

			kfstreambuffer=malloc(filesize);

			fread(kfstreambuffer,filesize,1,f);

			hxc_fclose(f);

			cellstream=(unsigned long*)malloc(filesize*sizeof(unsigned long));
			memset(cellstream,0,filesize*sizeof(unsigned long));


		#ifdef KFSTREAMDBG
			mck=((18432000 * 73) / 14) / 2;
			sck=mck/2;
			ick=mck/16;
		#endif

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

						cell_value = cell_value + overflowvalue;


						overflowvalue = 0;

						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
						totalcell = totalcell + cell_value;
						totalstreampos++;
					break;

					// Nop
					case 0x0A:
						offset=offset+3;
						totalstreampos++;
					break;
					case 0x09:
						offset=offset+2;
						totalstreampos++;
					break;
					case 0x08:
						offset=offset+1;
						totalstreampos++;
					break;
						//

					//0x0B 	Overflow16 	1 	Next cell value is increased by 0×10000 (16-bits). Decoding of *this* cell should continue at next stream position
					case 0x0B:
						overflowvalue = overflowvalue + 0x10000;
						offset++;
						totalstreampos++;
					break;

					//0x0C 	Value16 	3 	New cell value: Upper 8 bits are offset+1 in the stream, lower 8-bits are offset+2
					case 0x0C:
						offset++;
						cell_value = kfstreambuffer[offset] << 8;
						offset++;
						cell_value = cell_value | kfstreambuffer[offset];
						offset++;

						cell_value = cell_value + overflowvalue;
						overflowvalue=0;

						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
						totalcell = totalcell + cell_value;
						totalstreampos++;
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

								tabindex[nbindex].StreamPosition = diskIndex->StreamPosition;
								//tabindex[nbindex].CellPos = cellpos;
								tabindex[nbindex].SysClk = diskIndex->SysClk;
								tabindex[nbindex].Timer = diskIndex->Timer;
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
						if(!streamend)
							offset=offset+oob->Size + sizeof(s_oob_header);
						totalstreampos++;
					break;

					default:
						cell_value = kfstreambuffer[offset];

						cell_value = cell_value + overflowvalue;
						overflowvalue=0;

						cellstream[cellpos++]=(unsigned long)((float)cell_value * timecoef);
						totalcell = totalcell + cell_value;
						offset++;
						totalstreampos++;
					break;
				}


			}while( (offset<filesize) && !streamend);

			hxcfe_FxStream_setResolution(fxs,41619); // 41,619 ns per tick

			track_dump = hxcfe_FxStream_ImportStream(fxs,cellstream,32,cellpos);

			free(cellstream);
			free(kfstreambuffer);

			for(i=0;i<nbindex;i++)
			{
				hxcfe_FxStream_AddIndex(fxs,track_dump,tabindex[i].StreamPosition);
			}
		}
	}

	return track_dump;
}
