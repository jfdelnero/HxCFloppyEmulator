/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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

#include "libhxcfe.h"

#include "dmk_format.h"
#include "dmk_loader.h"

#include "dmk_writer.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

int gettrackbit(unsigned char * input_data,int bit_offset)
{
	return ((input_data[bit_offset>>3]>>(0x7-(bit_offset&0x7))))&0x01;
}

void setdmktrackbit(unsigned char * input_data,int bit_offset,int state)
{
	if(state)
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] |  (0x80 >> ( bit_offset&0x7 ) );
	}
	else
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] & ~(0x80 >> ( bit_offset&0x7 ) );
	}

	return;
}

int dmkbitlookingfor(unsigned char * input_data,unsigned long intput_data_size,int searchlen,unsigned char * chr_data,unsigned long chr_data_size,unsigned long bit_offset)
{
	unsigned long i,j,trackoffset,cnt,k,starti;
	unsigned char stringtosearch[8][128];
	unsigned char mask[8][128];

	unsigned char prev;
	unsigned long tracksize;
	int searchsize;
	int t;
	int found;
	int bitoffset;

	memset(stringtosearch,0,8*128);
	memset(mask,0xFF,8*128);

	cnt=(chr_data_size>>3);
	if(chr_data_size&7)
		cnt++;

	// Prepare strings & mask ( input string shifted 7 times...)
	for(i=0;i<8;i++)
	{
		prev=0;

		mask[i][0]=0xFF>>i;
		for(j=0;j<cnt;j++)
		{
			stringtosearch[i][j]=prev | (chr_data[j]>>i);
			prev=chr_data[j]<<(8-i);
		}
		stringtosearch[i][j]=prev;
		mask[i][j]=0xFF<<(8-i);

	}

	found=0;
	starti = bit_offset & 7;
	trackoffset = bit_offset >> 3;

	tracksize = intput_data_size >> 3;
	if( intput_data_size & 7 ) tracksize++;

	tracksize= tracksize - ( chr_data_size >> 3 );
	if( chr_data_size & 7 ) tracksize--;

	if(searchlen>0)
	{
		searchsize = searchlen >> 3;
		if( searchlen & 7 ) searchsize++;
	}
	else
	{
		searchsize = tracksize;
	}

	t=0;
	// Scan the track data...
	do
	{

		for(i=starti;i<8;i++)
		{
			k = trackoffset;
			j=0;

			while( ( j < cnt ) && ( ( stringtosearch[i][j] & mask[i][j] ) == ( input_data[k] & mask[i][j] ) )  )
			{
				j++;
				k++;
			}

			if( j == cnt )
			{
				found=0xFF;
				bitoffset = ( trackoffset << 3 ) + i;
				i=8;
			}
		}

		trackoffset++;
		t++;

		starti=0;

	}while(!found && (trackoffset<tracksize) && (t<searchsize));

	if(!found)
	{
		bitoffset=-1;
	}

	return bitoffset;
}

