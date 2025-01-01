/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : afi_loader.c
// Contains: AFI floppy image loader
//
// Written by: Jean-Fran�ois DEL NERO
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

#include "afi_format.h"
#include "afi_loader.h"

#include "thirdpartylibs/zlib/zlib.h"
#include "tracks/crc.h"

#include "libhxcadaptor.h"

uint16_t filecheckcrc(FILE * f,uint32_t fileoffset,uint32_t size)
{
	unsigned char crc16l,crc16h;
	uint32_t temp_fileptr;
	unsigned char buffer[512];
	unsigned char crctable[32];
	int i,s;

	CRC16_Init(&crc16h,&crc16l,(unsigned char*)crctable,0x1021,0xFFFF);

	temp_fileptr=ftell(f);
	fseek(f,fileoffset,SEEK_SET);
	s=size;
	while(s)
	{

		if(s>512)
		{
			hxc_fread(&buffer,512,f);
			for(i=0;i<512;i++)
			{
				CRC16_Update(&crc16h,&crc16l,buffer[i],(unsigned char*)crctable);
			}
			s=s-512;
		}
		else
		{
			hxc_fread(&buffer,s,f);
			for(i=0;i<s;i++)
			{
				CRC16_Update(&crc16h,&crc16l,buffer[i],(unsigned char*)crctable);
			}
			s=0;
		}

	}
	fseek(f,temp_fileptr,SEEK_SET);

	return (uint16_t)((crc16l<<8) | crc16h);
}

uint32_t * bitrate_rle_unpack(uint32_t * packedbuffer,uint32_t len,unsigned long * outlen)
{
	uint32_t i,j,out_index;
	uint32_t unpackedlen;
	uint32_t * unpacked;

	// 0x80 00 00 00
	// 1XXXXXXX 00 00 00

	unpackedlen = 0;
	for(i=0;i<len;i++)
	{
		if(packedbuffer[i] & 0x80000000)
		{
			unpackedlen = unpackedlen + ((packedbuffer[i]>>24)&0x7F);
		}
		else
		{
			unpackedlen++;
		}
	}

	out_index = 0;
	unpacked = malloc(unpackedlen * sizeof(uint32_t));
	if(unpacked)
	{
		memset(unpacked,0,unpackedlen * sizeof(uint32_t));

		unpackedlen = 0;
		for(i=0;i<len;i++)
		{
			if(packedbuffer[i] & 0x80000000)
			{
				for(j=0;j<((packedbuffer[i]>>24)&0x7F);j++)
				{
					unpacked[out_index+j] = packedbuffer[i] & 0x00FFFFFF;
				}
				out_index = out_index + j;
			}
			else
			{
				unpacked[out_index] = packedbuffer[i] & 0x00FFFFFF;
				out_index ++;
			}
		}
	}

	*outlen = out_index;

	return unpacked;
}


int AFI_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	AFIIMG *header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"afi",SYS_PATH_TYPE))
	{

		header = (AFIIMG *)imgfile->file_header;

		if( !strncmp((char*)header->afi_img_tag,AFI_IMG_TAG,strlen(AFI_IMG_TAG)) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI_libIsValidDiskFile : AFI file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI_libIsValidDiskFile : non AFI file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI_libIsValidDiskFile : non AFI file !");
		return HXCFE_BADFILE;
	}
}

