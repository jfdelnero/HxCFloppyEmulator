#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "fdc_ctrl.h"

#include "libhxcfe.h"

#include "tracks/sector_extractor.h"
#include "tracks/crc.h"

#include "tracks/track_formats/centurion_mfm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/mfm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_Centurion_MFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset;
	int tmp_bit_offset;
	int sector_extractor_sm;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char crctable[32];
	int k;

	memset(sector,0,sizeof(HXCFE_SECTCFG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				floppycontext->hxc_printf(MSG_DEBUG, "Looking for Centurion sector mark starting at %d\n", bit_offset);

				mfm_buffer[0] = 0x91;
				mfm_buffer[1] = 0x22;
				mfm_buffer[2] = 0x44;
				mfm_buffer[3] = 0x89;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,4*8,bit_offset);

				if(bit_offset!=-1)
				{
					floppycontext->hxc_printf(MSG_DEBUG, "Found Centurion MFM sector mark at offset %d\n", bit_offset);
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;
			case LOOKFOR_ADDM:
				sector->startsectorindex=bit_offset;

				// Skip over sector mark
				bit_offset = chgbitptr(track->tracklen, bit_offset, 4*8);

				// Read out sector header
				bit_offset = mfmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,4,bit_offset,0);
				sector->cylinder = tmp_buffer[0];
				sector->sector = tmp_buffer[1];
				sector->header_crc = (tmp_buffer[2] << 8) | tmp_buffer[3];

				// Assume CRCs are bad initially
				sector->use_alternate_header_crc = 0xFF;
				sector->use_alternate_data_crc = 0xFF;

				// Calculate CCITT/XMODEM CRC16 over sector header
				CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0x0);
				for(k=0;k<4;k++)
				{
					CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
				}

				if(!CRC16_High && !CRC16_Low)
				{ // crc ok !!!
					floppycontext->hxc_printf(MSG_DEBUG, "Valid Centurion MFM sector header found - Cyl: %d Sect:%d\n", sector->cylinder, sector->sector);
					sector->use_alternate_header_crc = 0;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG, "Bad Centurion MFM sector header found - Cyl: %d, Sect:%d\n", sector->cylinder, sector->sector);
				}

				if(track->timingbuffer)
					sector->bitrate = track->timingbuffer[bit_offset/8];
				else
					sector->bitrate = track->bitrate;

				// Give some room for a write splice
				bit_offset = chgbitptr(track->tracklen, bit_offset, 400);

				// Look for end of gap.
				mfm_buffer[0] = 0xAA;
				mfm_buffer[1] = 0xAA;
				mfm_buffer[2] = 0xAA;
				mfm_buffer[3] = 0xA9;

				tmp_bit_offset = searchBitStream(track->databuffer, track->tracklen, -1, mfm_buffer, 4*8, bit_offset);
				if(tmp_bit_offset != -1)
				{
					// Skip past gap.
					bit_offset = chgbitptr(track->tracklen, tmp_bit_offset, 4*8);

					// Read out sector key.  This seems to be some sort of
					// encryption key identifier.  Only zero has been observed
					// so far which seems to indicate no encryption.
					bit_offset = mfmtobin(track->databuffer, NULL, track->tracklen, tmp_buffer, 1, bit_offset, 0);
					int sector_key = tmp_buffer[0];

					if (sector_key == 0) {
						// Initialize CRC16 that will be computed over data header,
						// payload, and trailing CRC
						CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0x0);

						// Read out data header which is just the sector size
						bit_offset = mfmtobin(track->databuffer, NULL, track->tracklen, tmp_buffer, 2, bit_offset, 0);
						sector->sectorsize = (tmp_buffer[0] << 8) | tmp_buffer[1];
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[0],(unsigned char*)crctable );
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[1],(unsigned char*)crctable );

						sector->startdataindex = bit_offset;

						// Read data bytes
						sector->input_data = (unsigned char*)malloc(sector->sectorsize);
						sector->input_data_index = (int*)malloc(sector->sectorsize*sizeof(int));

						if( sector->input_data && sector->input_data_index)
						{
							memset(sector->input_data, 0, sector->sectorsize);
							memset(sector->input_data_index, 0, sector->sectorsize * sizeof(int));

							bit_offset = mfmtobin(track->databuffer,sector->input_data_index,track->tracklen,sector->input_data,sector->sectorsize,bit_offset,0);

							for(k=0; k < sector->sectorsize; k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, sector->input_data[k],(unsigned char*)crctable );
							}

							// Data CRC is immediately after
							bit_offset = mfmtobin(track->databuffer, NULL, track->tracklen, tmp_buffer, 2, bit_offset, 0);
							sector->data_crc = (tmp_buffer[0] << 8) | tmp_buffer[1];
							CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[0],(unsigned char*)crctable );
							CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[1],(unsigned char*)crctable );

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG, "Valid Centurion MFM data found - Cyl: %d Sect:%d\n", sector->cylinder, sector->sector);
								sector->use_alternate_data_crc = 0;
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG, "Bad Centurion MFM data found - Cyl: %d, Sect:%d\n", sector->cylinder, sector->sector);
							}
						}
						else
						{
							if(sector->input_data)
								free(sector->input_data);

							if(sector->input_data_index)
								free(sector->input_data_index);

							sector->input_data = NULL;
							sector->input_data_index = NULL;

							floppycontext->hxc_printf(MSG_ERROR, "get_next_Centurion_MFM_sector : Allocation error !\n");
							return -1;
						}
					} else {
						floppycontext->hxc_printf(MSG_DEBUG, "Don't know how to decode Centurion MFM sector key %d", sector_key);
					}

					sector->endsectorindex = bit_offset;
					sector_extractor_sm=ENDOFSECTOR;
				} else {
					sector->startdataindex = bit_offset;
					sector->endsectorindex = bit_offset;

					bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );

					sector_extractor_sm=ENDOFSECTOR;
				}
			break;
			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}
