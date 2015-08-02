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

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "trackutils.h"

HXCFE_SIDE * hxcfe_getSide( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track, int32_t side )
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

HXCFE_SIDE * hxcfe_duplicateSide( HXCFE* floppycontext, HXCFE_SIDE * side )
{
	HXCFE_SIDE * new_side;
	int track_wordsize;

	new_side = 0;

	if(side)
	{
		new_side = malloc(sizeof(HXCFE_SIDE));
		if( new_side )
		{
			memset( new_side, 0, sizeof(HXCFE_SIDE) );

			track_wordsize = side->tracklen / 8;
			if(side->tracklen&7)
				track_wordsize++;

			if( side->databuffer )
			{
				new_side->databuffer = malloc( track_wordsize );
				if( new_side->databuffer )
					memcpy(new_side->databuffer, side->databuffer,track_wordsize);
			}

			if( side->flakybitsbuffer )
			{
				new_side->flakybitsbuffer = malloc( track_wordsize );
				if( new_side->flakybitsbuffer )
					memcpy(new_side->flakybitsbuffer, side->flakybitsbuffer,track_wordsize);
			}

			if( side->indexbuffer )
			{
				new_side->indexbuffer = malloc( track_wordsize );
				if( new_side->indexbuffer )
					memcpy(new_side->indexbuffer, side->indexbuffer,track_wordsize);
			}

			if( side->timingbuffer )
			{
				new_side->timingbuffer = malloc( track_wordsize * sizeof(uint32_t) );
				if( new_side->timingbuffer )
					memcpy(new_side->timingbuffer, side->timingbuffer,track_wordsize * sizeof(uint32_t));
			}

			if( side->track_encoding_buffer )
			{
				new_side->track_encoding_buffer = malloc( track_wordsize );
				if( new_side->track_encoding_buffer )
					memcpy(new_side->track_encoding_buffer, side->track_encoding_buffer,track_wordsize );
			}
			
			if( side->track_encoding_buffer )
			{
				new_side->track_encoding_buffer = malloc( track_wordsize );
				if( new_side->track_encoding_buffer )
					memcpy(new_side->track_encoding_buffer, side->track_encoding_buffer,track_wordsize );
			}

			new_side->bitrate = side->bitrate;
			new_side->number_of_sector = side->number_of_sector;
			new_side->track_encoding = side->track_encoding;
			new_side->tracklen = side->tracklen;
		}
	}

	return new_side;
}

void hxcfe_freeSide( HXCFE* floppycontext, HXCFE_SIDE * side )
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

HXCFE_SIDE * hxcfe_replaceSide( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track_number, int32_t side_number, HXCFE_SIDE * side )
{
	HXCFE_SIDE * new_side;

	new_side = 0;

	if( fp && side)
	{
		if( ( track_number < fp->floppyNumberOfTrack ) && ( side_number < fp->floppyNumberOfSide ) )
		{
			if(fp->tracks)
			{
				if(fp->tracks[track_number]->sides && ( side_number < fp->tracks[track_number]->number_of_side) )
				{
					hxcfe_freeSide( floppycontext, fp->tracks[track_number]->sides[side_number] );

					fp->tracks[track_number]->sides[side_number] = hxcfe_duplicateSide( floppycontext, side );

					new_side = fp->tracks[track_number]->sides[side_number];
				}
			}
		}
	}

	return new_side;
}

