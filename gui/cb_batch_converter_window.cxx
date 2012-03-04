/*
//
// Copyright (C) 2006-2012 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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
// File : cb_batch_converter_window.cxx
// Contains: Batch converter window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "batch_converter_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

#include "fl_dnd_box.h"

extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
	#include "os_api.h"
}



#include "loader.h"

extern HXCFLOPPYEMULATOR * flopemu;

	typedef struct ff_type_
	{
		int id;
		char * name;
		char * plug_id;
		char * ext;
	}ff_type;

	typedef struct batchconverterparams_
	{
		HXCFLOPPYEMULATOR * flopemu;
		//HWND windowshwd;
		batch_converter_window *windowshwd;
		char sourcedir[1024];
		char destdir[1024];
		char **filelist;
		int fileformat;
		unsigned long numberoffileconverted;
		int abort;
	}batchconverterparams;

	enum
	{
		FF_HFE=0,
		FF_MFM,
		FF_AFI,
		FF_VTR,
		FF_RAW,
		FF_IMD,
		FF_EHFE
	};

	ff_type ff_type_list[]=
	{
		{ FF_HFE,"HFE - SDCard HxC Floppy Emulator file format",PLUGIN_HXC_HFE,".hfe"},
		{ FF_MFM,"MFM - MFM/FM track file format",PLUGIN_HXC_MFM,".mfm"},
		{ FF_AFI,"AFI - Advanced file image format",PLUGIN_HXC_AFI,".afi"},
		{ FF_VTR,"VTR - VTrucco Floppy Emulator file format",PLUGIN_VTR_IMG,".vtr"},
		{ FF_RAW,"RAW - RAW sectors file format",PLUGIN_RAW_IMG,".img"},
		{ FF_IMD,"IMD - IMD sectors file format",PLUGIN_IMD_IMG,".imd"},
		{ FF_EHFE,"HFE - Rev 2 - Experimental",PLUGIN_HXC_EXTHFE,".hfe"},
		{ -1,"",0,0}			
	};


int draganddropconvert(HXCFLOPPYEMULATOR* floppycontext,char ** filelist,char * destfolder,int output_file_format,batchconverterparams * params)
{
	int i,j,filenb,ret;
	char *destinationfile,*tempstr;
	FLOPPY * thefloppydisk;
	
	filenb=0;
	while(filelist[filenb])
	{		
		hxcfe_selectContainer(floppycontext,"AUTOSELECT");
		thefloppydisk=hxcfe_floppyLoad(floppycontext,filelist[filenb],&ret);
		if(ret!=HXCFE_NOERROR || !thefloppydisk)
		{
			switch(ret)
			{
				case HXCFE_UNSUPPORTEDFILE:
					printf("Load error!: Image file not yet supported!\n");
				break;
				case HXCFE_FILECORRUPTED:
					printf("Load error!: File corrupted ? Read error ?\n");
				break;
				case HXCFE_ACCESSERROR:
					printf("Load error!:  Read file error!\n");
				break;
				default:
					printf("Load error! error %d\n",ret);
				break;
			}
		}
		else
		{

			j=strlen(filelist[filenb]);
			while(filelist[filenb][j]!='\\' && j)
			{
				j--;
			}
			if(filelist[filenb][j]=='\\') j++;
			destinationfile=(char*)malloc(strlen(&filelist[filenb][j])+strlen(destfolder)+2+99);
			sprintf(destinationfile,"%s\\%s",destfolder,&filelist[filenb][j]);
			i=strlen(destinationfile);
			
			do
			{
				i--;
			}while(i && destinationfile[i]!='.');

			if(i)
			{
				destinationfile[i]='_';
			}
			ret=1;
			
			strcat(destinationfile,ff_type_list[output_file_format].ext);

			ret=hxcfe_selectContainer(floppycontext,ff_type_list[output_file_format].plug_id);
			if(!ret)
			{			
			//	hxcfe_floppyGetSetParams(floppycontext,thefloppydisk,SET,DOUBLESTEP,&hwif->double_step);
			//	hxcfe_floppyGetSetParams(floppycontext,thefloppydisk,SET,INTERFACEMODE,&hwif->interface_mode);
				ret=hxcfe_floppyExport(floppycontext,thefloppydisk,destinationfile);
			}

			hxcfe_floppyUnload(floppycontext,thefloppydisk);


			tempstr=(char*)malloc(1024);
						
			i=strlen(destinationfile);
			do
			{
				i--;
			}while(i && destinationfile[i]!='\\');

			if(!ret)
			{
				sprintf(tempstr,"%s created",&destinationfile[i]);
				//SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
				params->numberoffileconverted++;
			}
			else
			{
				sprintf(tempstr,"Error cannot create %s",&destinationfile[i]);
				//SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
			}

			free(destinationfile);
			free(tempstr);
				

		}
		
		free(filelist[filenb]);
		filenb++;
	}	

	return 0;
}

int browse_and_convert_directory(HXCFLOPPYEMULATOR* floppycontext,char * folder,char * destfolder,char * file,int output_file_format,batchconverterparams * params)
{
	long hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int ret,i;
	char * destinationfolder;
	char * destinationfile;
	FLOPPY * thefloppydisk;
	unsigned char * fullpath;
	unsigned char * tempstr;


	hfindfile=find_first_file(folder,file, &FindFileData); 
	if(hfindfile!=-1)
	{
		bbool=1;
		while(hfindfile!=-1 && bbool && !params->abort)
		{
			if(!params->abort)
			{

				if(FindFileData.isdirectory)
				{
					if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
					{
						destinationfolder=(char*)malloc(strlen(FindFileData.filename)+strlen(destfolder)+2);
						sprintf(destinationfolder,"%s\\%s",destfolder,FindFileData.filename);

						printf("Creating directory %s\n",destinationfolder);
						mkdir(destinationfolder);

						fullpath=(unsigned char*)malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
						sprintf((char*)fullpath,"%s\\%s",folder,FindFileData.filename);

						floppycontext->hxc_printf(MSG_INFO_1,"Entering directory %s",FindFileData.filename);

						tempstr=(unsigned char*)malloc(1024);
						sprintf((char*)tempstr,"Entering directory %s",FindFileData.filename);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);

						if(browse_and_convert_directory(floppycontext,(char*)fullpath,destinationfolder,file,output_file_format,params))
						{
							free(destinationfolder);
							free(fullpath);
							free(tempstr);
							find_close(hfindfile);
							return 1;
						}
						free(destinationfolder);
						free(fullpath);
						floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);
						
						sprintf((char*)tempstr,"Leaving directory %s",FindFileData.filename);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);
						free(tempstr);
						
					}
				}
				else
				{			
					floppycontext->hxc_printf(MSG_INFO_1,"converting file %s, %dB",FindFileData.filename,FindFileData.size);
					if(FindFileData.size)
					{

						fullpath=(unsigned char*)malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
						sprintf((char*)fullpath,"%s\\%s",folder,FindFileData.filename);

						thefloppydisk=hxcfe_floppyLoad(floppycontext,(char*)fullpath,&ret);
						free(fullpath);
						if(ret!=HXCFE_NOERROR || !thefloppydisk)
						{
							switch(ret)
							{
								case HXCFE_UNSUPPORTEDFILE:
									printf("Load error!: Image file not yet supported!\n");
								break;
								case HXCFE_FILECORRUPTED:
									printf("Load error!: File corrupted ? Read error ?\n");
								break;
								case HXCFE_ACCESSERROR:
									printf("Load error!:  Read file error!\n");
								break;
								default:
									printf("Load error! error %d\n",ret);
								break;
							}
						}
						else
						{
							destinationfile=(char*)malloc(strlen(FindFileData.filename)+strlen(destfolder)+2+99);
							sprintf(destinationfile,"%s\\%s",destfolder,FindFileData.filename);
							i=strlen(destinationfile);
							do
							{
								i--;
							}while(i && destinationfile[i]!='.');

							if(i)
							{
							  destinationfile[i]='_';
							}
						
							//printf("Creating file %s\n",destinationfile);
							strcat(destinationfile,ff_type_list[output_file_format].ext);							

							ret=hxcfe_selectContainer(floppycontext,ff_type_list[output_file_format].plug_id);
							if(!ret)
							{			
								//	hxcfe_floppyGetSetParams(floppycontext,thefloppydisk,SET,DOUBLESTEP,&hwif->double_step);
								//	hxcfe_floppyGetSetParams(floppycontext,thefloppydisk,SET,INTERFACEMODE,&hwif->interface_mode);
								ret=hxcfe_floppyExport(floppycontext,thefloppydisk,destinationfile);
							}

							hxcfe_floppyUnload(floppycontext,thefloppydisk);
							hxcfe_selectContainer(floppycontext,PLUGIN_AUTOSELECT);

							tempstr=(unsigned char*)malloc(1024);
							
							i=strlen(destinationfile);
							do
							{
								i--;
							}while(i && destinationfile[i]!='\\');

							if(!ret)
							{
								sprintf((char*)tempstr,"%s created",&destinationfile[i]);
								params->windowshwd->strout_convert_status->value((const char*)tempstr);
								params->numberoffileconverted++;
							}
							else
							{
								sprintf((char*)tempstr,"Error cannot create %s",&destinationfile[i]);
								params->windowshwd->strout_convert_status->value((const char*)tempstr);
							}

							free(destinationfile);
							free(tempstr);
					
						}
					}	
				}
			
				bbool=find_next_file(hfindfile,folder,file,&FindFileData);	
			}
		}
		
	}
	else printf("Error FindFirstFile\n");
	
	find_close(hfindfile);
	
	return 0;
}

int convertthread(void* floppycontext,void* hw_context)
{
	
	HXCFLOPPYEMULATOR* floppyem;
	batch_converter_window *bcw;
	batchconverterparams bcparams;
	char * tempstr;

	floppyem=(HXCFLOPPYEMULATOR*)floppycontext;
	bcw=(batch_converter_window *)hw_context;

	memset(&bcparams,0,sizeof(batchconverterparams));
	strcat((char*)&bcparams.sourcedir,bcw->strin_src_dir->value());
	strcat((char*)&bcparams.destdir,bcw->strin_dst_dir->value());
	bcparams.windowshwd=bcw;
	
	if(strlen(bcparams.sourcedir) && strlen(bcparams.destdir))
	{
	
		browse_and_convert_directory(	flopemu,
										bcparams.sourcedir,
										bcparams.destdir,
										"*.*",
										bcw->choice_file_format->value(),
										&bcparams);

		tempstr=(char*)malloc(1024);
		sprintf(tempstr,"%d files converted!",bcparams.numberoffileconverted);
		bcparams.windowshwd->strout_convert_status->value((const char*)tempstr);
		free(tempstr);
	}
	
	bcw->bt_convert->activate();
	return 0;
}

int draganddropconvertthread(void* floppycontext,void* hw_context)
{
	
	HXCFLOPPYEMULATOR* floppyem;
	batch_converter_window *bcw;
	batchconverterparams bcparams;
	char * tempstr;

	floppyem=(HXCFLOPPYEMULATOR*)floppycontext;
	bcw=(batch_converter_window *)hw_context;

	memset(&bcparams,0,sizeof(batchconverterparams));
	strcat((char*)&bcparams.sourcedir,bcw->strin_src_dir->value());
	strcat((char*)&bcparams.destdir,bcw->strin_dst_dir->value());
	bcparams.windowshwd=bcw;
	
	if(strlen(bcparams.destdir))
	{
	
/*		draganddropconvert(	flopemu,
							char ** filelist,
							bcparams.destdir,
							bcw->choice_file_format->value(),
							&bcparams)*/

		tempstr=(char*)malloc(1024);
		sprintf(tempstr,"%d files converted!",bcparams.numberoffileconverted);
		bcparams.windowshwd->strout_convert_status->value((const char*)tempstr);
		free(tempstr);
	}
	
	bcw->bt_convert->activate();
	return 0;
}


