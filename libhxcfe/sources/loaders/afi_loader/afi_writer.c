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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "afi_format.h"
#include "afi_loader.h"

#include "thirdpartylibs/zlib/zlib.h"

#include "tracks/crc.h"

#include "libhxcadaptor.h"

AFI_DATACODE datacode[]={
	{AFI_DATA_MFM,AFI_DATA_TYPE_MFM},
	{AFI_DATA_INDEX,AFI_DATA_TYPE_INDEX},
	{AFI_DATA_BITRATE,AFI_DATA_TYPE_BITRATE},
	{AFI_DATA_PDC,AFI_DATA_TYPE_PDC},
	{AFI_DATA_WEAKBITS,AFI_DATA_TYPE_WEAKBITS},
	{AFI_DATA_CELL,AFI_DATA_TYPE_CELL},
	{AFI_DATA_NONE,""}
};

unsigned short getcrc(void * buffer1, int size1,void * buffer2, int size2)
{

	unsigned char crc16l,crc16h;
	int i;
	unsigned char crctable[32];

	CRC16_Init(&crc16h,&crc16l,(unsigned char*)crctable,0x1021,0xFFFF);

	if(buffer1)
	{
		for(i=0;i<size1;i++)
		{
			CRC16_Update(&crc16h,&crc16l,*((unsigned char*)(buffer1)+i),(unsigned char*)crctable);
		}
	}

	if(buffer2)
	{
		for(i=0;i<size2;i++)
		{
			CRC16_Update(&crc16h,&crc16l,*((unsigned char*)(buffer2)+i),(unsigned char*)crctable);
		}
	}

	return (crc16l<<8) | crc16h;

}

int adddatablock(FILE* f,int typecode,int compressdata,unsigned char* data,int datalen,int bitsperelement)
{
	int i;
	unsigned long actualdestbufferlen;
	AFIDATA     afidata;
	unsigned char *tempcompressbuffer;
	unsigned short tempcrc;

	i=0;
	while(datacode[i].idcode!=typecode && datacode[i].idcode!=AFI_DATA_NONE)
	{
		i++;
	}

	memset(&afidata,0,sizeof(AFIDATA));
	strcpy((char*)&afidata.afi_data_tag,AFI_DATA_TAG);
	afidata.TYPEIDCODE=datacode[i].idcode;
	strcpy((char*)&afidata.type_tag,datacode[i].idcodetag);
	afidata.nb_bits_per_element = bitsperelement;

	if(compressdata)
	{
		actualdestbufferlen = compressBound(datalen);
		tempcompressbuffer = (unsigned char*)malloc(actualdestbufferlen);
		if(!tempcompressbuffer)
		{
			return 0;
		}

		compress2(tempcompressbuffer, &actualdestbufferlen,
			 data, datalen,
			 Z_BEST_COMPRESSION);
		afidata.unpacked_size = datalen;
		afidata.packed_size = actualdestbufferlen;
		afidata.packer_id = AFI_COMPRESS_GZIP;
	}
	else
	{
		tempcompressbuffer = (unsigned char*)malloc(datalen);
		if(!tempcompressbuffer)
		{
			return 0;
		}

		memcpy(tempcompressbuffer,data,datalen);
		afidata.unpacked_size = datalen;
		afidata.packed_size = datalen; // no comp
		afidata.packer_id = AFI_COMPRESS_NONE;
	}

	fwrite(&afidata,sizeof(afidata),1,f);
	fwrite(tempcompressbuffer,afidata.packed_size,1,f);
	tempcrc=getcrc(&afidata,sizeof(afidata),tempcompressbuffer,afidata.packed_size);
	fwrite(&tempcrc,sizeof(tempcrc),1,f);            //temporary crc

	free(tempcompressbuffer);
	return afidata.packed_size;
}

uint32_t * bitrate_rle_pack(uint32_t * bitrate,uint32_t len,uint32_t * outlen)
{
	uint32_t i,j;
	uint32_t nb_entry;

	uint32_t entry,cur_nb;

	uint32_t * packed;

	// 0x80 00 00 00
	// 1XXXXXXX 00 00 00

	nb_entry = 1;
	cur_nb = 1;
	for(i=1;i<len;i++)
	{
		if(bitrate[i-1] == bitrate[i] && cur_nb<0x7F)
		{
			cur_nb++;
		}
		else
		{
			nb_entry++;
			cur_nb = 1;
		}
	}
	nb_entry++;

	j=0;
	packed = malloc(nb_entry * sizeof(uint32_t));
	if(packed)
	{
		memset(packed,0,nb_entry * sizeof(uint32_t));
		nb_entry = 1;
		cur_nb = 1;

		for(i=1;i<len;i++)
		{
			if(bitrate[i-1] == bitrate[i] && cur_nb<127)
			{
				cur_nb++;
			}
			else
			{
				nb_entry++;

				entry = ( bitrate[i] & 0x00FFFFFF ) | ((0x80 |(cur_nb&0x7F)) << 24);
				packed[j] = entry;
				cur_nb = 1;
				j++;
			}
		}

		entry = ( bitrate[i-1] & 0x00FFFFFF ) | ((0x80 |(cur_nb&0x7F)) << 24);
		packed[j] = entry;
		j++;
	}

	*outlen = j;

	return packed;
}


