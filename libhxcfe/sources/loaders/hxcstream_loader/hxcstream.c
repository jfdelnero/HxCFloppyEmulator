/*
//
// Copyright (C) 2006-2020 Jean-François DEL NERO
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
#include "thirdpartylibs/lz4/lib/lz4lib.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "hxcstream_loader.h"
#include "hxcstream_format.h"
#include "hxcstream.h"

#include "libhxcadaptor.h"

#define HXCSTREAM_NS_PER_TICK 40000 // 40,000 ns per tick

uint32_t * conv_stream(uint32_t * trackbuf_dword, unsigned char * unpacked_data, uint32_t pulses_count)
{
	unsigned int k,l,p;
	unsigned char c;
	uint32_t tmp_dword,cumul;

	if( pulses_count )
	{
		p = 0;

		cumul = 0;

		k = 0;
		l = 0;
		cumul = 0;
		while(l < pulses_count )
		{
			c  = unpacked_data[k++];

			if( !(c & 0x80) )
			{
				trackbuf_dword[p++] = c;
				cumul += c;
			}
			else
			{
				if( (c & 0xC0) == 0x80 )
				{
					tmp_dword = (((uint32_t)(c & 0x3F) << 8) | unpacked_data[k++]);
					trackbuf_dword[p++] = tmp_dword;
					cumul += tmp_dword;
				}
				else
				{
					if( (c & 0xE0) == 0xC0 )
					{
						tmp_dword =  ((uint32_t)(c & 0x1F) << 16);
						tmp_dword |= ((uint32_t)unpacked_data[k++]<<8);
						tmp_dword |= ((uint32_t)unpacked_data[k++]<<0);

						trackbuf_dword[p++] = tmp_dword;
						cumul += tmp_dword;
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
							cumul += tmp_dword;
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

HXCFE_TRKSTREAM* DecodeHxCStreamFile(HXCFE* floppycontext,HXCFE_FXSA * fxs,char * file,float timecoef)
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
	unsigned char * hxcstreambuf;
	uint16_t * iostreambuf;
	uint32_t * stream;
	int nb_pulses,total_nb_pulses,old_index,cnt_io,i,j,totalcnt;
	int sampleperiod;
	int total_nb_words, nb_words;

	track_dump=0;
	hxcstreambuffer = NULL;
	iostreambuf = NULL;
	stream = NULL;
	total_nb_pulses = 0;
	total_nb_words = 0;
	nb_words = 0;
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

				hxcstreambuffer = realloc(hxcstreambuffer,header.size);
				if( hxcstreambuffer )
				{
					hxc_fread(hxcstreambuffer,header.size,f);

					crc32 = std_crc32(0xFFFFFFFF, (void*)hxcstreambuffer, header.size - 4);
					if(crc32 != *((uint32_t*)&hxcstreambuffer[header.size - 4]))
					{
						// BAD CRC !
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

								packet_offset += (metadata->payload_size + sizeof(chunkblock_header));
							break;

							case 0x00000001:
								// IO packet
								pio_header = (packed_io_header *)&hxcstreambuffer[packet_offset];

								nb_words = pio_header->unpacked_size;

								iostreambuf = realloc(iostreambuf, total_nb_words + pio_header->unpacked_size);
								if(iostreambuf)
								{
									LZ4_decompress_safe ((const char*)(pio_header) + sizeof(packed_io_header), (char*)&iostreambuf[total_nb_words/2], pio_header->packed_size, pio_header->unpacked_size);
									cnt_io = pio_header->unpacked_size / 2;
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

									stream = realloc(stream,(total_nb_pulses+nb_pulses+1)*sizeof(uint32_t));
									if(stream)
									{
										memset(&stream[total_nb_pulses],0x00,(nb_pulses+1)*sizeof(uint32_t));
										conv_stream(&stream[total_nb_pulses],hxcstreambuf, nb_pulses);
									}
									free(hxcstreambuf);

									total_nb_pulses += nb_pulses;
								}

								packet_offset += (stream_header->packed_size + sizeof(packed_stream_header));
							break;
							default:
								// Unknown block !
								hxc_fclose(f);

								if(hxcstreambuffer)
									free(hxcstreambuffer);

								if(iostreambuf)
									free(iostreambuf);

								if(stream)
									free(stream);

								return NULL;
							break;
						}

						if(packet_offset&3)
							packet_offset = (packet_offset&~3) + 4;
					}
				}
			}

			if(stream)
			{
				hxcfe_FxStream_setResolution(fxs,sampleperiod);

				track_dump = hxcfe_FxStream_ImportStream(fxs,stream,32,total_nb_pulses);

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

			free(hxcstreambuffer);
			free(iostreambuf);
			free(stream);

			hxc_fclose(f);

			return track_dump;
		}
		else
		{
			hxc_fclose(f);

			return NULL;
		}
	}

	return track_dump;
}
