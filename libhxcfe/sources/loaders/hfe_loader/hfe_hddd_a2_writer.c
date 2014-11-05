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

#define FASTWRITE 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "hfe_loader.h"
#include "hfe_format.h"

#include "libhxcadaptor.h"

unsigned char * ramfile;
int ramfile_size;

extern unsigned char bit_inverter[];
extern unsigned short bit_expander[];

unsigned short ext_a2_bit_expander[]=
{
	0x0000,0x0002,0x0008,0x000a,0x0020,0x0022,0x0028,0x002a,
	0x0080,0x0082,0x0088,0x008a,0x00a0,0x00a2,0x00a8,0x00aa,
	0x0200,0x0202,0x0208,0x020a,0x0220,0x0222,0x0228,0x022a,
	0x0280,0x0282,0x0288,0x028a,0x02a0,0x02a2,0x02a8,0x02aa,
	0x0800,0x0802,0x0808,0x080a,0x0820,0x0822,0x0828,0x082a,
	0x0880,0x0882,0x0888,0x088a,0x08a0,0x08a2,0x08a8,0x08aa,
	0x0a00,0x0a02,0x0a08,0x0a0a,0x0a20,0x0a22,0x0a28,0x0a2a,
	0x0a80,0x0a82,0x0a88,0x0a8a,0x0aa0,0x0aa2,0x0aa8,0x0aaa,
	0x2000,0x2002,0x2008,0x200a,0x2020,0x2022,0x2028,0x202a,
	0x2080,0x2082,0x2088,0x208a,0x20a0,0x20a2,0x20a8,0x20aa,
	0x2200,0x2202,0x2208,0x220a,0x2220,0x2222,0x2228,0x222a,
	0x2280,0x2282,0x2288,0x228a,0x22a0,0x22a2,0x22a8,0x22aa,
	0x2800,0x2802,0x2808,0x280a,0x2820,0x2822,0x2828,0x282a,
	0x2880,0x2882,0x2888,0x288a,0x28a0,0x28a2,0x28a8,0x28aa,
	0x2a00,0x2a02,0x2a08,0x2a0a,0x2a20,0x2a22,0x2a28,0x2a2a,
	0x2a80,0x2a82,0x2a88,0x2a8a,0x2aa0,0x2aa2,0x2aa8,0x2aaa,
	0x8000,0x8002,0x8008,0x800a,0x8020,0x8022,0x8028,0x802a,
	0x8080,0x8082,0x8088,0x808a,0x80a0,0x80a2,0x80a8,0x80aa,
	0x8200,0x8202,0x8208,0x820a,0x8220,0x8222,0x8228,0x822a,
	0x8280,0x8282,0x8288,0x828a,0x82a0,0x82a2,0x82a8,0x82aa,
	0x8800,0x8802,0x8808,0x880a,0x8820,0x8822,0x8828,0x882a,
	0x8880,0x8882,0x8888,0x888a,0x88a0,0x88a2,0x88a8,0x88aa,
	0x8a00,0x8a02,0x8a08,0x8a0a,0x8a20,0x8a22,0x8a28,0x8a2a,
	0x8a80,0x8a82,0x8a88,0x8a8a,0x8aa0,0x8aa2,0x8aa8,0x8aaa,
	0xa000,0xa002,0xa008,0xa00a,0xa020,0xa022,0xa028,0xa02a,
	0xa080,0xa082,0xa088,0xa08a,0xa0a0,0xa0a2,0xa0a8,0xa0aa,
	0xa200,0xa202,0xa208,0xa20a,0xa220,0xa222,0xa228,0xa22a,
	0xa280,0xa282,0xa288,0xa28a,0xa2a0,0xa2a2,0xa2a8,0xa2aa,
	0xa800,0xa802,0xa808,0xa80a,0xa820,0xa822,0xa828,0xa82a,
	0xa880,0xa882,0xa888,0xa88a,0xa8a0,0xa8a2,0xa8a8,0xa8aa,
	0xaa00,0xaa02,0xaa08,0xaa0a,0xaa20,0xaa22,0xaa28,0xaa2a,
	0xaa80,0xaa82,0xaa88,0xaa8a,0xaaa0,0xaaa2,0xaaa8,0xaaaa
};