int AFI_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{

	FILE * hxcafifile;
	AFIIMG      afiheader;
	AFIIMGINFO  afiinfo;
//	AFI_STRING  afistring;
	AFITRACKLIST afitracklist;
	AFITRACK   afitrack;
	unsigned short tempcrc;

	uint32_t * track_list;
	int compressdata;

	uint32_t data_list[16];

	int i,j,k,t;
	unsigned int l;

	uint32_t trackposition;
	uint32_t dataposition;

	uint32_t track_listptr;
	uint32_t track_fileptr;
	uint32_t temp_fileptr;

	uint32_t *tempbitratetrack;
	unsigned char *tempweakbitstrack;
	uint32_t *bitrate_rle_packed;
	int block_size;
	uint32_t bytelen,outsize;

	compressdata=1;

	floppy->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write AFI file %s...",filename);

	hxcafifile=hxc_fopen(filename,"wb");
	if(hxcafifile)
	{
		tempcrc=0x0000;

		//------------- header -------------
		memset(&afiheader,0,sizeof(AFIIMG));
		sprintf((char*)&afiheader.afi_img_tag,AFI_IMG_TAG);
		afiheader.header_size=sizeof(AFIIMG);
		afiheader.version_code_minor = 2;
		afiheader.header_crc=getcrc(&afiheader,sizeof(afiheader)-2,0,0);
		//------------- info -------------
		memset(&afiinfo,0,sizeof(AFIIMGINFO));
		sprintf((char*)&afiinfo.afi_img_infos_tag,AFI_INFO_TAG);

		afiinfo.start_side=0;
		afiinfo.end_side=floppy->floppyNumberOfSide-1;

		afiinfo.start_track=0;
		afiinfo.end_track=floppy->floppyNumberOfTrack-1;

		afiinfo.total_track=floppy->floppyNumberOfTrack*floppy->floppyNumberOfSide;

		switch(floppy->floppyiftype)
		{
			case IBMPC_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_DD;
				afiinfo.platformtype_code=AFI_PLATFORM_PC;
				break;
			case IBMPC_HD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_HD;
				afiinfo.platformtype_code=AFI_PLATFORM_PC;
				break;
			case IBMPC_ED_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_ED;
				afiinfo.platformtype_code=AFI_PLATFORM_PC;
				break;
			case ATARIST_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_DD;
				afiinfo.platformtype_code=AFI_PLATFORM_PC;
				break;
			case ATARIST_HD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_HD;
				afiinfo.platformtype_code=AFI_PLATFORM_ATARI_ST;
				break;
			case AMIGA_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_DD;
				afiinfo.platformtype_code=AFI_PLATFORM_AMIGA;
				break;
			case AMIGA_HD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_HD;
				afiinfo.platformtype_code=AFI_PLATFORM_AMIGA;
				break;
			case CPC_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P00;
				afiinfo.platformtype_code=AFI_PLATFORM_CPC;
				break;

			case GENERIC_SHUGART_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_DD;
				afiinfo.platformtype_code=AFI_PLATFORM_UNKNOW;
				break;

			case MSX2_DD_FLOPPYMODE:
				afiinfo.mediatype_code=AFI_MEDIA_3P50_HD;
				afiinfo.platformtype_code=AFI_PLATFORM_MSX2;
				break;
		}


		afiinfo.number_of_string=0;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d Tracks, %d side(s)",afiinfo.end_track,afiinfo.end_side);

		//------------- track list -------------
		memset(&afitracklist,0,sizeof(AFITRACKLIST));
		sprintf((char*)&afitracklist.afi_img_track_list_tag,AFI_TRACKLIST_TAG);
		afitracklist.number_of_track=floppy->floppyNumberOfTrack*floppy->floppyNumberOfSide;

		track_list=(uint32_t *)malloc(afitracklist.number_of_track*sizeof(uint32_t));
		if(track_list)
		{
			memset(track_list,0,afitracklist.number_of_track*sizeof(uint32_t));

			afiheader.track_list_offset=sizeof(AFIIMG);

			fwrite(&afiheader,sizeof(afiheader),1,hxcafifile);	      //write temporary file header
			fwrite(&afitracklist,sizeof(afitracklist),1,hxcafifile);  //write temporary track list header
			track_listptr=ftell(hxcafifile);
			fwrite(track_list,afitracklist.number_of_track*sizeof(uint32_t),1,hxcafifile);
			tempcrc=getcrc(&afitracklist,sizeof(afitracklist),track_list,afitracklist.number_of_track*sizeof(uint32_t));
			fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc

			trackposition=sizeof(afitracklist)+afitracklist.number_of_track*sizeof(uint32_t)+sizeof(tempcrc);
			t=0;
			for(i=0;i<floppy->floppyNumberOfTrack;i++)
			{
				for(j=0;j<floppy->tracks[i]->number_of_side;j++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) + (j&1),(2*floppy->floppyNumberOfTrack) );

					memset(data_list,0,sizeof(uint32_t)*16);
					memset(&afitrack,0,sizeof(afitrack));
					sprintf((char*)&afitrack.afi_track_tag,AFI_TRACK_TAG);

					track_list[t]=trackposition;

					afitrack.track_number=i;
					afitrack.side_number=j;

					afitrack.encoding_mode = AFI_TRACKENCODING_CELLARRAY;

					bytelen = floppy->tracks[i]->sides[j]->tracklen;
					if(bytelen&7)
					{
						bytelen = (bytelen >> 3) + 1;
					}
					else
					{
						bytelen = (bytelen >> 3);
					}

					afitrack.nb_of_element = floppy->tracks[i]->sides[j]->tracklen;
					afitrack.number_of_data_chunk = 4;//-------

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d [%d:%d], file offset %X",t,afitrack.track_number,afitrack.side_number,track_list[t]+afiheader.track_list_offset);

					track_fileptr = ftell(hxcafifile);
					fwrite(&afitrack,sizeof(afitrack),1,hxcafifile);
					fwrite(&data_list,afitrack.number_of_data_chunk*sizeof(uint32_t),1,hxcafifile);
					tempcrc=getcrc(&afitrack,sizeof(afitrack),data_list,afitrack.number_of_data_chunk*sizeof(uint32_t));
					fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc

					dataposition=sizeof(afitrack)+(afitrack.number_of_data_chunk*sizeof(uint32_t))+sizeof(tempcrc);

					//---------------- index data ----------------
					block_size=adddatablock(hxcafifile,AFI_DATA_INDEX,compressdata,floppy->tracks[i]->sides[j]->indexbuffer,bytelen,1);
					//--------------------------------------------
					data_list[0]=dataposition;

					///rewrite///
					temp_fileptr=ftell(hxcafifile);
					fseek(hxcafifile,track_fileptr,SEEK_SET);
					fwrite(&afitrack,sizeof(afitrack),1,hxcafifile);
					fwrite(&data_list,afitrack.number_of_data_chunk*sizeof(uint32_t),1,hxcafifile);
					tempcrc=getcrc(&afitrack,sizeof(afitrack),data_list,afitrack.number_of_data_chunk*sizeof(uint32_t));
					fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc
					fseek(hxcafifile,temp_fileptr,SEEK_SET);
					//////////////////////////////////////////////

					dataposition=dataposition+sizeof(AFIDATA)+block_size+sizeof(tempcrc);

					//---------------- track data ----------------
					block_size=adddatablock(hxcafifile,AFI_DATA_CELL,compressdata,floppy->tracks[i]->sides[j]->databuffer,bytelen,1);
					//--------------------------------------------
					data_list[1]=dataposition;

					///rewrite///
					temp_fileptr=ftell(hxcafifile);
					fseek(hxcafifile,track_fileptr,SEEK_SET);
					fwrite(&afitrack,sizeof(afitrack),1,hxcafifile);
					fwrite(&data_list,afitrack.number_of_data_chunk*sizeof(uint32_t),1,hxcafifile);
					tempcrc=getcrc(&afitrack,sizeof(afitrack),data_list,afitrack.number_of_data_chunk*sizeof(uint32_t));
					fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc
					fseek(hxcafifile,temp_fileptr,SEEK_SET);
					//////////////////////////////////////////////

					dataposition=dataposition+sizeof(AFIDATA)+block_size+sizeof(tempcrc);

					//---------------- bitrate data ----------------

					tempbitratetrack=(uint32_t *)malloc(bytelen * 8 * sizeof(uint32_t));
					if(tempbitratetrack)
					{
						if(floppy->tracks[i]->sides[j]->bitrate==VARIABLEBITRATE)
						{
							for(l=0;l<bytelen;l++)
							{
								for(k=0;k<8;k++)
								{
									tempbitratetrack[(l*8)+k] = floppy->tracks[i]->sides[j]->timingbuffer[l];
								}
							}
						}
						else
						{
							for(l=0;l<bytelen;l++)
							{
								for(k=0;k<8;k++)
								{
									tempbitratetrack[(l*8)+k] = floppy->tracks[i]->sides[j]->bitrate;
								}
							}
						}

						bitrate_rle_packed = bitrate_rle_pack(tempbitratetrack,bytelen*8,&outsize);

						block_size=adddatablock(hxcafifile,AFI_DATA_BITRATE,compressdata,(unsigned char*)bitrate_rle_packed,outsize*4,32);
						free(bitrate_rle_packed);
						free(tempbitratetrack);
					}

					//--------------------------------------------
					data_list[2]=dataposition;

					///rewrite///
					temp_fileptr=ftell(hxcafifile);
					fseek(hxcafifile,track_fileptr,SEEK_SET);
					fwrite(&afitrack,sizeof(afitrack),1,hxcafifile);
					fwrite(&data_list,afitrack.number_of_data_chunk*sizeof(uint32_t),1,hxcafifile);
					tempcrc=getcrc(&afitrack,sizeof(afitrack),data_list,afitrack.number_of_data_chunk*sizeof(uint32_t));
					fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc
					fseek(hxcafifile,temp_fileptr,SEEK_SET);
					//////////////////////////////////////////////

					dataposition=dataposition+sizeof(AFIDATA)+block_size+sizeof(tempcrc);

					//---------------- weakbits data ----------------
					if(floppy->tracks[i]->sides[j]->flakybitsbuffer)
					{
						block_size=adddatablock(hxcafifile,AFI_DATA_WEAKBITS,compressdata,floppy->tracks[i]->sides[j]->flakybitsbuffer,bytelen,1);
					}
					else
					{
						tempweakbitstrack=(unsigned char *)malloc(bytelen);
						if(tempweakbitstrack)
						{
							memset(tempweakbitstrack,0,bytelen);

							block_size=adddatablock(hxcafifile,AFI_DATA_WEAKBITS,compressdata,tempweakbitstrack,bytelen,1);
							free(tempweakbitstrack);
						}
					}
					//--------------------------------------------
					data_list[3]=dataposition;

					///rewrite///
					temp_fileptr=ftell(hxcafifile);
					fseek(hxcafifile,track_fileptr,SEEK_SET);
					fwrite(&afitrack,sizeof(afitrack),1,hxcafifile);
					fwrite(&data_list,afitrack.number_of_data_chunk*sizeof(uint32_t),1,hxcafifile);
					tempcrc=getcrc(&afitrack,sizeof(afitrack),data_list,afitrack.number_of_data_chunk*sizeof(uint32_t));
					fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc
					fseek(hxcafifile,temp_fileptr,SEEK_SET);
					//////////////////////////////////////////////

					dataposition=dataposition+sizeof(AFIDATA)+block_size+sizeof(tempcrc);

					t++;
					trackposition=trackposition+dataposition;

				}
			}

			temp_fileptr=ftell(hxcafifile);
			tempcrc=getcrc(&afitracklist,sizeof(afitracklist),track_list,afitracklist.number_of_track*sizeof(uint32_t));
			fseek(hxcafifile,track_listptr,SEEK_SET);
			fwrite(track_list,afitracklist.number_of_track*sizeof(uint32_t),1,hxcafifile);
			fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc
			fseek(hxcafifile,temp_fileptr,SEEK_SET);

			afiheader.floppyinfo_offset=ftell(hxcafifile);
			fwrite(&afiinfo,sizeof(AFIIMGINFO),1,hxcafifile);
			tempcrc=getcrc(&afiinfo,sizeof(AFIIMGINFO),0,0);
			fwrite(&tempcrc,sizeof(tempcrc),1,hxcafifile);            //temporary crc


			temp_fileptr=ftell(hxcafifile);
			fseek(hxcafifile,0,SEEK_SET);
			afiheader.header_crc=getcrc(&afiheader,sizeof(afiheader)-2,0,0);
			fwrite(&afiheader,sizeof(afiheader),1,hxcafifile);	      //write temporary file header
			fseek(hxcafifile,temp_fileptr,SEEK_SET);

			free(track_list);
		}

		hxc_fclose(hxcafifile);

		return HXCFE_NOERROR;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);

		return HXCFE_ACCESSERROR;
	}
}