int AFI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	AFIIMG header;
	AFIIMGINFO  afiinfo;
	AFITRACKLIST trackliststruct;
	AFITRACK track;
	AFIDATA datablock;
	int i,j;
	uint32_t k,bytelen;
	unsigned long destLen;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	uint32_t * tracklistoffset;
	uint32_t * datalistoffset;

	unsigned char * temp_uncompressbuffer;
	uint32_t * temp_uncompressbuffer_long;

	uint32_t * temp_timing;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"AFI_libLoad_DiskFile : Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.afi_img_tag,AFI_IMG_TAG,strlen(AFI_IMG_TAG)))
	{

		if(filecheckcrc(f,0,sizeof(AFIIMG)))
		{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"AFI_libLoad_DiskFile : bad header CRC !");
				hxc_fclose(f);
				return HXCFE_BADFILE;
		}

		fseek(f,header.floppyinfo_offset,SEEK_SET);
		hxc_fread(&afiinfo,sizeof(afiinfo),f);

		floppydisk->floppyNumberOfTrack=afiinfo.end_track+1;
		floppydisk->floppyNumberOfSide=afiinfo.end_side+1;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;

		floppydisk->floppyBitRate = VARIABLEBITRATE;

		switch(afiinfo.platformtype_code)
		{
		case AFI_PLATFORM_ATARI_ST:
			switch(afiinfo.mediatype_code)
			{
			case AFI_MEDIA_3P50_DD:
				floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
				break;
			case AFI_MEDIA_3P50_HD:
				floppydisk->floppyiftype=ATARIST_HD_FLOPPYMODE;
				break;

			}
			break;
		case AFI_PLATFORM_AMIGA:
			switch(afiinfo.mediatype_code)
			{
			case AFI_MEDIA_3P50_DD:
				floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
				break;
			case AFI_MEDIA_3P50_HD:
				floppydisk->floppyiftype=AMIGA_HD_FLOPPYMODE;
				break;
			}
			break;

		case AFI_PLATFORM_PC:
			switch(afiinfo.mediatype_code)
			{
			case AFI_MEDIA_3P50_DD:
				floppydisk->floppyiftype=IBMPC_DD_FLOPPYMODE;
				break;
			case AFI_MEDIA_3P50_HD:
				floppydisk->floppyiftype=IBMPC_HD_FLOPPYMODE;
				break;
			case AFI_MEDIA_3P50_ED:
				floppydisk->floppyiftype=IBMPC_ED_FLOPPYMODE;
				break;
			}
			break;
		case AFI_PLATFORM_CPC:
			floppydisk->floppyiftype=CPC_DD_FLOPPYMODE;
			break;
		case AFI_PLATFORM_MSX2:
			floppydisk->floppyiftype=MSX2_DD_FLOPPYMODE;
			break;
		}

		//floppydisk->floppyBitRate=header.floppyBitRate*1000;
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AFI File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"floppydisk->tracks alloc error !");

			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		fseek(f,header.track_list_offset,SEEK_SET);
		hxc_fread(&trackliststruct,sizeof(trackliststruct),f);
		if(strncmp((char*)trackliststruct.afi_img_track_list_tag,AFI_TRACKLIST_TAG,strlen(AFI_TRACKLIST_TAG)))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad AFI_TRACKLIST_TAG");
			return HXCFE_BADFILE;
		}

		tracklistoffset=(uint32_t*)malloc(trackliststruct.number_of_track*sizeof(uint32_t));
		if(!tracklistoffset)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"tracklistoffset alloc error !");

			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		hxc_fread(tracklistoffset,trackliststruct.number_of_track*sizeof(uint32_t),f);

		for(i=0;i<(int)trackliststruct.number_of_track;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,trackliststruct.number_of_track );

			fseek(f,header.track_list_offset+tracklistoffset[i],SEEK_SET);
			hxc_fread(&track,sizeof(track),f);
			if(strncmp((char*)track.afi_track_tag,AFI_TRACK_TAG,strlen(AFI_TRACK_TAG)))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad AFI_TRACK_TAG");
				return HXCFE_BADFILE;
			}

			if(filecheckcrc(f,header.track_list_offset+tracklistoffset[i],(track.number_of_data_chunk*sizeof(uint32_t))+sizeof(AFITRACK)+sizeof(unsigned short)))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad track CRC !");
				hxc_fclose(f);
				return HXCFE_BADFILE;
			}

			datalistoffset=(uint32_t *)malloc(track.number_of_data_chunk*sizeof(uint32_t));
			if(!datalistoffset)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Data list alloc error !");
				hxc_fclose(f);
				return HXCFE_INTERNALERROR;
			}

			hxc_fread(datalistoffset,track.number_of_data_chunk*sizeof(uint32_t),f);

			if(!floppydisk->tracks[track.track_number])
			{
				floppydisk->tracks[track.track_number]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
				if(!floppydisk->tracks[track.track_number])
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"tracks array alloc error !");
					hxc_fclose(f);
					return HXCFE_INTERNALERROR;
				}

				currentcylinder = floppydisk->tracks[track.track_number];
				currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
				currentcylinder->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
				if(!currentcylinder->sides)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"sides array alloc error !");
					hxc_fclose(f);
					return HXCFE_INTERNALERROR;
				}

				memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
				currentcylinder->floppyRPM=0;//header.floppyRPM;
			}
			else
			{
				currentcylinder = floppydisk->tracks[track.track_number];
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
			track.track_number,
			track.side_number,
			track.number_of_data_chunk,
			track.nb_of_element);

			currentcylinder->sides[track.side_number]=malloc(sizeof(HXCFE_SIDE));
			memset(currentcylinder->sides[track.side_number],0,sizeof(HXCFE_SIDE));
			currentside=currentcylinder->sides[track.side_number];

			currentside->number_of_sector=floppydisk->floppySectorPerTrack;
			currentside->tracklen=track.nb_of_element;

			bytelen = currentside->tracklen;
			if(bytelen&7)
			{
				bytelen = (bytelen >> 3) + 1;
			}
			else
			{
				bytelen = (bytelen >> 3);
			}

			currentside->track_encoding=UNKNOWN_ENCODING;
			currentside->bitrate=250000;
			currentside->flakybitsbuffer=0;
			for(j=0;j<(int)track.number_of_data_chunk;j++)
			{
				fseek(f,header.track_list_offset+tracklistoffset[i]+datalistoffset[j],SEEK_SET);
				hxc_fread(&datablock,sizeof(datablock),f);

				if(strncmp((char*)datablock.afi_data_tag,AFI_DATA_TAG,strlen(AFI_DATA_TAG)))
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad AFI_DATA_TAG");
					return HXCFE_BADFILE;
				}

				if(filecheckcrc(f,header.track_list_offset+tracklistoffset[i]+datalistoffset[j],sizeof(AFIDATA)+datablock.packed_size+sizeof(unsigned short)))
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad data CRC !");
					hxc_fclose(f);
					return HXCFE_BADFILE;
				}

				switch(datablock.TYPEIDCODE)
				{
					case AFI_DATA_MFM:
						switch(datablock.packer_id)
						{
							case AFI_COMPRESS_NONE:
								currentside->databuffer = malloc(datablock.unpacked_size);
								if(currentside->databuffer)
									hxc_fread(currentside->databuffer,datablock.unpacked_size,f);
							break;
							case AFI_COMPRESS_GZIP:
								currentside->databuffer = malloc(datablock.unpacked_size);
								temp_uncompressbuffer = malloc(datablock.packed_size);
								if(currentside->databuffer && temp_uncompressbuffer)
								{
									hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);
									destLen = datablock.unpacked_size;
									uncompress(currentside->databuffer, &destLen,temp_uncompressbuffer, datablock.packed_size);
									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
								else
								{
									free(currentside->databuffer);
									currentside->databuffer = NULL;

									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
							break;
							default:
							break;
						}
					break;


					case AFI_DATA_CELL:
						switch(datablock.packer_id)
						{
							case AFI_COMPRESS_NONE:
								currentside->databuffer=malloc(datablock.unpacked_size);
								if(currentside->databuffer)
									hxc_fread(currentside->databuffer,datablock.unpacked_size,f);
								break;
							case AFI_COMPRESS_GZIP:
								currentside->databuffer=malloc(datablock.unpacked_size);
								temp_uncompressbuffer=malloc(datablock.packed_size);
								if(currentside->databuffer && temp_uncompressbuffer)
								{
									hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);
									destLen=datablock.unpacked_size;
									uncompress(currentside->databuffer, &destLen,temp_uncompressbuffer, datablock.packed_size);
									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
								else
								{
									free(currentside->databuffer);
									currentside->databuffer = NULL;

									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
							break;
							default:
							break;
						}
					break;

					case AFI_DATA_INDEX:
						switch(datablock.packer_id)
						{
							case AFI_COMPRESS_NONE:
								currentside->indexbuffer=malloc(datablock.packed_size);
								if(currentside->indexbuffer)
									hxc_fread(currentside->indexbuffer,datablock.packed_size,f);
							break;
							case AFI_COMPRESS_GZIP:
								currentside->indexbuffer=malloc(datablock.unpacked_size);
								temp_uncompressbuffer=malloc(datablock.packed_size);
								if(currentside->indexbuffer && temp_uncompressbuffer)
								{
									hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);
									destLen=datablock.unpacked_size;
									uncompress(currentside->indexbuffer, &destLen,temp_uncompressbuffer, datablock.packed_size);
									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
								else
								{
									free(currentside->indexbuffer);
									currentside->indexbuffer = NULL;

									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}

							break;
							default:
							break;
						}
					break;

					case AFI_DATA_BITRATE:
						currentside->bitrate=VARIABLEBITRATE;

						if(track.encoding_mode == AFI_TRACKENCODING_CELLARRAY)
						{
							switch(datablock.packer_id)
							{
								case AFI_COMPRESS_NONE:

									temp_timing = malloc(datablock.packed_size);
									currentside->timingbuffer=malloc( bytelen * sizeof(uint32_t));

									if(temp_timing && currentside->timingbuffer)
									{
										hxc_fread(temp_timing,datablock.packed_size,f);

										for(k=0;k<bytelen;k++)
										{
											currentside->timingbuffer[k] = temp_timing[k*8];
										}

										k=0;
										do
										{
											k++;
										}while( ( k < bytelen ) && (currentside->timingbuffer[k-1]==currentside->timingbuffer[k]) );

										if( k == bytelen )
										{
											currentside->bitrate=currentside->timingbuffer[0];
											free(currentside->timingbuffer);
											currentside->timingbuffer = NULL;
										}

										free(temp_timing);
										temp_timing = NULL;
									}
									else
									{
										free(currentside->timingbuffer);
										currentside->timingbuffer = NULL;

										free(temp_timing);
										temp_timing = NULL;
									}

								break;

								case AFI_COMPRESS_GZIP:

									currentside->timingbuffer=malloc(bytelen * sizeof(uint32_t));
									temp_uncompressbuffer=malloc(datablock.packed_size);
									if(temp_uncompressbuffer && currentside->timingbuffer)
									{
										hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);

										destLen = datablock.unpacked_size;
										if( destLen & ( sizeof(uint32_t) - 1 ) )
											destLen = ( destLen & ~( sizeof(uint32_t) - 1 ) ) + sizeof(uint32_t);

										temp_timing = malloc(destLen);
										if(temp_timing)
										{
											uncompress((unsigned char*)temp_timing, &destLen,temp_uncompressbuffer, datablock.packed_size);

											free(temp_uncompressbuffer);
											temp_uncompressbuffer = NULL;

											temp_uncompressbuffer_long = bitrate_rle_unpack(temp_timing,destLen/4,&destLen);
											if( temp_uncompressbuffer_long )
											{
												for(k=0;k<bytelen;k++)
												{
													currentside->timingbuffer[k] = temp_uncompressbuffer_long[k*8];
												}

												free(temp_uncompressbuffer_long);
												temp_uncompressbuffer_long = NULL;
											}

											k=0;
											do
											{
												k++;
											}while( ( k < bytelen ) && (currentside->timingbuffer[k-1]==currentside->timingbuffer[k]));

											if( k == bytelen )
											{
												currentside->bitrate=currentside->timingbuffer[0];
												free(currentside->timingbuffer);
												currentside->timingbuffer = NULL;
											}
										}

										free(temp_timing);
										temp_timing = NULL;
									}
									else
									{
										free(currentside->timingbuffer);
										currentside->timingbuffer = NULL;

										free(temp_uncompressbuffer);
										temp_uncompressbuffer = NULL;
									}
								break;
								default:
								break;
							}

						}
						else
						{
							switch(datablock.packer_id)
							{
								case AFI_COMPRESS_NONE:
									currentside->timingbuffer=malloc(datablock.packed_size);
									if(currentside->timingbuffer)
									{
										hxc_fread(currentside->timingbuffer,datablock.packed_size,f);

										k=0;
										do
										{
											k++;
										}while( ( k < datablock.packed_size ) && ( currentside->timingbuffer[k-1] == currentside->timingbuffer[k] ));

										if( k == datablock.packed_size )
										{
											currentside->bitrate=currentside->timingbuffer[0];
											free(currentside->timingbuffer);
											currentside->timingbuffer = NULL;
										}
									}
								break;

								case AFI_COMPRESS_GZIP:
									currentside->timingbuffer=malloc(datablock.unpacked_size);
									temp_uncompressbuffer=malloc(datablock.packed_size);
									if(currentside->timingbuffer && temp_uncompressbuffer)
									{
										hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);
										destLen=datablock.unpacked_size;
										uncompress((unsigned char*)currentside->timingbuffer, &destLen,temp_uncompressbuffer, datablock.packed_size);

										k=0;
										do
										{
											k++;
										}while( ( k < datablock.unpacked_size ) && ( currentside->timingbuffer[k-1] == currentside->timingbuffer[k] ) );

										if( k == datablock.unpacked_size )
										{
											currentside->bitrate = currentside->timingbuffer[0];
											free(currentside->timingbuffer);
											currentside->timingbuffer = NULL;
										}

										free(temp_uncompressbuffer);
										temp_uncompressbuffer = NULL;
									}
									else
									{
										free(currentside->timingbuffer);
										currentside->timingbuffer = NULL;

										free(temp_uncompressbuffer);
										temp_uncompressbuffer = NULL;
									}
								break;
								default:
								break;
							}
						}

					break;

					case AFI_DATA_PDC:

					break;

					case AFI_DATA_WEAKBITS:
						switch(datablock.packer_id)
						{
							case AFI_COMPRESS_NONE:
								currentside->flakybitsbuffer = malloc(datablock.packed_size);
								if(currentside->flakybitsbuffer)
								{
									hxc_fread(currentside->flakybitsbuffer,datablock.packed_size,f);
									k=0;
									do
									{
										k++;
									}while( ( k < datablock.packed_size ) && ( currentside->flakybitsbuffer[k-1] == currentside->flakybitsbuffer[k] ) );

									if( k == datablock.packed_size )
									{
										free(currentside->flakybitsbuffer);
										currentside->flakybitsbuffer = NULL;
									}
								}
								break;

							case AFI_COMPRESS_GZIP:
								currentside->flakybitsbuffer=malloc(datablock.unpacked_size);
								temp_uncompressbuffer=malloc(datablock.packed_size);
								if( currentside->flakybitsbuffer && temp_uncompressbuffer )
								{
									hxc_fread(temp_uncompressbuffer,datablock.packed_size,f);
									destLen=datablock.unpacked_size;
									uncompress(currentside->flakybitsbuffer, &destLen,temp_uncompressbuffer, datablock.packed_size);

									k=0;
									do
									{
										k++;
									}while( ( k < datablock.unpacked_size ) && ( currentside->flakybitsbuffer[k-1] == currentside->flakybitsbuffer[k] ) && !currentside->flakybitsbuffer[k]);

									if(k==datablock.unpacked_size)
									{
										free(currentside->flakybitsbuffer);
										currentside->flakybitsbuffer = NULL;
									}

									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
								else
								{
									free(currentside->flakybitsbuffer);
									currentside->flakybitsbuffer = NULL;

									free(temp_uncompressbuffer);
									temp_uncompressbuffer = NULL;
								}
								break;
							default:
							break;
						}
					break;

					default:

					break;

				}
			}

			free(datalistoffset);
			datalistoffset = NULL;
		}

		free(tracklistoffset);
		tracklistoffset = NULL;

		if((header.version_code_major == 0) && header.version_code_minor == 1)
		{
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					if(floppydisk->tracks[j])
					{
						if(floppydisk->tracks[j]->sides[i])
						{
							currentside=floppydisk->tracks[j]->sides[i];
							currentside->tracklen=currentside->tracklen*8;
						}
					}
				}
			}
		}


		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				if(floppydisk->tracks[j])
				{
					if(floppydisk->tracks[j]->sides[i])
					{
						currentside=floppydisk->tracks[j]->sides[i];

						if(currentside->timingbuffer)
						{
							floppydisk->tracks[j]->floppyRPM = (short)( 60 / GetTrackPeriod(imgldr_ctx->hxcfe,currentside) );
						}
						else
						{
							floppydisk->tracks[j]->floppyRPM = (unsigned short)( ( currentside->bitrate * 60 ) / ( currentside->tracklen / 2 ) );
						}
					}
				}
			}
		}

		floppyTrackTypeIdentification(imgldr_ctx->hxcfe,floppydisk);

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;
}

int AFI_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int AFI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_AFI";
	static const char plug_desc[]="HxC AFI file loader";
	static const char plug_ext[]="afi";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   AFI_libIsValidDiskFile,
		(LOADDISKFILE)      AFI_libLoad_DiskFile,
		(WRITEDISKFILE)     AFI_libWrite_DiskFile,
		(GETPLUGININFOS)    AFI_libGetPluginInfo
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