int select_dir(char * title,char * str) 
{
	char * dir;
	Fl_Native_File_Chooser fnfc;
  
	fnfc.title(title);
	fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	fnfc.filter("\t*.*\n");
	// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
		{
			break; // ERROR
		}
		case 1:
		{
			break; // CANCEL
		}
		default:
		{			
			dir=(char*)fnfc.filename();
			sprintf(str,"%s",dir);
			return 0;
			break; // FILE CHOSEN
		}
	}
	return -1;
}


void batch_converter_window_bt_convert(Fl_Button* bt, void*)
{
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();
	bcw->bt_convert->deactivate();
	hxc_createthread(flopemu,bcw,&convertthread,1);
}

void batch_converter_window_bt_select_src(Fl_Button* bt, void*)
{
	char dirstr[512];
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(!select_dir((char*)"Select source",(char*)&dirstr))
	{
		bcw->strin_src_dir->value(dirstr);
	}
}

void batch_converter_window_bt_select_dst(Fl_Button* bt, void*)
{
	char dirstr[512];
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(!select_dir((char*)"Select destination",(char*)&dirstr))
	{
		bcw->strin_dst_dir->value(dirstr);
	}
}


void dnd_bc_conv(const char *urls)
{
	//loadfloppy((char*)urls);
}

void dnd_bc_cb(Fl_Widget *o, void *v)
{
	batch_converter_window *bcw;
	Fl_Window *dw;
	Fl_DND_Box *dnd = (Fl_DND_Box*)o;

	dw=((Fl_Window*)(o->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(dnd->event() == FL_PASTE)
	{
//	dnd_bc_conv(dnd->event_text());
		bcw->bt_convert->deactivate();
		hxc_createthread(flopemu,bcw,&draganddropconvertthread,1);

	}
}