int32_t hxcfe_shiftTrackData( HXCFE* floppycontext, HXCFE_SIDE * side, int32_t bitoffset )
{
	unsigned char * tmpbuffer;
	uint32_t * longtmpbuffer;
	int32_t i,j,oldptr,ptr;

	if(side)
	{

		if(!bitoffset)
		{
			return HXCFE_NOERROR;
		}

		tmpbuffer = malloc( (side->tracklen>>3) + 1 );
		if(tmpbuffer)
		{
			if(bitoffset<0)
				bitoffset = side->tracklen + bitoffset;

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

			if(side->timingbuffer)
			{
				longtmpbuffer = malloc( ((side->tracklen>>3) + 8) * sizeof(uint32_t) );
				if(longtmpbuffer)
				{
					oldptr = -1;
					for(i=0;i<side->tracklen;i++)
					{
						ptr = ( ( i + bitoffset ) % side->tracklen )>>3;
						if(oldptr!=ptr)
						{
							longtmpbuffer[ptr] = side->timingbuffer[(i%side->tracklen)>>3];
							oldptr=ptr;
						}
					}

					j=0;
					if(side->tracklen&7)
						j = 1;

					memcpy(side->timingbuffer,longtmpbuffer,((side->tracklen>>3) + j)*sizeof(uint32_t));

					free(longtmpbuffer);
				}
			}

			free(tmpbuffer);

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_rotateFloppy( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t bitoffset, int32_t total )
{
	int32_t i,j;
	double period,periodtoshift;
	int32_t offset;

	if(fp)
	{
		for(i=0;i<fp->floppyNumberOfTrack;i++)
		{
			for(j=0;j<fp->floppyNumberOfSide;j++)
			{
				period = GetTrackPeriod(floppycontext,fp->tracks[i]->sides[j]);
				periodtoshift = (period * (double)((double)bitoffset/(double)total)); //
				offset = fp->tracks[i]->sides[j]->tracklen - us2index(0,fp->tracks[i]->sides[j],(uint32_t)((double)(periodtoshift * 1000000)),0,0);

				hxcfe_shiftTrackData(floppycontext,fp->tracks[i]->sides[j],(offset&(~0x7)));
			}
		}
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_getCellState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber )
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

int32_t hxcfe_setCellState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, int32_t state )
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

int32_t hxcfe_removeCell( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, int32_t numberofcells )
{
	int32_t i;

	int32_t loopcnt;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			if( numberofcells < currentside->tracklen )
			{

				if( (cellnumber + numberofcells) < currentside->tracklen )
					loopcnt = currentside->tracklen - ( cellnumber + numberofcells);
				else
				{

					loopcnt = (currentside->tracklen - ( numberofcells ));
					currentside->tracklen = cellnumber;
					cellnumber = 0;
					numberofcells = currentside->tracklen - loopcnt;

				}

				for(i=0;i<loopcnt;i++)
				{
					setbit(currentside->databuffer, (cellnumber+i) ,getbit(currentside->databuffer,( cellnumber + numberofcells + i )));

					if(currentside->flakybitsbuffer)
					{
						setbit(currentside->flakybitsbuffer,(cellnumber+i),getbit(currentside->flakybitsbuffer,( cellnumber + numberofcells + i )));
					}

					if(currentside->indexbuffer)
					{
						setbit(currentside->indexbuffer,(cellnumber+i),getbit(currentside->indexbuffer,( cellnumber + numberofcells + i )));
					}

					if(currentside->timingbuffer)
					{
						currentside->timingbuffer[(cellnumber+i)/8] = currentside->timingbuffer[( cellnumber + numberofcells + i )/8];
					}

					if(currentside->track_encoding_buffer)
					{
						currentside->track_encoding_buffer[(cellnumber+i)/8] = currentside->track_encoding_buffer[( cellnumber + numberofcells + i )/8];
					}

				}

				currentside->tracklen = currentside->tracklen - numberofcells;

			}
			else
			{
				currentside->tracklen = 1;
			}

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_insertCell( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, int32_t state, int32_t numberofcells )
{
	int32_t i,tmpbufsize,oldbufsize;

	unsigned char * tmpbuffer;
	uint32_t * tmpulongbuffer;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{

			oldbufsize = currentside->tracklen / 8;
			if( currentside->tracklen & 7)
			{
				oldbufsize++;
			}

			tmpbufsize = ( currentside->tracklen + numberofcells ) / 8;
			if( ( currentside->tracklen + numberofcells ) & 7 )
			{
				tmpbufsize++;
			}

			tmpbuffer = malloc(tmpbufsize);
			if(tmpbuffer)
			{

				memcpy(tmpbuffer,currentside->databuffer,oldbufsize);

				for(i=cellnumber;i<cellnumber+numberofcells;i++)
				{
					setbit( tmpbuffer, i, state );
				}

				for(i=cellnumber;i<currentside->tracklen;i++)
				{
					setbit(tmpbuffer, i + numberofcells, getbit(currentside->databuffer,i) );
				}

				free( currentside->databuffer );

				currentside->databuffer = tmpbuffer;
			}

			if(currentside->flakybitsbuffer)
			{
				tmpbuffer = malloc(tmpbufsize);
				if(tmpbuffer)
				{

					memcpy(tmpbuffer,currentside->flakybitsbuffer,oldbufsize);

					for(i=cellnumber;i<cellnumber+numberofcells;i++)
					{
						setbit( tmpbuffer, i, 0 );
					}

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						setbit(tmpbuffer, i + numberofcells, getbit(currentside->flakybitsbuffer,i) );
					}

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

					for(i=cellnumber;i<cellnumber+numberofcells;i++)
					{
						setbit( tmpbuffer, i, 0 );
					}

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						setbit(tmpbuffer, i + numberofcells, getbit(currentside->indexbuffer,i) );
					}

					free(currentside->indexbuffer);

					currentside->indexbuffer = tmpbuffer;
				}
			}

			if(currentside->timingbuffer)
			{

				tmpulongbuffer = malloc(sizeof(uint32_t) * tmpbufsize);
				if(tmpulongbuffer)
				{

					memcpy(tmpulongbuffer,currentside->timingbuffer,sizeof(uint32_t) * oldbufsize);

					for(i=cellnumber;i<cellnumber+numberofcells;i++)
					{
						tmpulongbuffer[i/8] = currentside->timingbuffer[cellnumber/8];
					}

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						tmpulongbuffer[( i + numberofcells ) / 8] =  currentside->timingbuffer[i/8];
					}

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

					for(i=cellnumber;i<cellnumber+numberofcells;i++)
					{
						tmpbuffer[i/8] = currentside->track_encoding_buffer[cellnumber/8];
					}

					for(i=cellnumber;i<currentside->tracklen;i++)
					{
						tmpbuffer[ ( i + numberofcells ) / 8] =  tmpbuffer[i/8];
					}

					free(currentside->track_encoding_buffer);

					currentside->track_encoding_buffer = tmpbuffer;
				}
			}

			currentside->tracklen += numberofcells;

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_getCellFlakeyState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber )
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

int32_t hxcfe_setCellFlakeyState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, int32_t state )
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

int32_t hxcfe_getCellIndexState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber )
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

