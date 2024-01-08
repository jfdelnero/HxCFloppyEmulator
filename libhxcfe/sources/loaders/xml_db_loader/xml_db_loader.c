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
// File : xml_db_loader.c
// Contains: XML database floppy format loader
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

#include "xml_db_loader.h"
#include "xml_db_writer.h"

#include "libhxcadaptor.h"

HXCFE_XMLLDR * rfb = 0;

int XMLDB_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	HXCFE_XMLLDR * xmlldr;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"XMLDB_libIsValidDiskFile");

	xmlldr = hxcfe_initXmlFloppy( imgldr_ctx->hxcfe );

	if(xmlldr)
	{

		hxcfe_deinitXmlFloppy( xmlldr );

		return HXCFE_VALIDFILE;

	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"XMLDB_libIsValidDiskFile : Internal error !");
		return HXCFE_INTERNALERROR;
	}
}

int XMLDB_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	HXCFE_XMLLDR* rfb;
	HXCFE_FLOPPY * fp;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"XMLDB_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	hxc_fclose(f);

	if(hxc_checkfileext(imgfile,"xml",SYS_PATH_TYPE) && filesize!=0)
	{
		rfb = hxcfe_initXmlFloppy(imgldr_ctx->hxcfe);
		if(rfb)
		{
			if(hxcfe_setXmlFloppyLayoutFile(rfb,imgfile) == HXCFE_NOERROR)
			{
				if(parameters)
				{
					fp = hxcfe_generateXmlFileFloppy(rfb,(char*)parameters);
				}
				else
				{
					fp = hxcfe_generateXmlFloppy(rfb,0,0);
				}

				if(fp)
				{
					memcpy(floppydisk,fp,sizeof(HXCFE_FLOPPY));
					free(fp);

					hxcfe_deinitXmlFloppy(rfb);

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"XMLDB_libLoad_DiskFile - disk generated !");

					return HXCFE_NOERROR;
				}
			}

			hxcfe_deinitXmlFloppy(rfb);
		}
	}

	return HXCFE_BADFILE;
}

int XMLDB_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static char plug_id[512]="XML_DATABASE_LOADER";
	static char plug_desc[512]="XML Format database Loader";
	static char plug_ext[512]="xml";
	char * tmp_ptr;

	if(imgldr_ctx)
	{
		if(returnvalue)
		{
			switch(infotype)
			{
				case GETPLUGINID:
					*(char**)(returnvalue)=(char*)plug_id;
					break;

				case GETDESCRIPTION:
					*(char**)(returnvalue)=(char*)plug_desc;
					break;

				case GETFUNCPTR:
					///memcpy(returnvalue,pluginfunc,sizeof(plugins_ptr));
					break;

				case GETEXTENSION:
					if(!rfb)
						rfb = hxcfe_initXmlFloppy(imgldr_ctx->hxcfe);

					*(char**)(returnvalue)=(char*)plug_ext;
					break;

				case GETNBSUBLOADER:
					if(!rfb)
						rfb = hxcfe_initXmlFloppy(imgldr_ctx->hxcfe);

					*(int*)(returnvalue) = hxcfe_numberOfXmlLayout(rfb);

					break;

				case SELECTSUBLOADER:
					imgldr_ctx->selected_subid = *((int*)returnvalue);

					if(!rfb)
						rfb = hxcfe_initXmlFloppy(imgldr_ctx->hxcfe);


					if( imgldr_ctx->selected_subid && rfb )
					{
						tmp_ptr = (char*)hxcfe_getXmlLayoutDesc( rfb, imgldr_ctx->selected_subid - 1 );
						if(tmp_ptr)
							strcpy(plug_desc,tmp_ptr);

						tmp_ptr = (char*)hxcfe_getXmlLayoutName( rfb, imgldr_ctx->selected_subid - 1 );
						if(tmp_ptr)
							strcpy(plug_id,tmp_ptr);

						hxcfe_selectXmlFloppyLayout( rfb, imgldr_ctx->selected_subid - 1 );
					}
					break;

				default:
					return HXCFE_BADPARAMETER;
					break;
			}

			return HXCFE_NOERROR;
		}
	}
	return HXCFE_BADPARAMETER;
}
