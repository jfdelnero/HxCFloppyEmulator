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
// File : hfev3_loader.c
// Contains: HFE V3 floppy image loader
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

#include "hfev3_loader.h"
#include "hfev3_format.h"

#include "hfev3_trackgen.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"


static char * trackencodingcode[]=
{
	"ISOIBM_MFM_ENCODING",
	"AMIGA_MFM_ENCODING",
	"ISOIBM_FM_ENCODING",
	"EMU_FM_ENCODING",
	"UNKNOWN_ENCODING"
};

static char * interfacemodecode[]=
{
	"IBMPC_DD_FLOPPYMODE",
	"IBMPC_HD_FLOPPYMODE",
	"ATARIST_DD_FLOPPYMODE",
	"ATARIST_HD_FLOPPYMODE",
	"AMIGA_DD_FLOPPYMODE",
	"AMIGA_HD_FLOPPYMODE",
	"CPC_DD_FLOPPYMODE",
	"GENERIC_SHUGART_DD_FLOPPYMODE",
	"IBMPC_ED_FLOPPYMODE",
	"MSX2_DD_FLOPPYMODE",
	"C64_DD_FLOPPYMODE",
	"EMU_SHUGART_FLOPPYMODE"
};

int HFEV3_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	picfileformatheader * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEV3_libIsValidDiskFile");

	if(imgfile)
	{
		header = (picfileformatheader *)&imgfile->file_header;

		if( !strncmp((char*)header->HEADERSIGNATURE,"HXCHFEV3",8))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEV3_libIsValidDiskFile : HFE V3 file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEV3_libIsValidDiskFile : non HFE V3 file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEV3_libIsValidDiskFile : non HFE V3 file !");
		return HXCFE_BADFILE;
	}
}

int setbits(unsigned char * bufout,int bitoffset_out,int state, int size,int bufoutsize)
{
	int i;

	for(i = 0;i<size;i++)
	{
		if(bitoffset_out < bufoutsize )
		{
			if( state )
			{
				bufout[bitoffset_out>>3] |= ( 0x80 >> (bitoffset_out & 7) );
			}
			else
			{
				bufout[bitoffset_out>>3] &= ~( 0x80 >> (bitoffset_out & 7) );
			}

			bitoffset_out++;
		}
		else
		{
			return bufoutsize;
		}
	}
	return bitoffset_out;
}

int cpybits(unsigned char * bufout,int bitoffset_out,unsigned char * bufin,int bitoffset_in, int size, int bufoutsize, int bufinsize)
{
	int i;

	for(i = 0;i<size;i++)
	{
		if( bitoffset_in < bufinsize && bitoffset_out < bufoutsize )
		{
			if( bufin[bitoffset_in>>3] & ( 0x80 >> (bitoffset_in & 7) ) )
			{
				bufout[bitoffset_out>>3] |= ( 0x80 >> (bitoffset_out & 7) );
			}
			else
			{
				bufout[bitoffset_out>>3] &= ~( 0x80 >> (bitoffset_out & 7) );
			}
			bitoffset_in++;
			bitoffset_out++;
		}
		else
		{
			return bitoffset_out;
		}
	}

	return bitoffset_out;
}

