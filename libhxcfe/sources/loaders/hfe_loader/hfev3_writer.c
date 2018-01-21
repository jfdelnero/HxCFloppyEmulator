/*
//
// Copyright (C) 2006-2018 Jean-François DEL NERO
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "hfev3_loader.h"
#include "hfev3_format.h"

#include "hfev3_trackgen.h"


#include "libhxcadaptor.h"

typedef struct opcodetrack_
{
	unsigned char * opcodetrack_H0;
	unsigned char * opcodetrack_H1;
	unsigned char * randomopcodetrack;
	unsigned long tracklen;
}opcodetrack;

opcodetrack precalcopcodestracks[256];

static unsigned char bit_inverter[]=
{
	0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,
	0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
	0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,
	0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
	0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,
	0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
	0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,
	0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
	0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,
	0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
	0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,
	0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
	0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,
	0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
	0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,
	0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
	0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,
	0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
	0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,
	0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
	0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,
	0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
	0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,
	0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
	0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,
	0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
	0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,
	0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
	0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,
	0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
	0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,
	0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

static void addpad(unsigned char * track,int mfmsize,int tracksize)
{

	int i,j,lastindex;

	lastindex = tracksize;
	i=tracksize-1;
	while( i > 0 && !track[i] )
	{

		lastindex = i;
		i--;
	};

	if( lastindex &&  (lastindex < tracksize) )
	{
		j= lastindex;
		do
		{
			if( lastindex >= 2 )
				track[j++]=  NOP_OPCODE;

			if( j < tracksize )
				track[j++] = NOP_OPCODE;

		}while( j < tracksize );
	}
}

typedef struct RAMFILE_
{
	uint8_t * ramfile;
	int32_t ramfile_size;
}RAMFILE;

static FILE * rfopen(char* fn,char * mode,RAMFILE * rf)
{
	rf->ramfile=0;
	rf->ramfile_size=0;
	return (FILE *)1;
};

static int rfwrite(void * buffer,int size,int mul,FILE * file,RAMFILE * rf)
{
	rf->ramfile = realloc(rf->ramfile,rf->ramfile_size+size);
	memcpy(&rf->ramfile[rf->ramfile_size],buffer,size);
	rf->ramfile_size = rf->ramfile_size + size;
	return size;
}

static int rfclose(FILE *f,RAMFILE * rf)
{
	if(rf->ramfile)
		free(rf->ramfile);
	return 0;
};

int HFEV3_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	RAMFILE rf;
	pictrack * track;

	FILE * hxcpicfile;

	picfileformatheader * FILEHEADER;
	unsigned char * mfmtracks0,*mfmtracks1,*mfmtrackfinal;
	unsigned char * offsettrack;
	int mfmsize,mfmsize2;
	unsigned int i,j,k;
	unsigned int trackpos;
	unsigned int tracklistlen;
	unsigned int tracksize;
	unsigned int final_buffer_len;
	unsigned char * final_buffer_H0;
	unsigned char * final_buffer_H1;
	unsigned char * final_randombuffer;

	int tracklenarray[256];

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write HFE V3 file %s for the standalone emulator.",filename);

	if(!floppy->floppyNumberOfTrack)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create zero track HFE file");
		return -1;
	}

	hxcpicfile=rfopen(filename,"wb",&rf);

	if(hxcpicfile)
	{
		FILEHEADER=(picfileformatheader *) malloc(512);
		memset(FILEHEADER,0xFF,512);
		memcpy(&FILEHEADER->HEADERSIGNATURE,"HXCHFEV3",8);

		FILEHEADER->number_of_track=(unsigned char)floppy->floppyNumberOfTrack;
		FILEHEADER->number_of_side=floppy->floppyNumberOfSide;
		if(floppy->floppyBitRate!=VARIABLEBITRATE)
		{
			FILEHEADER->bitRate=(floppy->floppyBitRate)/1000;
		}
		else
		{
			if(floppy->tracks[0]->sides[0]->bitrate == VARIABLEBITRATE)
				FILEHEADER->bitRate=(unsigned short)((uint32_t)(floppy->tracks[0]->sides[0]->timingbuffer[ (floppy->tracks[0]->sides[0]->tracklen/8) / 2])/1000);
			else
				FILEHEADER->bitRate=(floppy->tracks[0]->sides[0]->bitrate)/1000;
		}
		FILEHEADER->floppyRPM=0;//floppy->floppyRPM;

		FILEHEADER->floppyinterfacemode=(unsigned char)floppy->floppyiftype;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Floppy interface mode %s (%s)",	hxcfe_getFloppyInterfaceModeName(imgldr_ctx->hxcfe,FILEHEADER->floppyinterfacemode),
																			hxcfe_getFloppyInterfaceModeDesc(imgldr_ctx->hxcfe,FILEHEADER->floppyinterfacemode) );

		FILEHEADER->track_encoding=0;
		FILEHEADER->formatrevision=0;
		FILEHEADER->track_list_offset=1;
		FILEHEADER->write_protected=1;

		if(floppy->tracks[floppy->floppyNumberOfTrack/2]->sides[0]->track_encoding)
		{
			FILEHEADER->track_encoding=floppy->tracks[floppy->floppyNumberOfTrack/2]->sides[0]->track_encoding;
		}

		if(floppy->floppyNumberOfTrack)
		{
			if( floppy->tracks[0]->number_of_side )
			{
				if(floppy->tracks[0]->sides[0]->track_encoding != FILEHEADER->track_encoding)
				{
					FILEHEADER->track0s0_altencoding = 0x00;
					FILEHEADER->track0s0_encoding = floppy->tracks[0]->sides[0]->track_encoding;
				}
			}

			if(floppy->tracks[0]->number_of_side == 2 )
			{
				if(floppy->tracks[0]->sides[1]->track_encoding != FILEHEADER->track_encoding)
				{
					FILEHEADER->track0s1_altencoding = 0x00;
					FILEHEADER->track0s1_encoding = floppy->tracks[0]->sides[1]->track_encoding;
				}
			}

		}

		if(floppy->double_step)
			FILEHEADER->single_step=0x00;
		else
			FILEHEADER->single_step=0xFF;

		rfwrite(FILEHEADER,512,1,hxcpicfile,&rf);

		tracklistlen=((((((FILEHEADER->number_of_track)+1)*sizeof(pictrack))/512)+1));
		offsettrack=(unsigned char*) malloc(tracklistlen*512);
		memset(offsettrack,0xFF,tracklistlen*512);

		i=0;
		trackpos=FILEHEADER->track_list_offset+tracklistlen;

		while(i<(FILEHEADER->number_of_track))
		{
				mfmsize=0;
				mfmsize2=0;

				final_buffer_len = GenOpcodesTrack(imgldr_ctx->hxcfe,
					floppy->tracks[i]->sides[0]->indexbuffer,
					floppy->tracks[i]->sides[0]->databuffer,
					floppy->tracks[i]->sides[0]->tracklen,
					floppy->tracks[i]->sides[1]->databuffer,
					floppy->tracks[i]->sides[1]->tracklen,
					floppy->tracks[i]->sides[0]->flakybitsbuffer,
					floppy->tracks[i]->sides[1]->flakybitsbuffer,
					floppy->tracks[i]->sides[0]->bitrate,
					floppy->tracks[i]->sides[0]->timingbuffer,
					floppy->tracks[i]->sides[1]->bitrate,
					floppy->tracks[i]->sides[1]->timingbuffer,
					&final_buffer_H0,
					&final_buffer_H1,
					&final_randombuffer);

				precalcopcodestracks[i].opcodetrack_H0 = (unsigned char *)malloc(final_buffer_len);
				precalcopcodestracks[i].opcodetrack_H1 = (unsigned char *)malloc(final_buffer_len);
				precalcopcodestracks[i].randomopcodetrack = (unsigned char *)malloc(final_buffer_len);
				precalcopcodestracks[i].tracklen = final_buffer_len;

				memcpy(precalcopcodestracks[i].opcodetrack_H0,final_buffer_H0,final_buffer_len);
				memcpy(precalcopcodestracks[i].opcodetrack_H1,final_buffer_H1,final_buffer_len);
				memcpy(precalcopcodestracks[i].randomopcodetrack,final_randombuffer,final_buffer_len);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 Track %d Size: %d bytes",i,final_buffer_len);

				if(final_randombuffer)
					free(final_randombuffer);
				if(final_buffer_H0)
					free(final_buffer_H0);
				if(final_buffer_H1)
					free(final_buffer_H1);

				/*
				mfmsize=floppy->tracks[i]->sides[0]->tracklen;
				if(mfmsize&7)
					mfmsize=(mfmsize/8)+1;
				else
					mfmsize=mfmsize/8;

				if(floppy->tracks[i]->number_of_side==2)
				{
					mfmsize2=floppy->tracks[i]->sides[1]->tracklen;
					if(mfmsize2&7)
						mfmsize2=(mfmsize2/8)+1;
					else
						mfmsize2=mfmsize2/8;
				}

				if(mfmsize2>mfmsize) mfmsize=mfmsize2;
*/
				mfmsize = final_buffer_len * 2;

				if(mfmsize>0xFFFF)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Argg!! track %d too long (%x) and shorten to 0xFFFF !",i,mfmsize*2);
					mfmsize=0xFFFF;
				}

				tracklenarray[i] = mfmsize / 2;

				track=(pictrack *)(offsettrack+(i*sizeof(pictrack)));
				track->track_len = mfmsize;
				track->offset = trackpos;

				if( ( mfmsize ) % 512 )
					trackpos=trackpos+(((mfmsize)/512)+1);
				else
					trackpos=trackpos+((mfmsize)/512);
			i++;
		};

		rfwrite(offsettrack,512*tracklistlen,1,hxcpicfile,&rf);

		i=0;
		while(i<(FILEHEADER->number_of_track))
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,(FILEHEADER->number_of_track) );

			track=(pictrack *)(offsettrack+(i*sizeof(pictrack)));

			if(track->track_len%512)
				tracksize=((track->track_len&(~0x1FF))+0x200)/2;//(((track->track_len/512)+1)*512)/2;
			else
				tracksize=track->track_len/2;

			mfmtracks0=(unsigned char*) malloc(tracksize);
			mfmtracks1=(unsigned char*) malloc(tracksize);
			mfmtrackfinal=(unsigned char*) malloc(tracksize*2);

			if( mfmtracks0 && mfmtracks1 && mfmtrackfinal)
			{
				memset(mfmtracks0,0x00,tracksize);
				memset(mfmtracks1,0x00,tracksize);
				memset(mfmtrackfinal,0x55,tracksize*2);

				memcpy(mfmtracks0,precalcopcodestracks[i].opcodetrack_H0,tracklenarray[i]);
				addpad(mfmtracks0,mfmsize,tracksize);

				if(floppy->tracks[i]->number_of_side==2)
				{
					memcpy(mfmtracks1,precalcopcodestracks[i].opcodetrack_H1,tracklenarray[i]);
					addpad(mfmtracks1,mfmsize2,tracksize);
				}

				for(k=0;k<tracksize/256;k++)
				{
					for(j=0;j<256;j++)
					{
						// inversion des bits pour le EUSART du PIC.

						// head 0
						mfmtrackfinal[(k*512)+j]=     bit_inverter[mfmtracks0[(k*256)+j]];

						// head 1
						mfmtrackfinal[(k*512)+j+256]= bit_inverter[mfmtracks1[(k*256)+j]];
					}
				}

				rfwrite(mfmtrackfinal,tracksize*2,1,hxcpicfile,&rf);

				free(mfmtracks0);
				free(mfmtracks1);
				free(mfmtrackfinal);
			}
			else
			{
				if( mfmtracks0 )
					free(mfmtracks0);

				if( mfmtracks1 )
					free(mfmtracks1);

				if( mfmtrackfinal )
					free(mfmtrackfinal);
			}

			i++;
		};

		free(offsettrack);


		hxcpicfile=hxc_fopen(filename,"wb");
		if(hxcpicfile)
		{
			fwrite(rf.ramfile,rf.ramfile_size,1,hxcpicfile);
			hxc_fclose(hxcpicfile);
		}
		else
		{
			rfclose(hxcpicfile,&rf);
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);
			return -1;
		}

		rfclose(hxcpicfile,&rf);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks written to the file",FILEHEADER->number_of_track);

		free(FILEHEADER);

		return 0;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);

		return -1;
	}

}
