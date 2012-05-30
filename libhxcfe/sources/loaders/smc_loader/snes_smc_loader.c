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
// File : snes_smc_DiskFile.c
// Contains: SMC rom floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "snes_smc_loader.h"
#include "../fat12floppy_loader/fat12floppy_loader.h"

#include "../fat12floppy_loader/fat12.h"

#include "os_api.h"

extern unsigned char msdos_bootsector;

int snes_smc_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	FILE * f;
	unsigned char * fileheader;
	int fileok;
	
	fileok=1;
	floppycontext->hxc_printf(MSG_DEBUG,"snes_smc_libIsValidDiskFile");
	if(imgfile)
	{
		f=hxc_fopen(imgfile,"r+b");
		if(f)
		{
			fileheader=(unsigned char*)malloc(512);
			if(fileheader)
			{
				fread(fileheader,1,512,f);
				if( fileheader[8]==0xaa && fileheader[9]==0xbb )
				{					
					switch(fileheader[10])
					{
					case 1:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : Super Magic Card saver data");
						break;
					case 2:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : Magic Griffin program (PC-Engine)");
						break;
					case 3:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : Magic Griffin SRAM data");
						break;
					case 4:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : SNES program");
						break;
					case 5:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : SWC & SMC password, SRAM data");
						break;
					case 6:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : Mega Drive program");
						break;
					case 7:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : SMD SRAM data");
						break;
					case 8:
						floppycontext->hxc_printf(MSG_INFO_1,"File type : SWC & SMC saver data");
						break;
					default:
						fileok=0;					
						break;
					}
					
					if(fileok)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"SMC/SMD file !");
						free(fileheader);
						hxc_fclose(f);
						return HXCFE_VALIDFILE;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non SMC/SMD file !");
						free(fileheader);
						hxc_fclose(f);
						return HXCFE_BADFILE;
					}
					
				}
				else
				{
						if(!strncmp(&fileheader[8],"SUPERUFO",8))
						{
							floppycontext->hxc_printf(MSG_INFO_1,"File type : SUPERUFO SMC");
						}
						else
						{
							if( ! checkfileext(imgfile,"smc") )
							{
								floppycontext->hxc_printf(MSG_ERROR,"unknow file type !");
								fileok=0;
							}
							else
							{
								floppycontext->hxc_printf(MSG_INFO_1,"File type : Super Pro Fighter SMC?");
							}
						}

					if(fileok)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"SMC/SMD file !");
						free(fileheader);
						hxc_fclose(f);
						return HXCFE_VALIDFILE;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non SMC/SMD file !");
						free(fileheader);
						hxc_fclose(f);
						return HXCFE_BADFILE;
					}
				}
				
			}
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}
		return HXCFE_ACCESSERROR;
		
	}
	
	return HXCFE_BADPARAMETER;
}



int snes_smc_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	floppycontext->hxc_printf(MSG_DEBUG,"snes_smc_libLoad_DiskFile %s",imgfile);
	
	return FAT12FLOPPY_libLoad_DiskFile(floppycontext,floppydisk,imgfile,".fat4572");
}
	
int snes_smc_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="SNES_SMC";
	static const char plug_desc[]="Super famicom SMC Loader";
	static const char plug_ext[]="smc";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	snes_smc_libIsValidDiskFile,
		(LOADDISKFILE)		snes_smc_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	snes_smc_libGetPluginInfo
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