int HFEV3_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	picfileformatheader header;
	int i,j,k,l;
	int offset,offset2;
	int bitoffset_in,bitoffset_out;
	unsigned int tracks_base;
	HXCFE_CYLINDER* currentcylinder = NULL;
	HXCFE_SIDE* currentside = NULL;
	pictrack* trackoffsetlist = NULL;
	unsigned char * hfetrack = NULL;
	unsigned char * hfetrack2 = NULL;
	int nbofblock,tracklen,bitrate;
	unsigned char tmp_randmask;
	unsigned int next_data_bitskip;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEV3_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.HEADERSIGNATURE,"HXCHFEV3",8))
	{

		floppydisk->floppyNumberOfTrack=header.number_of_track;
		floppydisk->floppyNumberOfSide=header.number_of_side;
		floppydisk->floppyBitRate=header.bitRate*1000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=header.floppyinterfacemode;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : %d track, %d side, %d bit/s, %d sectors, interface mode %s, track encoding:%s",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype<0xC?interfacemodecode[floppydisk->floppyiftype]:"Unknow!",
			(header.track_encoding&(~3))?trackencodingcode[4]:trackencodingcode[header.track_encoding&0x3]);

		trackoffsetlist=(pictrack*)malloc(sizeof(pictrack)* header.number_of_track);
		if(!trackoffsetlist)
			goto alloc_error;

		memset(trackoffsetlist,0,sizeof(pictrack)* header.number_of_track);
		fseek( f,512,SEEK_SET);
		hxc_fread( trackoffsetlist,sizeof(pictrack)* header.number_of_track,f);

		tracks_base= 512+( (((sizeof(pictrack)* header.number_of_track)/512)+1)*512);
		fseek( f,tracks_base,SEEK_SET);

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto alloc_error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{

			fseek(f,(trackoffsetlist[i].offset*512),SEEK_SET);
			if(trackoffsetlist[i].track_len&0x1FF)
			{
				tracklen=(trackoffsetlist[i].track_len&(~0x1FF))+0x200;
			}
			else
			{
				tracklen=trackoffsetlist[i].track_len;
			}

			hfetrack = (unsigned char*)malloc( tracklen );
			hfetrack2 = (unsigned char*)malloc( tracklen );

			if(!hfetrack || !hfetrack2)
				goto alloc_error;

			memset(hfetrack, 0, tracklen);
			memset(hfetrack2, 0, tracklen);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : reading track %d, track size:%d - file offset:%.8X",
				i,tracklen,(trackoffsetlist[i].offset*512));

			hxc_fread( hfetrack,tracklen,f);

			floppydisk->tracks[i] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
			if( !floppydisk->tracks[i] )
				goto alloc_error;
			memset(floppydisk->tracks[i],0, sizeof(HXCFE_CYLINDER));

			currentcylinder=floppydisk->tracks[i];
			currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;

			currentcylinder->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
			if( !currentcylinder->sides )
				goto alloc_error;

			memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
			currentcylinder->floppyRPM = header.floppyRPM;

			for(j=0;j<currentcylinder->number_of_side;j++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) + (j&1),floppydisk->floppyNumberOfTrack*2 );

				currentcylinder->sides[j] = malloc(sizeof(HXCFE_SIDE));
				if( !currentcylinder->sides[j] )
					goto alloc_error;
				memset(currentcylinder->sides[j],0,sizeof(HXCFE_SIDE));
				currentside = currentcylinder->sides[j];

				currentside->number_of_sector = floppydisk->floppySectorPerTrack;
				currentside->tracklen = tracklen/2;

				currentside->databuffer = malloc(currentside->tracklen);
				if( !currentside->databuffer )
					goto alloc_error;
				memset(currentside->databuffer,0,currentside->tracklen);

				currentside->flakybitsbuffer = malloc(currentside->tracklen);
				if( !currentside->flakybitsbuffer )
					goto alloc_error;
				memset(currentside->flakybitsbuffer,0,currentside->tracklen);

				currentside->indexbuffer = malloc(currentside->tracklen);
				if( !currentside->indexbuffer )
					goto alloc_error;
				memset(currentside->indexbuffer,0,currentside->tracklen);

				currentside->timingbuffer = malloc(currentside->tracklen * sizeof(uint32_t));
				if( !currentside->timingbuffer )
					goto alloc_error;

				for(k=0;k<currentside->tracklen;k++)
				{
					currentside->timingbuffer[k] = floppydisk->floppyBitRate;
				}

				currentside->bitrate = VARIABLEBITRATE;

				currentside->track_encoding=header.track_encoding;

				if( i == 0 )
				{
					if ( j == 0 )
					{
						if(!header.track0s0_altencoding)
						{
							currentside->track_encoding = header.track0s0_encoding;
						}
					}
					else
					{
						if(!header.track0s1_altencoding)
						{
							currentside->track_encoding = header.track0s1_encoding;
						}
					}
				}

				nbofblock=(currentside->tracklen/256);
				for(k=0;k<nbofblock;k++)
				{
					for(l=0;l<256;l++)
					{
						offset=(k*256)+l;
						offset2=(k*512)+l+(256*j);
						hfetrack2[offset] = LUT_ByteBitsInverter[hfetrack[offset2]];
					}
				}

				bitrate = floppydisk->floppyBitRate;
				l = 0;
				bitoffset_in = 0;
				bitoffset_out = 0;
				next_data_bitskip = 0;
				k = 0;
				while(k < currentside->tracklen && l < currentside->tracklen)
				{
					if( ( hfetrack2[l] & OPCODE_MASK ) == OPCODE_MASK )
					{
						switch( hfetrack2[l] )
						{
							case NOP_OPCODE:

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : NOP_OPCODE",i,j,l);

								l++;
								bitoffset_in += 8;
								break;

							case SETINDEX_OPCODE:

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : SETINDEX_OPCODE",i,j,l);

								setbits(currentside->indexbuffer,bitoffset_out,1, 256*8, currentside->tracklen*8);

								l++;
								bitoffset_in += 8;
								break;

							case SETBITRATE_OPCODE:
								if( hfetrack2[l+1] )
								{
									bitrate = FLOPPYEMUFREQ / ( hfetrack2[l+1] * 2);
								}
								else
								{
									imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFEv3 : T%.3dS%d Off[%.5d] : SETBITRATE_OPCODE : %d. NULL Rate divisor !",i,j,l,hfetrack2[l+1]);
								}

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : SETBITRATE_OPCODE : %d - %d",i,j,l,hfetrack2[l+1],bitrate);

								l += 2;
								bitoffset_in += 8*2;
								break;

							case SKIPBITS_OPCODE:
								next_data_bitskip = hfetrack2[l+1];

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : SKIPBITS_OPCODE : %d 0x%.2X",i,j,l,next_data_bitskip,hfetrack2[l+2]);

								next_data_bitskip &= 0x7;

								bitoffset_in += 8*2;
								l += 2;
							break;

							case RAND_OPCODE:

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : RAND_OPCODE",i,j,l);

								tmp_randmask = rand();
								tmp_randmask = tmp_randmask  & 0x54;

								cpybits(currentside->databuffer,bitoffset_out,&tmp_randmask,0, 8 - next_data_bitskip,currentside->tracklen*8,tracklen*8);
								tmp_randmask = 0xFF;
								bitoffset_out = cpybits(currentside->flakybitsbuffer,bitoffset_out,&tmp_randmask,0, 8 - next_data_bitskip,currentside->tracklen*8,tracklen*8);

								currentside->timingbuffer[k] = bitrate;
								bitoffset_in += 8;

								next_data_bitskip = 0;

								k++;
								l++;
							break;

							default:

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFEv3 : T%.3dS%d Off[%.5d] : Unknown Opcode !?! : 0x%.2X",hfetrack2[l]);

								l++;
								bitoffset_in += 8;
								break;
						}
					}
					else
					{
						bitoffset_out = cpybits(currentside->databuffer,bitoffset_out,hfetrack2,bitoffset_in + next_data_bitskip, 8 - next_data_bitskip,currentside->tracklen*8,tracklen*8);

						currentside->timingbuffer[k] = bitrate;
						bitoffset_in += 8;
						next_data_bitskip = 0;
						k++;
						l++;
					}
				}

				currentside->tracklen = bitoffset_out;

				if(!currentcylinder->floppyRPM)
					currentcylinder->floppyRPM = (short)( 60 / GetTrackPeriod(imgldr_ctx->hxcfe,currentside) );

			}

			free(hfetrack);
			free(hfetrack2);

		}

		free(trackoffsetlist);

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;

alloc_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Internal memory allocation error ! Please report !");

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	free(trackoffsetlist);
	free(hfetrack);
	free(hfetrack2);

	return HXCFE_INTERNALERROR;
}

int HFEV3_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int HFEV3_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_HFEV3";
	static const char plug_desc[]="SD Card HxCFE HFE V3 file Loader";
	static const char plug_ext[]="hfe";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   HFEV3_libIsValidDiskFile,
		(LOADDISKFILE)      HFEV3_libLoad_DiskFile,
		(WRITEDISKFILE)     HFEV3_libWrite_DiskFile,
		(GETPLUGININFOS)    HFEV3_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
