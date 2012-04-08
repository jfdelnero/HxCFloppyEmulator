/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : KryoFluxStream_loader.c
// Contains: KryoFlux Stream floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"

#include "os_api.h"

int KryoFluxStream_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int found,track,side;
	struct stat staterep;
	char * filepath;
	FILE * f;
	s_oob_header oob;

	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libIsValidDiskFile");
	
	if(imgfile)
	{
		if(!stat(imgfile,&staterep))
		{
		
			if(staterep.st_mode&S_IFDIR)
			{
			
				filepath = malloc( strlen(imgfile) + 32 );

				track=0;
				side=0;
				found=0;
				do
				{
					
					sprintf(filepath,"%s\\track%.2d.%d.raw",imgfile,track,side);
					f=fopen(filepath,"rb");
					if(f)
					{
						fread(&oob,sizeof(s_oob_header),1,f);
						if(oob.Sign==OOB_SIGN)
						{
							found=1;
						}
						fclose(f);
					}
					side++;
					if(side>1) 
					{
						side = 0;
						track++;
					}

				}while(track<84);

				free( filepath );

				if(found)
				{
					return HXCFE_VALIDFILE;
				}
				else
				{
					return HXCFE_BADFILE;
				}

			}
		}
	}
	
	
	return HXCFE_BADPARAMETER;
}

int decodestream(HXCFLOPPYEMULATOR* floppycontext,char * file)
{
	s_oob_header * oob;
	s_oob_StreamRead	* streamRead;
	s_oob_StreamEnd		* streamEnd;
	s_oob_DiskIndex		* diskIndex;

	unsigned long * cellstream;
	unsigned long cell_value;
	double mck;
	double sck;
	double ick;
	int cellpos;

	s_oob_DiskIndex  tabindex[16];
	int nbindex;

	char		* tempstr;
	int inc0B;

	int filesize,offset;
	FILE * f;
	FILE * fdbg;


	unsigned char * buffer;
	
	floppycontext->hxc_printf(MSG_DEBUG,"decodestream %s",file);

	fdbg=fopen("g:\\streamdbg.txt","w+");

	f=fopen(file,"rb");
	if(f)
	{
		fseek(f,0,SEEK_END);
		filesize=ftell(f);
		fseek(f,0,SEEK_SET);

		buffer=malloc(filesize);

		fread(buffer,filesize,1,f);

		fclose(f);

		cellstream=(unsigned long*)malloc(filesize*sizeof(unsigned long));
		memset(cellstream,0,filesize*sizeof(unsigned long));

		mck=((18432000 * 73) / 14) / 2;
		sck=mck/2;
		ick=mck/16;

		cell_value=0;
		offset=0;
		nbindex=0;
		cellpos=0;

		do
		{
			switch(buffer[offset])
			{
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x07:
					cell_value = buffer[offset] << 8;
					offset++;
					cell_value = cell_value | buffer[offset];
					offset++;

					if(inc0B)
					{
						cell_value=cell_value+ 0x10000;
						inc0B=0;
					}
					cellstream[cellpos++]=cell_value;
					//fprintf(fdbg,"%f\n",(cell_value*1000000/sck));
				break;
				
				// Nop
				case 0x0A:
					offset++;
				case 0x09:
					offset++;
				case 0x08:
					offset++;
				break;
				//
				
				//0x0B 	Overflow16 	1 	Next cell value is increased by 0×10000 (16-bits). Decoding of *this* cell should continue at next stream position
				case 0x0B:
					inc0B=1;
					offset++;
				break;

				//0x0C 	Value16 	3 	New cell value: Upper 8 bits are offset+1 in the stream, lower 8-bits are offset+2 
				case 0x0C:
					offset++;
					cell_value = buffer[offset] << 8;
					offset++;
					cell_value = cell_value | buffer[offset];
					offset++;

					if(inc0B)
					{
						cell_value=cell_value+ 0x10000;
						inc0B=0;
					}
					cellstream[cellpos++]=cell_value;

					//fprintf(fdbg,"%f\n",(cell_value*1000000/sck));
				break;

				case 0x0D:
					oob=(s_oob_header*)&buffer[offset];
					floppycontext->hxc_printf(MSG_DEBUG,"OOB 0x%.2x 0x%.2x Size:0x%.4x",oob->Sign,oob->Type,oob->Size);
					switch(oob->Type)
					{
						case 0x01:
							floppycontext->hxc_printf(MSG_DEBUG,"---Stream Read---");
							streamRead=	(s_oob_StreamRead*) &buffer[offset + sizeof(s_oob_header) ];
							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X TrTime: 0x%.8X",streamRead->StreamPosition,streamRead->TrTime);
							break;

						case 0x02:
							floppycontext->hxc_printf(MSG_DEBUG,"---Index--- : %d sp:%d",nbindex,cellpos);
							diskIndex=	(s_oob_DiskIndex*) &buffer[offset + sizeof(s_oob_header) ];
							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X SysClk: 0x%.8X Timer: 0x%.8X",diskIndex->StreamPosition,diskIndex->SysClk,diskIndex->Timer);

							tabindex[nbindex].StreamPosition=diskIndex->StreamPosition;
							tabindex[nbindex].SysClk=diskIndex->SysClk;
							tabindex[nbindex].Timer=diskIndex->Timer;
							if(nbindex)
							{
								floppycontext->hxc_printf(MSG_DEBUG,"Delta : %d Rpm : %f ",tabindex[nbindex].SysClk-tabindex[nbindex-1].SysClk,(float)(ick*(float)60)/(float)(tabindex[nbindex].SysClk-tabindex[nbindex-1].SysClk));
							}
							nbindex++;
							break;

						case 0x03:
							floppycontext->hxc_printf(MSG_DEBUG,"---Stream End---");
							streamEnd=	(s_oob_StreamEnd*) &buffer[offset + sizeof(s_oob_header) ];
							floppycontext->hxc_printf(MSG_DEBUG,"StreamPosition: 0x%.8X Result: 0x%.8X",streamEnd->StreamPosition,streamEnd->Result);
							break;

						case 0x04:
							floppycontext->hxc_printf(MSG_DEBUG,"---String---");
							tempstr=malloc(oob->Size+1);
							memset(tempstr,0,oob->Size+1);
							memcpy(tempstr,&buffer[offset + sizeof(s_oob_header)],oob->Size);
							floppycontext->hxc_printf(MSG_DEBUG,"String : %s",tempstr);
		
							free(tempstr);
							
							break;
					}
					offset=offset+oob->Size + sizeof(s_oob_header);
				break;
				
				default:
					cell_value = buffer[offset];

					if(inc0B)
					{
						cell_value=cell_value+ 0x10000;
						inc0B=0;
					}
					cellstream[cellpos++]=cell_value;

					//fprintf(fdbg,"%f\n",(cell_value*1000000/sck));
					offset++;
				break;
			}

		}while(offset<filesize);
		
		free(cellstream);
		fclose(fdbg);
	}

	return 0;
}