int32_t hxcfe_setCellIndexState( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, int32_t state )
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

int32_t hxcfe_getCellBitrate( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber )
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

int32_t hxcfe_setCellBitrate( HXCFE* floppycontext, HXCFE_SIDE * currentside, int32_t cellnumber, uint32_t bitrate, int32_t numberofcells )
{
	int tmpbufsize;
	int32_t i,j;

	if(currentside && floppycontext)
	{
		if(cellnumber < currentside->tracklen)
		{
			if(currentside->timingbuffer)
			{
				for(j=0;j<numberofcells;j++)
				{
					currentside->timingbuffer[((cellnumber+j)%currentside->tracklen)/8] = bitrate;
				}
			}
			else
			{
				tmpbufsize = ( currentside->tracklen ) / 8;
				if( ( currentside->tracklen ) & 7 )
				{
					tmpbufsize++;
				}

				currentside->timingbuffer = malloc( tmpbufsize * sizeof(uint32_t) );
				if( currentside->timingbuffer )
				{
					for(i=0;i<currentside->tracklen;i++)
					{
						currentside->timingbuffer[i/8] = currentside->bitrate;
					}

					for(j=0;j<numberofcells;j++)
					{
						currentside->timingbuffer[((cellnumber+j)%currentside->tracklen)/8] = bitrate;
					}

					currentside->bitrate = VARIABLEBITRATE;
				}

			}

			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}


int32_t hxcfe_setTrackRPM( HXCFE* floppycontext, HXCFE_SIDE * side, int32_t rpm )
{
	double period_s0,period_s1;
	int32_t i;
	int32_t bitrate;

	if(side)
	{
		period_s0 = GetTrackPeriod(floppycontext,side);
		period_s1 = (double)1/(double)((double)rpm/(double)60);

		for(i=0;i<side->tracklen;i++)
		{
			if(!(i&7))
			{
				bitrate = hxcfe_getCellBitrate( floppycontext, side, i );

				bitrate = (int32_t)((double)bitrate * (double)(period_s0/period_s1));
			}

			hxcfe_setCellBitrate( floppycontext, side, i, bitrate, 1 );
		}

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_removeOddTracks( HXCFE* floppycontext, HXCFE_FLOPPY * fp)
{
	int32_t i,tracknum;

	if(fp)
	{
		for(i=0;i<fp->floppyNumberOfTrack;i++)
		{
			if(i&1)
			{
				if(fp->tracks[i]->number_of_side>0)
				{
					hxcfe_freeSide( floppycontext, fp->tracks[i]->sides[0] );
				}

				if(fp->tracks[i]->number_of_side>1)
				{
					hxcfe_freeSide( floppycontext, fp->tracks[i]->sides[1] );
				}

				free(fp->tracks[i]);

				fp->tracks[i] = 0;
			}
		}

		tracknum = 0;
		for(i=0;i<fp->floppyNumberOfTrack;i=i+2)
		{
			fp->tracks[i/2] = fp->tracks[i];
				tracknum++;
		}
		fp->floppyNumberOfTrack = tracknum;

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}


int32_t hxcfe_removeLastTrack( HXCFE* floppycontext, HXCFE_FLOPPY * fp)
{
	if(fp)
	{
		if(fp->floppyNumberOfTrack)
		{
			if(fp->tracks[fp->floppyNumberOfTrack-1]->number_of_side>0)
			{
				hxcfe_freeSide( floppycontext, fp->tracks[fp->floppyNumberOfTrack-1]->sides[0] );
			}

			if(fp->tracks[fp->floppyNumberOfTrack-1]->number_of_side>1)
			{
				hxcfe_freeSide( floppycontext, fp->tracks[fp->floppyNumberOfTrack-1]->sides[1] );
			}

			free(fp->tracks[fp->floppyNumberOfTrack-1]);

			fp->floppyNumberOfTrack--;
		}

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_addTrack( HXCFE* floppycontext, HXCFE_FLOPPY * fp, uint32_t bitrate, int32_t rpm )
{
	HXCFE_CYLINDER ** oldcylinderarray;
	HXCFE_CYLINDER ** newcylinderarray;
	int i;

	if(fp)
	{
		if(fp->floppyNumberOfTrack<256)
		{

			fp->floppyNumberOfTrack++;

			oldcylinderarray = fp->tracks;

			newcylinderarray = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*(fp->floppyNumberOfTrack));
			if(newcylinderarray)
			{
				memcpy(newcylinderarray,oldcylinderarray,sizeof(HXCFE_CYLINDER*)*(fp->floppyNumberOfTrack-1));

				newcylinderarray[fp->floppyNumberOfTrack-1] = allocCylinderEntry(rpm,fp->floppyNumberOfSide);

				for(i=0;i<fp->floppyNumberOfSide;i++)
				{
					newcylinderarray[fp->floppyNumberOfTrack-1]->sides[i] = tg_generateTrack(0,512,0,0,0,1,1,0,bitrate,rpm,ISOFORMAT_DD,34,0,2500,-2500);
				}

				if(newcylinderarray[fp->floppyNumberOfTrack-1]->sides[0])
				{
					fp->tracks = newcylinderarray;

					free(oldcylinderarray);
				}
				else
				{
					free(newcylinderarray[fp->floppyNumberOfTrack-1]->sides);
					free(newcylinderarray[fp->floppyNumberOfTrack-1]);
					free(newcylinderarray);

					fp->floppyNumberOfTrack--;

					return HXCFE_BADPARAMETER;
				}

			}

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADPARAMETER;
}

const unsigned short test_pattern_array[]=
{
	0x5555,0x9555,0x2555,0xA555,0x4955,0x8955,0x2955,0xA955,
	0x5255,0x9255,0x2255,0xA255,0x4A55,0x8A55,0x2A55,0xAA55,
	0x5495,0x9495,0x2495,0xA495,0x4895,0x8895,0x2895,0xA895,
	0x5295,0x9295,0x2295,0xA295,0x4A95,0x8A95,0x2A95,0xAA95,
	0x5525,0x9525,0x2525,0xA525,0x4925,0x8925,0x2925,0xA925,
	0x5225,0x9225,0x2225,0xA225,0x4A25,0x8A25,0x2A25,0xAA25,
	0x54A5,0x94A5,0x24A5,0xA4A5,0x48A5,0x88A5,0x28A5,0xA8A5,
	0x52A5,0x92A5,0x22A5,0xA2A5,0x4AA5,0x8AA5,0x2AA5,0xAAA5,
	0x5549,0x9549,0x2549,0xA549,0x4949,0x8949,0x2949,0xA949,
	0x5249,0x9249,0x2249,0xA249,0x4A49,0x8A49,0x2A49,0xAA49,
	0x5489,0x9489,0x2489,0xA489,0x4889,0x8889,0x2889,0xA889,
	0x5289,0x9289,0x2289,0xA289,0x4A89,0x8A89,0x2A89,0xAA89,
	0x5529,0x9529,0x2529,0xA529,0x4929,0x8929,0x2929,0xA929,
	0x5229,0x9229,0x2229,0xA229,0x4A29,0x8A29,0x2A29,0xAA29,
	0x54A9,0x94A9,0x24A9,0xA4A9,0x48A9,0x88A9,0x28A9,0xA8A9,
	0x52A9,0x92A9,0x22A9,0xA2A9,0x4AA9,0x8AA9,0x2AA9,0xAAA9,
	0x5552,0x9552,0x2552,0xA552,0x4952,0x8952,0x2952,0xA952,
	0x5252,0x9252,0x2252,0xA252,0x4A52,0x8A52,0x2A52,0xAA52,
	0x5492,0x9492,0x2492,0xA492,0x4892,0x8892,0x2892,0xA892,
	0x5292,0x9292,0x2292,0xA292,0x4A92,0x8A92,0x2A92,0xAA92,
	0x5522,0x9522,0x2522,0xA522,0x4922,0x8922,0x2922,0xA922,
	0x5222,0x9222,0x2222,0xA222,0x4A22,0x8A22,0x2A22,0xAA22,
	0x54A2,0x94A2,0x24A2,0xA4A2,0x48A2,0x88A2,0x28A2,0xA8A2,
	0x52A2,0x92A2,0x22A2,0xA2A2,0x4AA2,0x8AA2,0x2AA2,0xAAA2,
	0x554A,0x954A,0x254A,0xA54A,0x494A,0x894A,0x294A,0xA94A,
	0x524A,0x924A,0x224A,0xA24A,0x4A4A,0x8A4A,0x2A4A,0xAA4A,
	0x548A,0x948A,0x248A,0xA48A,0x488A,0x888A,0x288A,0xA88A,
	0x528A,0x928A,0x228A,0xA28A,0x4A8A,0x8A8A,0x2A8A,0xAA8A,
	0x552A,0x952A,0x252A,0xA52A,0x492A,0x892A,0x292A,0xA92A,
	0x522A,0x922A,0x222A,0xA22A,0x4A2A,0x8A2A,0x2A2A,0xAA2A,
	0x54AA,0x94AA,0x24AA,0xA4AA,0x48AA,0x88AA,0x28AA,0xA8AA,
	0x52AA,0x92AA,0x22AA,0xA2AA,0x4AAA,0x8AAA,0x2AAA,0xAAAA,
};


uint32_t gettestpattern(unsigned long patternvalue)
{
	uint32_t pattern;

	pattern =  test_pattern_array[ patternvalue & 0xFF ] << 16;
	pattern |= test_pattern_array[ (patternvalue>>8) & 0xFF ];

	if( pattern & 0x00010000 )
		pattern = pattern & ~0x00008000;

	return pattern;
}

int32_t hxcfe_localRepair( HXCFE* floppycontext, HXCFE_FLOPPY *fp, int32_t track, int32_t side, int32_t start_cellnumber, int32_t numberofcells )
{
	int32_t i,test_pattern_size;
	uint32_t pattern,loop,old_pattern;
	HXCFE_SIDE * currentside;
	HXCFE_SECTORACCESS * sa;
	HXCFE_SECTCFG** sc_list;
	HXCFE_SECTCFG* sc;
	int32_t nb_sectorfound,sectorpos,good_pattern;

	int32_t tab[5]={0,1,1,-3,-1};
	uint32_t test_pattern;

	good_pattern = 0;
	if(fp && floppycontext)
	{
		if( ( track < fp->floppyNumberOfTrack ) && ( side < fp->floppyNumberOfSide ) )
		{
			nb_sectorfound = 0;

			currentside = fp->tracks[track]->sides[side];

			sa = hxcfe_initSectorAccess( floppycontext, fp );
			sc_list = hxcfe_getAllTrackISOSectors( sa, track, side, &nb_sectorfound );

			sectorpos = -1;
			if(nb_sectorfound)
			{
				i = 0;
				while(i < nb_sectorfound)
				{
					if( ( start_cellnumber >= sc_list[i]->startsectorindex ) && ( ((start_cellnumber + numberofcells )%currentside->tracklen ) < sc_list[i]->endsectorindex ) )
					{
						sectorpos = i;
					}
					i++;
				}
			}

			if(sectorpos>=0)
			{
				test_pattern_size = numberofcells;

				if( numberofcells && ( test_pattern_size < 28 ) )
				{
					// Back up...
					old_pattern = 0x00000000;
					for(i = 0; i < test_pattern_size;i++)
					{
						if(hxcfe_getCellState( floppycontext, currentside, (start_cellnumber + i) % currentside->tracklen))
						{
							old_pattern = old_pattern | (0x00000001 << i);
						}
					}

					loop = 0;
					do
					{
						if(tab[loop]>0)
						{
							hxcfe_insertCell( floppycontext, currentside, start_cellnumber, 0,  tab[loop] );
							test_pattern_size += tab[loop];
							
						}
						else
						{
							if(tab[loop]<0)
							{
								if( test_pattern_size > 0 )
								{
									hxcfe_removeCell( floppycontext, currentside, start_cellnumber, -tab[loop] );
									test_pattern_size += tab[loop];
								}
							}
						}

						if( test_pattern_size > 0 )
						{
							pattern = 0x00000000;
							do
							{
								test_pattern = gettestpattern(pattern);

								for(i = 0; i < test_pattern_size;i++)
								{
									if( test_pattern & (0x80000000 >> i) )
									{
										hxcfe_setCellState( floppycontext, currentside, (start_cellnumber + i) % currentside->tracklen, 1 );
									}
									else
									{
										hxcfe_setCellState( floppycontext, currentside, (start_cellnumber + i) % currentside->tracklen, 0 );
									}
								}


								// Test Sector State...
								hxcfe_resetSearchTrackPosition( sa );

								sc = hxcfe_searchSector ( sa, track, side, sc_list[sectorpos]->sector, ISOIBM_MFM_ENCODING );
								if( sc )
								{
									if(!sc->use_alternate_data_crc)
									{
										good_pattern = 1;
									}
									hxcfe_freeSectorConfig( sa, sc );
								}
								
								pattern++;
							}while( (pattern & ((2<<(test_pattern_size-1))-1)) && !good_pattern);
						}

						loop++;
					}while( ( loop < 5 ) && ( test_pattern_size > 0 ) && !good_pattern);

					// Not found... restore the old state
					if(!good_pattern)
					{
						hxcfe_insertCell( floppycontext, currentside, start_cellnumber, 0,  2 );
						test_pattern_size += 2;
						for(i = 0; i < test_pattern_size;i++)
						{
							if( old_pattern & (0x00000001 << i) )
							{
								hxcfe_setCellState( floppycontext, currentside, (start_cellnumber + i) % currentside->tracklen, 1 );
							}
							else
							{
								hxcfe_setCellState( floppycontext, currentside, (start_cellnumber + i) % currentside->tracklen, 0 );
							}
						}
					}
				}
			}

			if(sc_list)
			{
				i = 0;
				while( i < nb_sectorfound )
				{

					hxcfe_freeSectorConfig( sa, sc_list[i] );
					i++;
				}
				free(sc_list);
			}
			hxcfe_deinitSectorAccess( sa );

			if(!good_pattern)
				return HXCFE_NOERROR;
			else
				return 1; // Repaired...

		}	
	}
	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_sectorRepair( HXCFE* floppycontext, HXCFE_FLOPPY *fp, int32_t track, int32_t side, int32_t start_cellnumber )
{
	int32_t i,retrep;
	HXCFE_SIDE * currentside;
	HXCFE_SECTORACCESS * sa;
	HXCFE_SECTCFG** sc_list;
	int32_t nb_sectorfound,sectorpos,cellnumber;

	if(fp && floppycontext)
	{
		if( ( track < fp->floppyNumberOfTrack ) && ( side < fp->floppyNumberOfSide ) )
		{
			sa = hxcfe_initSectorAccess( floppycontext, fp );
			sc_list = hxcfe_getAllTrackISOSectors( sa, track, side, &nb_sectorfound );

			currentside = fp->tracks[track]->sides[side];

			sectorpos = -1;
			if(nb_sectorfound)
			{
				i = 0;
				while(i < nb_sectorfound)
				{
					if( ( start_cellnumber >= sc_list[i]->startsectorindex ) && ( (start_cellnumber%currentside->tracklen ) < sc_list[i]->endsectorindex ) )
					{
						sectorpos = i;
					}
					i++;
				}
			}

			if(sectorpos>=0)
			{

				cellnumber = ( sc_list[sectorpos]->startdataindex + (4*8*2) );
				do
				{
					retrep = hxcfe_localRepair( floppycontext, fp, track, side, cellnumber, 10 );
					cellnumber = cellnumber + 8;
				}while(cellnumber < ( sc_list[sectorpos]->endsectorindex - 16 ) && !retrep);
			}

			if(sc_list)
			{
				i = 0;
				while( i < nb_sectorfound )
				{

					hxcfe_freeSectorConfig( sa, sc_list[i] );
					i++;
				}
				free(sc_list);
			}
			hxcfe_deinitSectorAccess( sa );

			if(!retrep)
				return HXCFE_NOERROR;
			else
				return 1; // Repaired...
		}
	}
	
	return HXCFE_BADPARAMETER;
}