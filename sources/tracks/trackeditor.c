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
// File : trackeditor.c
// Contains:
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libhxcfe.h"
#include "trackutils.h"

SIDE * hxcfe_getSide(FLOPPY * fp,int track,int side)
{
	if(fp)
	{
		if( ( track < fp->floppyNumberOfTrack ) && ( side < fp->floppyNumberOfSide ) )
		{
			if(fp->tracks)
			{
				if(fp->tracks[track]->sides && ( side < fp->tracks[track]->number_of_side) )
				{
					return fp->tracks[track]->sides[side];
				}
			}
		}
	}

	return 0;
}

void hxcfe_freeSide(SIDE * side)
{
	if(side)
	{
		if(side->databuffer)
		{
			free(side->databuffer);
			side->databuffer=0;
		}
		if(side->flakybitsbuffer)
		{
			free(side->flakybitsbuffer);
			side->flakybitsbuffer=0;
		}
		if(side->indexbuffer)
		{
			free(side->indexbuffer);
			side->indexbuffer=0;
		}
		if(side->timingbuffer)
		{
			free(side->timingbuffer);
			side->timingbuffer=0;
		}

		if(side->track_encoding_buffer)
		{
			free(side->track_encoding_buffer);
			side->track_encoding_buffer=0;
		}

		free(side);
	}

	return;
}

int hxcfe_shiftTrackData(SIDE * side,long bitoffset)
{
	unsigned char * tmpbuffer;
	unsigned long i,j;

	if(side)
	{
		tmpbuffer = malloc( (side->tracklen>>3) + 1 );
		if(tmpbuffer)
		{
			memset(tmpbuffer,0,(side->tracklen>>3) + 1);

			for(i=0;i<side->tracklen;i++)
			{
				setbit(tmpbuffer,((i+bitoffset)%side->tracklen),getbit(side->databuffer,i));
			}

			j=0;
			if(side->tracklen&7)
				j = 1;

			memcpy(side->databuffer,tmpbuffer,(side->tracklen>>3) + j);

			////////////////////////////////////////////////////////////

			if(side->flakybitsbuffer)
			{
				memset(tmpbuffer,0,(side->tracklen>>3) + 1);

				for(i=0;i<side->tracklen;i++)
				{
					setbit(tmpbuffer,((i+bitoffset)%side->tracklen),getbit(side->flakybitsbuffer,i));
				}

				j=0;
				if(side->tracklen&7)
					j = 1;

				memcpy(side->flakybitsbuffer,tmpbuffer,(side->tracklen>>3) + j);
			}

			////////////////////////////////////////////////////////////

			free(tmpbuffer);

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int  hxcfe_getCellState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen && currentside->databuffer)
		{
			return getbit(currentside->databuffer,cellnumber);
		}
	}
	return HXCFE_BADPARAMETER;
}

int hxcfe_setCellState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber,int state)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen && currentside->databuffer)
		{
			setbit(currentside->databuffer,cellnumber,state);
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}

