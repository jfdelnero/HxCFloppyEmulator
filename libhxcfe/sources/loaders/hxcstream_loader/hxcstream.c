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
// File : hxcstream.c
// Contains: HxC Stream file loader
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

#include "tracks/std_crc32.h"
#include "thirdpartylibs/lz4/lib/lz4.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "hxcstream_loader.h"
#include "hxcstream_format.h"
#include "hxcstream.h"

#include "libhxcadaptor.h"

#define HXCSTREAM_NS_PER_TICK 40000 // 40,000 ns per tick

#define MAX_INDEX 512

typedef struct Index_
{
	uint32_t    StreamPosition;
	uint32_t    Timer;
	uint32_t    SysClk;
	uint32_t    CellPos;
	uint32_t    IndexTime;
	uint32_t    PreIcTime;
	uint32_t    PostIcTime;

	uint32_t    Prev_Index_Tick;
	uint32_t    Next_Index_Tick;
	uint32_t    type;
}Index;

static uint32_t get_tick_from_reversal(uint32_t* buffer,uint32_t reversal)
{
	uint32_t i;
	uint32_t tick;

	tick = 0;

	for(i = 0;i < reversal; i++ )
	{
		tick += buffer[i];
	}

	return tick;
}

uint32_t * conv_stream(uint32_t * trackbuf_dword, unsigned char * unpacked_data, unsigned int unpacked_data_size, uint32_t pulses_count)
{
	unsigned int k,l,p;
	unsigned char c;
	uint32_t tmp_dword;

	if( pulses_count )
	{
		p = 0;
		k = 0;
		l = 0;

		while(l < pulses_count && k < unpacked_data_size)
		{
			c  = unpacked_data[k++];

			if( !(c & 0x80) )
			{
				trackbuf_dword[p++] = c;
			}
			else
			{
				if( (c & 0xC0) == 0x80 )
				{
					tmp_dword = (((uint32_t)(c & 0x3F) << 8) | unpacked_data[k++]);
					trackbuf_dword[p++] = tmp_dword;
				}
				else
				{
					if( (c & 0xE0) == 0xC0 )
					{
						tmp_dword =  ((uint32_t)(c & 0x1F) << 16);
						tmp_dword |= ((uint32_t)unpacked_data[k++]<<8);
						tmp_dword |= ((uint32_t)unpacked_data[k++]<<0);

						trackbuf_dword[p++] = tmp_dword;
					}
					else
					{
						if( (c & 0xF0) == 0xE0 )
						{
							tmp_dword =  ((uint32_t)(c & 0x0F) << 24);
							tmp_dword |= ((uint32_t)unpacked_data[k++]<<16);
							tmp_dword |= ((uint32_t)unpacked_data[k++]<<8);
							tmp_dword |= ((uint32_t)unpacked_data[k++]<<0);

							trackbuf_dword[p++] = tmp_dword;
						}
						else
						{

						}
					}
				}
			}

			l++;
		}

		// dummy pulse
		trackbuf_dword[p++] = 300;

	}

	return trackbuf_dword;
}