extern void addpad(unsigned char * track,int mfmsize,int tracksize);
extern FILE * rfopen(char* fn,char * mode);
extern int rfwrite(void * buffer,int size,int mul,FILE * file);
extern int rfclose(FILE *f);

int HFE_HDDD_A2_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{

	pictrack * track;

	FILE * hxcpicfile;

	picfileformatheader * FILEHEADER;
	unsigned char * mfmtracks0,*mfmtracks1,*mfmtrackfinal;
	unsigned char * offsettrack;
	int mfmsize,mfmsize2,i_conv;
	unsigned int i,j,k;
	unsigned int trackpos;
	unsigned int tracklistlen;
	unsigned int tracksize;

	unsigned short fm_pulses;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write HFE file %s for the standalone emulator (with HDDD A2 support).",filename);

	if(!floppy->floppyNumberOfTrack)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create zero track HFE file");
		return -1;
	}

	ramfile=0;
	ramfile_size=0;

	hxcpicfile=rfopen(filename,"wb");

	if(hxcpicfile)
	{
		FILEHEADER=(picfileformatheader *) malloc(512);
		memset(FILEHEADER,0xFF,512);
		memcpy(&FILEHEADER->HEADERSIGNATURE,"HXCPICFE",8);

		FILEHEADER->number_of_track=(unsigned char)floppy->floppyNumberOfTrack;
		FILEHEADER->number_of_side=floppy->floppyNumberOfSide;
		if(floppy->floppyBitRate!=VARIABLEBITRATE)
		{
			FILEHEADER->bitRate=(floppy->floppyBitRate * 2)/1000;
		}
		else
		{
			if(floppy->tracks[0]->sides[0]->bitrate == VARIABLEBITRATE)
				FILEHEADER->bitRate=(unsigned short)((unsigned long)(floppy->tracks[0]->sides[0]->timingbuffer[ (floppy->tracks[0]->sides[0]->tracklen/8) / 2] * 2)/1000);
			else
				FILEHEADER->bitRate=(floppy->tracks[0]->sides[0]->bitrate * 2)/1000;
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

		if( FILEHEADER->track_encoding == APPLEII_GCR1_ENCODING )
		{
			FILEHEADER->track_encoding = APPLEII_HDDD_A2_GCR1_ENCODING;
		}

		if( FILEHEADER->track_encoding == APPLEII_GCR2_ENCODING )
		{
			FILEHEADER->track_encoding = APPLEII_HDDD_A2_GCR2_ENCODING;
		}

		if( floppy->floppyNumberOfTrack	> 42 )
		{
			FILEHEADER->single_step=0xFF;
		}
		else
		{
			FILEHEADER->single_step=0x00;
		}

		rfwrite(FILEHEADER,512,1,hxcpicfile);

		tracklistlen=((((((FILEHEADER->number_of_track)+1)*sizeof(pictrack))/512)+1));
		offsettrack=(unsigned char*) malloc(tracklistlen*512);
		memset(offsettrack,0xFF,tracklistlen*512);

		i=0;
		trackpos=FILEHEADER->track_list_offset+tracklistlen;

		while(i<(FILEHEADER->number_of_track))
		{
				hxcfe_imgCallProgressCallback(imgldr_ctx,i,(FILEHEADER->number_of_track) );

				mfmsize=0;
				mfmsize2=0;

				mfmsize=floppy->tracks[i]->sides[0]->tracklen * 2;
				if(mfmsize&7)
					mfmsize=(mfmsize/8)+1;
				else
					mfmsize=mfmsize/8;


				if(floppy->tracks[i]->number_of_side==2)
				{
					mfmsize2=floppy->tracks[i]->sides[1]->tracklen * 2;
					if(mfmsize2&7)
						mfmsize2=(mfmsize2/8)+1;
					else
						mfmsize2=mfmsize2/8;
				}

				if(mfmsize2>mfmsize) mfmsize=mfmsize2;

				if(mfmsize*2>0xFFFF)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Argg!! track %d too long (%x) and shorten to 0xFFFF !",i,mfmsize*2);
					mfmsize=0x7FFF;
				}

				track=(pictrack *)(offsettrack+(i*sizeof(pictrack)));
				track->track_len=mfmsize*2;
				track->offset=trackpos;

				if((mfmsize*2)%512)
					trackpos=trackpos+(((mfmsize*2)/512)+1);
				else
					trackpos=trackpos+((mfmsize*2)/512);

				//trackpos=trackpos+(((mfmsize*2)/512)+1);
			i++;
		};

		rfwrite(offsettrack,512*tracklistlen,1,hxcpicfile);

		i=0;
		while(i<(FILEHEADER->number_of_track))
		{

				mfmsize=floppy->tracks[i]->sides[0]->tracklen * 2;
				if(mfmsize&7)
					mfmsize=(mfmsize/8)+1;
				else
					mfmsize=mfmsize/8;

				mfmsize2=0;
				if(floppy->tracks[i]->number_of_side==2)
				{
					mfmsize2=floppy->tracks[i]->sides[1]->tracklen * 2;
					if(mfmsize2&7)
						mfmsize2=(mfmsize2/8)+1;
					else
						mfmsize2=mfmsize2/8;
				}

				if(mfmsize>0x7FFF)
				{
					mfmsize=0x7FFF;
				}
				if(mfmsize2>0x7FFF)
				{
					mfmsize2=0x7FFF;
				}
				track=(pictrack *)(offsettrack+(i*sizeof(pictrack)));

				if(track->track_len%512)
					tracksize=((track->track_len&(~0x1FF))+0x200)/2;//(((track->track_len/512)+1)*512)/2;
				else
					tracksize=track->track_len/2;

				mfmtracks0=(unsigned char*) malloc(tracksize);
				mfmtracks1=(unsigned char*) malloc(tracksize);
				mfmtrackfinal=(unsigned char*) malloc(tracksize*2);

				memset(mfmtracks0,0x00,tracksize);
				memset(mfmtracks1,0x00,tracksize);
				memset(mfmtrackfinal,0x55,tracksize*2);

				for(i_conv=0;i_conv<(mfmsize/2);i_conv++)
				{
					// Add the FM Clocks
					fm_pulses = ext_a2_bit_expander[floppy->tracks[i]->sides[0]->databuffer[i_conv]] | 0x2222;
					mfmtracks0[(i_conv*2)+0] = fm_pulses >> 8;
					mfmtracks0[(i_conv*2)+1] = fm_pulses &  0xFF;
				}

				addpad(mfmtracks0,mfmsize,tracksize);

				if(floppy->tracks[i]->number_of_side==2)
				{
					for(i_conv=0;i_conv<(mfmsize2/2);i_conv++)
					{
						// Add the FM Clocks
						fm_pulses = ext_a2_bit_expander[floppy->tracks[i]->sides[1]->databuffer[i_conv]] | 0x2222;
						mfmtracks1[(i_conv*2)+0] = fm_pulses >> 8;
						mfmtracks1[(i_conv*2)+1] = fm_pulses &  0xFF;
					}
					addpad(mfmtracks1,mfmsize2,tracksize);
				}
				else
				{
					memset(mfmtracks1,0xAA,tracksize);
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

				rfwrite(mfmtrackfinal,tracksize*2,1,hxcpicfile);

				free(mfmtracks0);
				free(mfmtracks1);
				free(mfmtrackfinal);

			i++;
		};

		free(offsettrack);


#ifdef FASTWRITE
		hxcpicfile=hxc_fopen(filename,"wb");
		if(hxcpicfile)
		{
			fwrite(ramfile,ramfile_size,1,hxcpicfile);
			hxc_fclose(hxcpicfile);
		}
		else
		{
			rfclose(hxcpicfile);
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);
			return -1;
		}

#endif

		rfclose(hxcpicfile);

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