int  hxcfe_removeCell(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber)
{
	unsigned long i;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			for(i=cellnumber + 1;i<currentside->tracklen;i++)
			{
				setbit(currentside->databuffer,i-1,getbit(currentside->databuffer,i));

				if(currentside->flakybitsbuffer)
				{
					setbit(currentside->flakybitsbuffer,i-1,getbit(currentside->flakybitsbuffer,i));
				}

				if(currentside->indexbuffer)
				{
					setbit(currentside->indexbuffer,i-1,getbit(currentside->indexbuffer,i));
				}

				if(currentside->timingbuffer)
				{
					currentside->timingbuffer[(i-1)/8] = currentside->timingbuffer[i/8];
				}

				if(currentside->track_encoding_buffer)
				{
					currentside->track_encoding_buffer[(i-1)/8] = currentside->track_encoding_buffer[i/8];
				}

			}

			currentside->tracklen--;

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int  hxcfe_insertCell(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber,int state)
{
	unsigned long i,tmpbufsize,oldbufsize;

	unsigned char * tmpbuffer;
	unsigned long * tmpulongbuffer;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{

			oldbufsize = currentside->tracklen / 8;
			if( currentside->tracklen & 7)
			{
				oldbufsize++;
			}

			tmpbufsize = ( currentside->tracklen + 1 ) / 8;
			if( ( currentside->tracklen + 1 ) & 7 )
			{
				tmpbufsize++;
			}

			tmpbuffer = malloc(tmpbufsize);
			if(tmpbuffer)
			{

				memcpy(tmpbuffer,currentside->databuffer,oldbufsize);

				for(i=cellnumber;i<currentside->tracklen;i++)
				{
					setbit(tmpbuffer,i+1,getbit(tmpbuffer,i));
				}

				setbit(tmpbuffer,cellnumber,state);

				free(currentside->databuffer);

				currentside->databuffer = tmpbuffer;
			}

			if(currentside->flakybitsbuffer)
			{
				tmpbuffer = malloc(tmpbufsize);
				if(tmpbuffer)
				{

					memcpy(tmpbuffer,currentside->flakybitsbuffer,oldbufsize);

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						setbit(tmpbuffer,i+1,getbit(tmpbuffer,i));
					}

					setbit(tmpbuffer,cellnumber,0);

					free(currentside->flakybitsbuffer);

					currentside->flakybitsbuffer = tmpbuffer;
				}
			}

			if(currentside->indexbuffer)
			{
				tmpbuffer = malloc(tmpbufsize);
				if(tmpbuffer)
				{

					memcpy(tmpbuffer,currentside->indexbuffer,oldbufsize);

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						setbit(tmpbuffer,i+1,getbit(tmpbuffer,i));
					}

					setbit(tmpbuffer,cellnumber,getbit(currentside->indexbuffer,cellnumber));

					free(currentside->indexbuffer);

					currentside->indexbuffer = tmpbuffer;
				}
			}

			if(currentside->timingbuffer)
			{

				tmpulongbuffer = malloc(sizeof(unsigned long) * tmpbufsize);
				if(tmpulongbuffer)
				{

					memcpy(tmpulongbuffer,currentside->timingbuffer,sizeof(unsigned long) * oldbufsize);

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						tmpulongbuffer[(i+1)/8] =  tmpulongbuffer[i/8];
					}

					tmpulongbuffer[cellnumber/8] = currentside->indexbuffer[cellnumber/8];

					free(currentside->timingbuffer);

					currentside->timingbuffer = tmpulongbuffer;
				}
			}

			if(currentside->track_encoding_buffer)
			{

				tmpbuffer = malloc(tmpbufsize);
				if(tmpbuffer)
				{

					memcpy(tmpbuffer,currentside->track_encoding_buffer,oldbufsize);

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						tmpbuffer[(i+1)/8] =  tmpbuffer[i/8];
					}

					tmpbuffer[cellnumber/8] = currentside->track_encoding_buffer[cellnumber/8];

					free(currentside->track_encoding_buffer);

					currentside->track_encoding_buffer = tmpbuffer;
				}
			}

			currentside->tracklen++;

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int  hxcfe_getCellFlakeyState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen && currentside->flakybitsbuffer)
		{
			return getbit(currentside->flakybitsbuffer,cellnumber);
		}
	}
	return HXCFE_BADPARAMETER;
}

int hxcfe_setCellFlakeyState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber,int state)
{
	int tmpbufsize;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			if(currentside->flakybitsbuffer)
			{
				setbit( currentside->flakybitsbuffer, cellnumber, state );
			}
			else
			{
				tmpbufsize = ( currentside->tracklen ) / 8;
				if( ( currentside->tracklen ) & 7 )
				{
					tmpbufsize++;
				}

				currentside->flakybitsbuffer = malloc( tmpbufsize );
				if( currentside->flakybitsbuffer )
				{
					memset( currentside->flakybitsbuffer, 0, tmpbufsize );
					setbit( currentside->flakybitsbuffer, cellnumber, state );
				}

			}
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}

int  hxcfe_getCellIndexState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen && currentside->indexbuffer)
		{
			return getbit(currentside->indexbuffer,cellnumber);
		}
	}
	return HXCFE_BADPARAMETER;
}

int hxcfe_setCellIndexState(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber,int state)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen && currentside->indexbuffer)
		{
			setbit(currentside->indexbuffer,cellnumber,state);
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}

int  hxcfe_getCellBitrate(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber)
{
	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			if(currentside->timingbuffer)
			{
				return currentside->timingbuffer[cellnumber/8];
			}
			else
			{
				return currentside->bitrate;
			}
		}
	}
	return HXCFE_BADPARAMETER;
}

int hxcfe_setCellBitrate(HXCFLOPPYEMULATOR* floppycontext,SIDE * currentside,unsigned long cellnumber,unsigned long bitrate)
{
	int tmpbufsize;
	unsigned long i;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			if(currentside->timingbuffer)
			{
				currentside->timingbuffer[cellnumber/8] = bitrate;
			}
			else
			{
				tmpbufsize = ( currentside->tracklen ) / 8;
				if( ( currentside->tracklen ) & 7 )
				{
					tmpbufsize++;
				}

				currentside->flakybitsbuffer = malloc( tmpbufsize * sizeof(unsigned long) );
				if( currentside->timingbuffer )
				{
					for(i=0;i<currentside->tracklen;i++)
					{
						currentside->timingbuffer[i/8] = currentside->bitrate;
					}

					currentside->timingbuffer[cellnumber/8] = bitrate;
				}
			}

			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}