HXCFE_TRKSTREAM* DecodeHxCStreamFile(HXCFE* floppycontext,HXCFE_FXSA * fxs,char * file)
{
	chunk_header header;
	unsigned char * hxcstreambuffer;
	FILE* f;
	uint32_t filesize;
	HXCFE_TRKSTREAM * track_dump;
	uint32_t crc32,packet_offset;
	packed_io_header * pio_header;
	packed_stream_header * stream_header;
	metadata_header * metadata;
	unsigned char * metadata_buffer;
	unsigned char * hxcstreambuf;
	uint16_t * iostreambuf, *tmp_w_ptr;
	uint32_t * stream, *tmp_dw_ptr;
	int nb_pulses,total_nb_pulses,old_index,cnt_io,i,j,k,totalcnt;
	int sampleperiod;
	int total_nb_words, nb_words;
	char temp_str[512];
	char name_str[512];
	char * str1,* str2;
	int nbindex,tmp_size;
	Index index_events[MAX_INDEX];
	uint32_t Prev_Max_Index_Tick;
	uint32_t Next_Max_Index_Tick;
	uint32_t totalticks;

	track_dump=0;
	hxcstreambuffer = NULL;
	iostreambuf = NULL;
	stream = NULL;
	metadata = NULL;
	metadata_buffer = NULL;
	str1 = NULL;
	total_nb_pulses = 0;
	total_nb_words = 0;
	nb_words = 0;
	cnt_io = 0;
	sampleperiod = HXCSTREAM_NS_PER_TICK;

	if(fxs)
	{
		f = hxc_fopen(file,"rb");
		if(f)
		{
			filesize = hxc_fgetsize(f);

			while( (uint32_t)ftell(f) < filesize)
			{
				hxc_fread(&header,sizeof(chunk_header),f);

				if(header.header != HXCSTREAM_HEADERSIGN && header.size > filesize)
				{
					hxc_fclose(f);
					return NULL;
				}

				fseek(f, ftell(f) - sizeof(chunk_header), SEEK_SET);

				hxcstreambuffer = malloc( header.size );
				if( hxcstreambuffer )
				{
					hxc_fread(hxcstreambuffer,header.size,f);

					crc32 = std_crc32(0xFFFFFFFF, (void*)hxcstreambuffer, header.size - 4);
					if(crc32 != *((uint32_t*)&hxcstreambuffer[header.size - 4]))
					{
						// BAD CRC !
						floppycontext->hxc_printf(MSG_ERROR,"DecodeHxCStreamFile: Bad CRC !");

						hxc_fclose(f);
						free(hxcstreambuffer);
						return NULL;
					}

					packet_offset = sizeof(chunk_header);

					while( packet_offset < header.size - 4 )
					{
						switch(*((uint32_t*)&hxcstreambuffer[packet_offset]))
						{
							case 0x00000000:
								// metadata
								metadata = (metadata_header *)&hxcstreambuffer[packet_offset];

								if(strstr( ((char*)metadata + sizeof(metadata_header)), "sample_rate_hz 25000000"))
									sampleperiod = 40000;

								if(strstr( ((char*)metadata + sizeof(metadata_header)), "sample_rate_hz 50000000"))
									sampleperiod = 20000;

								if(!metadata_buffer)
								{
									metadata_buffer = calloc( 1, metadata->payload_size + sizeof(chunkblock_header) );
									if( metadata_buffer )
									{
										memcpy( metadata_buffer, (unsigned char*)metadata, metadata->payload_size + sizeof(chunkblock_header) );
									}
								}

								packet_offset += (metadata->payload_size + sizeof(chunkblock_header));
							break;

							case 0x00000001:
								// IO packet
								pio_header = (packed_io_header *)&hxcstreambuffer[packet_offset];

								nb_words = pio_header->unpacked_size;
								tmp_size = total_nb_words + pio_header->unpacked_size;
								if(tmp_size & (sizeof(uint16_t) - 1) )
									tmp_size++;

								tmp_w_ptr = realloc(iostreambuf, tmp_size);
								if(tmp_w_ptr)
								{
									iostreambuf = tmp_w_ptr;
									LZ4_decompress_safe ((const char*)(pio_header) + sizeof(packed_io_header), (char*)&iostreambuf[total_nb_words/2], pio_header->packed_size, pio_header->unpacked_size);
									cnt_io = pio_header->unpacked_size / 2;
								}
								else
								{
									free(iostreambuf);
									iostreambuf = NULL;
								}

								total_nb_words += nb_words;
								cnt_io = total_nb_words / 2;

								packet_offset += (pio_header->payload_size + sizeof(chunkblock_header));
							break;
							case 0x00000002:
								// stream packet
								stream_header = (packed_stream_header *)&hxcstreambuffer[packet_offset];

								hxcstreambuf = malloc(stream_header->unpacked_size);
								if(hxcstreambuf)
								{
									LZ4_decompress_safe ((const char*)(stream_header) + sizeof(packed_stream_header), (char*)hxcstreambuf, stream_header->packed_size, stream_header->unpacked_size);
									nb_pulses = stream_header->number_of_pulses;
									if(nb_pulses > 0 && nb_pulses < (500*1000*1000))
									{
										tmp_dw_ptr = realloc(stream,(total_nb_pulses+nb_pulses+1)*sizeof(uint32_t));
										if(tmp_dw_ptr)
										{
											stream = tmp_dw_ptr;
											memset(&stream[total_nb_pulses],0x00,(nb_pulses+1)*sizeof(uint32_t));
											conv_stream(&stream[total_nb_pulses],hxcstreambuf, stream_header->unpacked_size, nb_pulses);
										}
										else
										{
											free(stream);
											stream = NULL;
										}

										total_nb_pulses += nb_pulses;
									}

									free(hxcstreambuf);
									hxcstreambuf = NULL;
								}

								packet_offset += (stream_header->packed_size + sizeof(packed_stream_header));
							break;
							default:
								floppycontext->hxc_printf(MSG_ERROR,"DecodeHxCStreamFile: Unknown block !");

								// Unknown block !
								hxc_fclose(f);

								free(hxcstreambuffer);
								free(iostreambuf);
								free(stream);

								return NULL;
							break;
						}

						if(packet_offset&3)
							packet_offset = (packet_offset&~3) + 4;
					}

					free(hxcstreambuffer);
					hxcstreambuffer = NULL;
				}
			}

			if(stream && iostreambuf)
			{
				hxcfe_FxStream_setResolution(fxs,sampleperiod);

				track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,total_nb_pulses, HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);

				if( hxcfe_getEnvVarValue( fxs->hxcfe, "FLUXSTREAM_ALL_REVOLUTIONS_IN_ONE" ) )
				{
					hxcfe_FxStream_AddIndex(fxs,track_dump,0,0,FXSTRM_INDEX_MAININDEX);
					hxcfe_FxStream_AddIndex(fxs,track_dump,total_nb_pulses-1,0,FXSTRM_INDEX_MAININDEX);
				}
				else
				{
					j = 0;
					totalticks = 0;
					old_index = 0;
					nbindex = 0;
					for(i=0;i<cnt_io;i++)
					{
						if( (iostreambuf[i]&1) != old_index )
						{
							old_index = iostreambuf[i]&1;

							if(old_index)
							{
								if(nbindex < MAX_INDEX)
								{
									while( totalticks < (i * 16)  && j < total_nb_pulses)
									{
										totalticks += stream[j];
										j++;
									}

									index_events[nbindex].StreamPosition = j;
									index_events[nbindex].SysClk = 0;
									index_events[nbindex].Timer = 0;
									index_events[nbindex].CellPos = j;
									index_events[nbindex].IndexTime = 0;
									index_events[nbindex].PreIcTime = 0;
									index_events[nbindex].PostIcTime = 0;
									index_events[nbindex].Prev_Index_Tick = 0;
									index_events[nbindex].Next_Index_Tick = 0;
									index_events[nbindex].type = FXSTRM_INDEX_MAININDEX;

									nbindex++;
								}
							}
						}
					}

					// Hard sector indexes detection
					if(nbindex > 1)
					{
						for( i = 0; i < nbindex ; i++ )
						{
							if(i)
							{
								index_events[i].Prev_Index_Tick = get_tick_from_reversal(stream,index_events[i].CellPos ) - \
																  get_tick_from_reversal(stream,index_events[i - 1].CellPos );
							}

							if( i < (nbindex - 1))
							{
								index_events[i].Next_Index_Tick = get_tick_from_reversal(stream,index_events[i + 1].CellPos ) - \
																  get_tick_from_reversal(stream,index_events[i].CellPos );
							}
						}
					}

					Prev_Max_Index_Tick = 0;
					Next_Max_Index_Tick = 0;
					for( i = 0; i < nbindex ; i++ )
					{
						if( Prev_Max_Index_Tick < index_events[i].Prev_Index_Tick )
						{
							Prev_Max_Index_Tick = index_events[i].Prev_Index_Tick;
						}

						if( Next_Max_Index_Tick < index_events[i].Next_Index_Tick )
						{
							Next_Max_Index_Tick = index_events[i].Next_Index_Tick;
						}
					}

					// Max index delta is < to 50 ms, load this track as an hard sectored track.
					if ( (Next_Max_Index_Tick * ((float)(sampleperiod) * 10E-10)) < 50 && (Prev_Max_Index_Tick * ((float)(sampleperiod) * 10E-10)) < 50 )
					{
						for( i = 0; i < nbindex ; i++ )
						{
							if ( ( index_events[i].Prev_Index_Tick > ( Prev_Max_Index_Tick / 4 ) * 3 ) &&
								 ( index_events[i].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ) )
							{
								index_events[i].type = FXSTRM_INDEX_SECTORINDEX;
							}
							else
							{
								if ( ( index_events[i].Prev_Index_Tick > ( Prev_Max_Index_Tick / 4 ) * 3 ) ||
									 ( index_events[i].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ) )
								{
									index_events[i].type = FXSTRM_INDEX_SECTORINDEX;
								}
								else
								{
									if( index_events[i].Prev_Index_Tick && index_events[i].Next_Index_Tick )
									{
										index_events[i].type = FXSTRM_INDEX_MAININDEX;
									}
								}
							}
						}

						if( ( index_events[0].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ))
						{
							index_events[0].type = FXSTRM_INDEX_SECTORINDEX;
						}
						else
						{
							if( ( index_events[1].Next_Index_Tick > ( Next_Max_Index_Tick / 4 ) * 3 ))
							{
								index_events[0].type = FXSTRM_INDEX_MAININDEX;
							}
							else
							{
								index_events[0].type = FXSTRM_INDEX_SECTORINDEX;
							}
						}

						if(nbindex)
						{
							if( ( index_events[nbindex-1].Prev_Index_Tick < ( Next_Max_Index_Tick / 4 ) * 3 ))
							{
								if ( ( index_events[nbindex-2].Prev_Index_Tick < ( Prev_Max_Index_Tick / 4 ) * 3 ) &&
									 ( index_events[nbindex-2].Next_Index_Tick < ( Next_Max_Index_Tick / 4 ) * 3 ) )
								{
									index_events[nbindex-1].type = FXSTRM_INDEX_SECTORINDEX;
								}
								else
								{
									index_events[nbindex-1].type = FXSTRM_INDEX_MAININDEX;
								}
							}
							else
							{
								index_events[nbindex-1].type = FXSTRM_INDEX_SECTORINDEX;
							}
						}
					}

					for( i = 0; i < nbindex ; i++ )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Index %d : Prev delta %f ms, Next delta %f ms, Type : 0x%x",i, \
									(float)index_events[i].Prev_Index_Tick * ((float)(sampleperiod) * 10E-10), \
									(float)index_events[i].Next_Index_Tick * ((float)(sampleperiod) * 10E-10), \
									index_events[i].type);

						hxcfe_FxStream_AddIndex(fxs,track_dump,index_events[i].CellPos,index_events[i].Timer,index_events[i].type);
					}
				}

				free(stream);
				stream = NULL;

				for(j=0;j<16;j++)
				{
					sprintf(temp_str,"io_channel %d ",j);

					if(metadata_buffer)
						str1 = strstr( ((char*)metadata_buffer + sizeof(metadata_header)), temp_str);

					if(str1)
					{
						int str_size;
						str2 = strchr(str1,0xA);
						str1 += strlen(temp_str);
						memset(name_str,0,sizeof(name_str));
						if(str2)
						{
							str_size = (str2-str1);
							if(str_size > sizeof(name_str) - 1)
								str_size = sizeof(name_str) - 1;

							strncpy(name_str,str1,str_size);
						}
						else
						{
							strncpy(name_str,str1,sizeof(name_str) - 1);
						}

						old_index = (iostreambuf[0]&(1<<j));
						k = 0;

						for(i=0;i<cnt_io;i++)
						{
							if( (iostreambuf[i]&(1<<j)) != old_index )
							{
								old_index = (iostreambuf[i]&(1<<j));
								k++;
							}
						}

						stream = malloc( ( k + 1 ) * sizeof(uint32_t) );

						if(stream)
						{
							memset(stream,0,( k + 1 ) * sizeof(uint32_t));
							old_index = (iostreambuf[0]&(1<<j));
							totalcnt = 0;
							k = 0;

							for(i=0;i<cnt_io;i++)
							{
								if( (iostreambuf[i]&(1<<j)) != old_index )
								{
									old_index = (iostreambuf[i]&(1<<j));

									stream[k++] = totalcnt*16;

									totalcnt = 0;
								}
								totalcnt++;
							}

							stream[k++] = totalcnt*16;

#if 0
							int t;

							if(k < 16)
							{
								for(t=0;t<k;t++)
								{
									printf(">>%d %d\n",t,stream[t]);
								}
							}

							int total_ticks;
							total_ticks = 0;
							for(t=0;t<k;t++)
							{
								total_ticks += stream[t];
							}

							printf("[chn %d] %d\n",j,total_ticks);
#endif

							floppycontext->hxc_printf(MSG_DEBUG,"DecodeHxCStreamFile: io_channel %d , pulses %d , io_cnt %d",j,k,cnt_io);

							if((iostreambuf[0]&(1<<j)))
								track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,k, HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_1, name_str, track_dump);
							else
								track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,k, HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_0, name_str, track_dump);

							free(stream);
							stream = NULL;
						}
					}
				}
			}

			free(hxcstreambuffer);
			free(iostreambuf);
			free(metadata_buffer);

			hxc_fclose(f);

			return track_dump;
		}
		else
		{
			return NULL;
		}
	}

	return track_dump;
}