int DMK_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{
	int i,j,k,nbsector_mfm,nbsector_fm;
	unsigned char IDAMbuf[0x80];
	FILE * dmkdskfile;
	int tabindex;
	int bit_offset;

	unsigned char * track_buf;
	unsigned char * tmp_fm_track_buf;
	unsigned short tab_ptr_value;
	int databitoffset;
	unsigned char mfm_buffer[16];
	int track_size;

	SECTORSEARCH* ss;
	SECTORCONFIG** sca_mfm;
	SECTORCONFIG** sca_fm;

	dmk_header dmkheader;

	floppycontext->hxc_printf(MSG_INFO_1,"Write DMK file %s...",filename);

	dmkdskfile=hxc_fopen(filename,"wb");
	if(dmkdskfile)
	{
		memset(&dmkheader,0,sizeof(dmkheader));

		track_size = ((floppy->tracks[0]->sides[0]->tracklen / 8)/2);

		dmkheader.track_number = (unsigned char)floppy->floppyNumberOfTrack;
		dmkheader.track_len = track_size + 0x80;

		if(floppy->floppyNumberOfSide == 1)
			dmkheader.flags = 0x10;

		track_buf = malloc(track_size+1);
		if(track_buf)
		{
			memset(track_buf,0xAA,track_size);

			fwrite(&dmkheader,sizeof(dmk_header),1,dmkdskfile);

			ss=hxcfe_initSectorSearch(floppycontext,floppy);
			if(ss)
			{
				for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
				{
					for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
					{

						memset(track_buf,0xAA,track_size);
						memset(&IDAMbuf,0,128);

						nbsector_mfm = 0;
						nbsector_fm = 0;

						sca_mfm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector_mfm);

						sca_fm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector_fm);

						if(nbsector_mfm)
						{
							if(sca_mfm[0]->startsectorindex&1)
							{
								databitoffset = 0;
							}
							else
							{
								databitoffset = 1;
							}

							for(k=0;k<track_size*8*2;k=k+2)
							{
								if(gettrackbit(floppy->tracks[j]->sides[i]->databuffer,k+databitoffset))
								{
									setdmktrackbit(track_buf,k/2,0xFF);
								}
								else
								{
									setdmktrackbit(track_buf,k/2,0x00);
								}
							}

							mfm_buffer[0]=0x44;
							mfm_buffer[1]=0x89;
							mfm_buffer[2]=0x44;
							mfm_buffer[3]=0x89;
							mfm_buffer[4]=0x44;
							mfm_buffer[5]=0x89;
							mfm_buffer[6]=0x55;
							mfm_buffer[7]=0x54;
							bit_offset = -1;
							tabindex = 0;
							do
							{
								bit_offset=searchBitStream(floppy->tracks[j]->sides[i]->databuffer,floppy->tracks[j]->sides[i]->tracklen,-1,mfm_buffer,8*8,bit_offset+1);
								if(bit_offset!=-1)
								{
									if(tabindex<0x40)
									{
										tab_ptr_value = (( (bit_offset + (3*8*2)) / 16 ) + 0x80) & 0x7FFF;

										IDAMbuf[(tabindex*2)] =		 tab_ptr_value & 0xFF;
										IDAMbuf[(tabindex*2 + 1)] = (tab_ptr_value>>8) | 0x80;

										tabindex++;
									}
								}

							}while(bit_offset!=-1);

							k=0;
							do
							{
								hxcfe_freeSectorConfig(ss,sca_mfm[k]);
								k++;
							}while(k<nbsector_mfm);
						}

						if(nbsector_fm)
						{

							tmp_fm_track_buf = malloc(track_size+1);
							if(tmp_fm_track_buf)
							{
								memset(tmp_fm_track_buf,0xAA,track_size);

								databitoffset = ( 3 - sca_fm[0]->startsectorindex&3 );

								for(k=0;k<track_size*8*2;k=k+4)
								{
									if(gettrackbit(floppy->tracks[j]->sides[i]->databuffer,k+databitoffset))
									{
										setdmktrackbit(tmp_fm_track_buf,k/4,0xFF);
									}
									else
									{
										setdmktrackbit(tmp_fm_track_buf,k/4,0x00);
									}
								}

								mfm_buffer[0]=0x55;
								mfm_buffer[1]=0x11;
								mfm_buffer[2]=0x15;
								mfm_buffer[3]=0x54;
								bit_offset = -1;
								tabindex = 0;
								do
								{
									bit_offset=searchBitStream(floppy->tracks[j]->sides[i]->databuffer,floppy->tracks[j]->sides[i]->tracklen,-1,mfm_buffer,4*8,bit_offset+1);
									if(bit_offset!=-1)
									{
										if(tabindex<0x40)
										{
											tab_ptr_value = (( (bit_offset) / 16 ) + 0x80) & 0x7FFF;

											IDAMbuf[(tabindex*2)] =		 tab_ptr_value & 0xFF;
											IDAMbuf[(tabindex*2 + 1)] = (tab_ptr_value>>8);

											tabindex++;
										}
									}
								}while(bit_offset!=-1);

								for(k=0;k<track_size;k=k+2)
								{
									track_buf[k + 0] = tmp_fm_track_buf[k/2];
									track_buf[k + 1] = tmp_fm_track_buf[k/2];
								}

								free(tmp_fm_track_buf);
							}

							k=0;
							do
							{
								hxcfe_freeSectorConfig(ss,sca_fm[k]);
								k++;
							}while(k<nbsector_fm);
						}

						fwrite(&IDAMbuf,128,1,dmkdskfile);
						fwrite(track_buf,track_size,1,dmkdskfile);
					}
				}

				hxcfe_deinitSectorSearch(ss);

			}

			free(track_buf);
		}

		hxc_fclose(dmkdskfile);
	}

	return 0;
}