int KryoFluxStream_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{	
	FILE * f;
	char * filepath;

	int mintrack,maxtrack;
	int minside,maxside;

	unsigned short i,j;
	unsigned char* trackdata;
	unsigned char  gap3len,interleave,skew,trackformat;
	unsigned short rpm;
	unsigned short sectorsize;
	CYLINDER* currentcylinder;

	int found,track,side;
	struct stat staterep;
	s_oob_header oob;

	int nbtrack,nbside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libLoad_DiskFile");
	
	if(imgfile)
	{
		if(!stat(imgfile,&staterep))
		{
		
			if(staterep.st_mode&S_IFDIR)
			{
			
				filepath = malloc( strlen(imgfile) + 32 );

				track=0;
				side=0;
				found=0;
				
				mintrack=84;
				maxtrack=0;
				minside=1;
				maxside=0;

				do
				{
					
					sprintf(filepath,"%s\\track%.2d.%d.raw",imgfile,track,side);
					f=fopen(filepath,"rb");
					if(f)
					{
						fread(&oob,sizeof(s_oob_header),1,f);
						if(oob.Sign==OOB_SIGN)
						{
							if(mintrack>track) mintrack=track;
							if(maxtrack<track) maxtrack=track;
							if(minside>side) minside=side;
							if(maxside<side) maxside=side;


							found=1;
						}
						fclose(f);
					}
					side++;
					if(side>1) 
					{
						side = 0;
						track++;
					}

				}while(track<84);

				if(!found)
				{
					free( filepath );
					return HXCFE_BADFILE;
				}

				nbside=(maxside-minside)+1;
				nbtrack=(maxtrack-mintrack)+1;

				floppycontext->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);
			
				floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
				floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
				floppydisk->floppyNumberOfTrack=nbtrack;
				floppydisk->floppyNumberOfSide=nbside;
				floppydisk->floppySectorPerTrack=10;
				skew=1;
				gap3len=255;
				sectorsize=512;
				trackformat=ISOFORMAT_DD;
				rpm=300; // normal rpm
				interleave=1;

				trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
	
				floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);


				for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
				{
				
					floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					currentcylinder=floppydisk->tracks[j];
								
					for(i=0;i<floppydisk->floppyNumberOfSide;i++)
					{
					
						sprintf(filepath,"%s\\track%.2d.%d.raw",imgfile,j,i);
						decodestream(floppycontext,filepath);

						//file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
						//	        (sectorsize*(floppydisk->floppySectorPerTrack)*i);
						//eek (f , file_offset , SEEK_SET);
						//ead(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

						currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,2500,-2500);
					}
				}

				free(trackdata);

				floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
				
				return HXCFE_NOERROR;


			}
		}
	}

	return HXCFE_BADFILE;
}

int KryoFluxStream_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="KRYOFLUXSTREAM";
	static const char plug_desc[]="KryoFlux Stream Loader";
	static const char plug_ext[]="raw";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	KryoFluxStream_libIsValidDiskFile,
		(LOADDISKFILE)		KryoFluxStream_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	KryoFluxStream_libGetPluginInfo
	};

	return libGetPluginInfo(
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