HXCFE_TRKSTREAM* hxcfe_FxStream_ImportHxCStreamBuffer(HXCFE_FXSA * fxs,unsigned char * buffer_in,int buffer_size)
{
	chunk_header * header;
	uint32_t filesize;
	HXCFE_TRKSTREAM * track_dump;
	uint32_t crc32,packet_offset,end_packet_offset;
	packed_io_header * pio_header;
	packed_stream_header * stream_header;
	metadata_header * metadata;
	unsigned char * metadata_buffer;
	unsigned char * hxcstreambuf;
	uint16_t * iostreambuf, *tmp_w_ptr;
	uint32_t * stream, *tmp_dw_ptr;
	int nb_pulses,total_nb_pulses,old_index,cnt_io,i,j,k,totalcnt;
	int sampleperiod;
	int total_nb_words, nb_words;
	int buffer_offset;
	int tmp_size;
	char temp_str[512];
	char name_str[512];
	char * str1,* str2;

	buffer_offset = 0;
	track_dump = NULL;
	iostreambuf = NULL;
	stream = NULL;
	metadata = NULL;
	metadata_buffer = NULL;
	str1 = NULL;
	total_nb_pulses = 0;
	total_nb_words = 0;
	nb_words = 0;
	cnt_io = 0;
	sampleperiod = HXCSTREAM_NS_PER_TICK;

	if(fxs)
	{
		filesize = buffer_size;

		while( (uint32_t)buffer_offset < filesize)
		{
			header = (chunk_header*)&buffer_in[buffer_offset];
			buffer_offset += sizeof(chunk_header);

			if(header->header != HXCSTREAM_HEADERSIGN && header->size > filesize)
			{
				break;
			}

			buffer_offset = buffer_offset - sizeof(chunk_header);

			crc32 = std_crc32(0xFFFFFFFF, (void*)&buffer_in[buffer_offset], header->size - 4);
			if(crc32 != *((uint32_t*)&buffer_in[buffer_offset + header->size - 4]))
			{
				// BAD CRC !
				break;
			}


			packet_offset = buffer_offset + sizeof(chunk_header);
			end_packet_offset = buffer_offset + header->size;

			buffer_offset += header->size;


			while( packet_offset < end_packet_offset - 4 )
			{
				switch(*((uint32_t*)&buffer_in[packet_offset]))
				{
					case 0x00000000:
						// metadata
						metadata = (metadata_header *)&buffer_in[packet_offset];

						if(strstr( ((char*)metadata + sizeof(metadata_header)), "sample_rate_hz 25000000"))
							sampleperiod = 40000;

						if(strstr( ((char*)metadata + sizeof(metadata_header)), "sample_rate_hz 50000000"))
							sampleperiod = 20000;

						if(!metadata_buffer)
						{
							metadata_buffer = calloc( 1, metadata->payload_size + sizeof(chunkblock_header) );
							if( metadata_buffer )
							{
								memcpy( metadata_buffer, (unsigned char*)metadata, metadata->payload_size + sizeof(chunkblock_header) );
							}
						}

						packet_offset += (metadata->payload_size + sizeof(chunkblock_header));

					break;

					case 0x00000001:
						// IO packet
						pio_header = (packed_io_header *)&buffer_in[packet_offset];

						nb_words = pio_header->unpacked_size;

						tmp_size = total_nb_words + pio_header->unpacked_size;
						if(tmp_size & 1)
							tmp_size++;

						tmp_w_ptr = realloc(iostreambuf, tmp_size);
						if(tmp_w_ptr)
						{
							iostreambuf = tmp_w_ptr;
							LZ4_decompress_safe ((const char*)(pio_header) + sizeof(packed_io_header), (char*)&iostreambuf[total_nb_words/2], pio_header->packed_size, pio_header->unpacked_size);
							cnt_io = pio_header->unpacked_size / 2;
						}
						else
						{
							free(iostreambuf);
							iostreambuf = NULL;
						}

						total_nb_words += nb_words;
						cnt_io = total_nb_words / 2;

						packet_offset += (pio_header->payload_size + sizeof(chunkblock_header));
					break;
					case 0x00000002:
						// stream packet
						stream_header = (packed_stream_header *)&buffer_in[packet_offset];

						hxcstreambuf = malloc(stream_header->unpacked_size);
						if(hxcstreambuf)
						{
							LZ4_decompress_safe ((const char*)(stream_header) + sizeof(packed_stream_header), (char*)hxcstreambuf, stream_header->packed_size, stream_header->unpacked_size);

							nb_pulses = stream_header->number_of_pulses;

							tmp_dw_ptr = realloc(stream,(total_nb_pulses+nb_pulses+1)*sizeof(uint32_t));
							if(tmp_dw_ptr)
							{
								stream = tmp_dw_ptr;
								memset(&stream[total_nb_pulses],0x00,(nb_pulses+1)*sizeof(uint32_t));
								conv_stream(&stream[total_nb_pulses],hxcstreambuf, stream_header->unpacked_size, nb_pulses);
							}
							else
							{
								free(stream);
								stream = NULL;
							}

							total_nb_pulses += nb_pulses;

							free(hxcstreambuf);
							hxcstreambuf = NULL;
						}

						packet_offset += (stream_header->packed_size + sizeof(packed_stream_header));
					break;
					default:
						// Unknown block !

						free(iostreambuf);
						free(stream);

						return NULL;
					break;
				}

				if(packet_offset&3)
					packet_offset = (packet_offset&~3) + 4;
			}
		}

		if(stream)
		{
			hxcfe_FxStream_setResolution(fxs,sampleperiod);

			track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,total_nb_pulses,HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);

			if( hxcfe_getEnvVarValue( fxs->hxcfe, "FLUXSTREAM_ALL_REVOLUTIONS_IN_ONE" ) )
			{
				hxcfe_FxStream_AddIndex(fxs,track_dump,0,0,FXSTRM_INDEX_MAININDEX);
				hxcfe_FxStream_AddIndex(fxs,track_dump,total_nb_pulses-1,0,FXSTRM_INDEX_MAININDEX);
			}
			else
			{
				j = 0;

				old_index = 0;
				for(i=0;i<cnt_io;i++)
				{
					if( (iostreambuf[i]&1) != old_index )
					{
						old_index = iostreambuf[i]&1;

						if(old_index)
						{
							totalcnt = 0;
							j = 0;
							while(totalcnt < (i*16) && j < total_nb_pulses)
							{
								totalcnt += stream[j];
								j++;
							}
							hxcfe_FxStream_AddIndex(fxs,track_dump,j,0,FXSTRM_INDEX_MAININDEX);
						}
					}
				}
			}

			free(stream);
			stream = NULL;

			for(j=0;j<16;j++)
			{
				sprintf(temp_str,"io_channel %d ",j);

				if(metadata_buffer)
					str1 = strstr( ((char*)metadata_buffer + sizeof(metadata_header)), temp_str);

				if(str1)
				{
					int str_size;
					str2 = strchr(str1,0xA);
					str1 += strlen(temp_str);
					memset(name_str,0,sizeof(name_str));
					if(str2)
					{
						str_size = (str2-str1);
						if(str_size > sizeof(name_str) - 1)
							str_size = sizeof(name_str) - 1;

						strncpy(name_str,str1,str_size);
					}
					else
					{
						strncpy(name_str,str1,sizeof(name_str) - 1);
					}

					old_index = (iostreambuf[0]&(1<<j));
					k = 0;

					for(i=0;i<cnt_io;i++)
					{
						if( (iostreambuf[i]&(1<<j)) != old_index )
						{
							old_index = (iostreambuf[i]&(1<<j));
							k++;
						}
					}

					stream = malloc( ( k + 1 ) * sizeof(uint32_t) );

					if(stream)
					{
						memset(stream,0,( k + 1 ) * sizeof(uint32_t));
						old_index = (iostreambuf[0]&(1<<j));
						totalcnt = 0;
						k = 0;

						for(i=0;i<cnt_io;i++)
						{
							if( (iostreambuf[i]&(1<<j)) != old_index )
							{
								old_index = (iostreambuf[i]&(1<<j));

								stream[k++] = totalcnt*16;

								totalcnt = 0;
							}
							totalcnt++;
						}

						stream[k++] = totalcnt*16;

#if 0
						int t;

						if(k < 16)
						{
							for(t=0;t<k;t++)
							{
								printf(">>%d %d\n",t,stream[t]);
							}
						}

						int total_ticks;
						total_ticks = 0;
						for(t=0;t<k;t++)
						{
							total_ticks += stream[t];
						}

						printf("[chn %d] %d\n",j,total_ticks);
#endif

						fxs->hxcfe->hxc_printf(MSG_DEBUG,"DecodeHxCStreamFile: io_channel %d , pulses %d , io_cnt %d",j,k,cnt_io);

						if((iostreambuf[0]&(1<<j)))
							track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,k, HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_1, name_str, track_dump);
						else
							track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,k, HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_0, name_str, track_dump);

						free(stream);
						stream = NULL;
					}
				}
			}
		}

		free(iostreambuf);
		free(metadata_buffer);

		return track_dump;
	}

	return track_dump;
}
