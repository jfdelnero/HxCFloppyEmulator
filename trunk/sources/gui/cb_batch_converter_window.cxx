/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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


#include <stdio.h>
#include <stdlib.h>
#include "fl_includes.h"
#include "fl_dnd_box.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "libhxcadaptor.h"
}

#include "fl_includes.h"

#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"
#include "about_gui.h"

#include "main.h"
#include "utils.h"
#include "ff_types.h"
#include "batch_converter_window.h"
#include "cb_batch_converter_window.h"
#include "main_gui.h"
#include "loader.h"
#include "cb_rawfile_loader_window.h"

#include "fileselector.h"

#ifdef WIN32
#define SEPARTOR '\\'
#else
#define SEPARTOR '/'
#endif

batchconverterparams bcparams;

extern s_gui_context * guicontext;

typedef struct s_param_bc_params_
{
	char * files;
	batch_converter_window * bcw;
}s_param_bc_params;

ff_type ff_type_list[]=
{
	{ FF_HFE,"HFE - SDCard HxC Floppy Emulator file format",PLUGIN_HXC_HFE,".hfe"},
	{ FF_MFM,"MFM - MFM/FM track file format",PLUGIN_HXC_MFM,".mfm"},
	{ FF_AFI,"AFI - Advanced file image format",PLUGIN_HXC_AFI,".afi"},
	{ FF_VTR,"VTR - VTrucco Floppy Emulator file format",PLUGIN_VTR_IMG,".vtr"},
	{ FF_RAW,"RAW - RAW sectors file format",PLUGIN_RAW_LOADER,".img"},
	{ FF_IMD,"IMD - IMD sectors file format",PLUGIN_IMD_IMG,".imd"},
	{ FF_JV3,"JV3 - JV3 TRS80 file format",PLUGIN_TRS80_JV3,".jv3"},
	{ FF_DMK,"DMK - DMK TRS80 file format",PLUGIN_TRS80_DMK,".dmk"},
	{ FF_V9T9,"TI99/4A V9T9 sectors file format",PLUGIN_TI994A_V9T9,".dsk"},
	{ FF_D88, "D88 - D88 PC88 file format",PLUGIN_NEC_D88,".d88"},
	{ FF_MSA, "MSA - ATARI ST MSA file format",PLUGIN_ATARIST_MSA,".msa"},
	{ FF_HDDDA2_HFE,"HFE - HDDD A2 Support",PLUGIN_HXC_HDDD_A2,".hfe"},
	{ FF_EHFE,"HFE - Rev 2 - Experimental",PLUGIN_HXC_EXTHFE,".hfe"},
	{ -1,"",0,0}			
};

int draganddropconvert(HXCFLOPPYEMULATOR* floppycontext,char ** filelist,char * destfolder,int output_file_format,batchconverterparams * params)
{
	int i,j,filenb,ret;
	char *destinationfile,*tempstr;
	FLOPPY * thefloppydisk;
	int loaderid;
	cfgrawfile rfc;
	Main_Window* mw;
	int disklayout;
	XmlFloppyBuilder* rfb;
	
	filenb=0;
	while(filelist[filenb])
	{		

		if(1)
		{
			if(!params->rawfilemode)
				loaderid=hxcfe_autoSelectLoader(floppycontext,filelist[filenb],0);

			if(loaderid>=0 || params->rawfilemode)
			{

				if(!params->rawfilemode)
				{
					thefloppydisk = hxcfe_floppyLoad(floppycontext,filelist[filenb],loaderid,&ret);
				}
				else
				{
					mw = (Main_Window*)guicontext->main_window;

					disklayout = mw->rawloader_window->choice_disklayout->value();
					if(disklayout>0)
					{
						rfb=hxcfe_initXmlFloppy(guicontext->hxcfe);
						if(rfb)
						{
							hxcfe_selectXmlFloppyLayout(rfb,disklayout-1);
							thefloppydisk = hxcfe_generateXmlFileFloppy (rfb,filelist[filenb]);
							hxcfe_deinitXmlFloppy(rfb);
							ret = 0;
						}
					}
					else
					{
						getWindowState(mw->rawloader_window,&rfc);
						thefloppydisk = loadrawimage(floppycontext,&rfc,filelist[filenb],&ret);
					}
				}

				if(ret!=HXCFE_NOERROR || !thefloppydisk)
				{
					switch(ret)
					{
						case HXCFE_UNSUPPORTEDFILE:
							//printf("Load error!: Image file not yet supported!\n");
						break;
						case HXCFE_FILECORRUPTED:
							//printf("Load error!: File corrupted ? Read error ?\n");
						break;
						case HXCFE_ACCESSERROR:
							//printf("Load error!:  Read file error!\n");
						break;
						default:
							//printf("Load error! error %d\n",ret);
						break;
					}
				}
				else
				{

					j=strlen(filelist[filenb]);
					while(filelist[filenb][j]!=SEPARTOR && j)
					{
						j--;
					}
					if(filelist[filenb][j]==SEPARTOR) j++;
					destinationfile=(char*)malloc(strlen(&filelist[filenb][j])+strlen(destfolder)+2+99);
					sprintf(destinationfile,"%s%c%s",destfolder,SEPARTOR,&filelist[filenb][j]);
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

					loaderid=hxcfe_getLoaderID(floppycontext,(char*)ff_type_list[output_file_format].plug_id);
					if(ret>=0)
					{	
						if(!guicontext->autoselectmode)
						{
							hxcfe_floppySetInterfaceMode(guicontext->hxcfe,thefloppydisk,guicontext->interfacemode);
						}
						
						hxcfe_floppySetDoubleStep(guicontext->hxcfe,thefloppydisk,guicontext->doublestep);

						ret=hxcfe_floppyExport(floppycontext,thefloppydisk,destinationfile,loaderid);
					}

					hxcfe_floppyUnload(floppycontext,thefloppydisk);

					tempstr=(char*)malloc(1024);

					i=strlen(destinationfile);
					do
					{
						i--;
					}while(i && destinationfile[i]!=SEPARTOR);

					if(!ret)
					{
						sprintf(tempstr,"%s created",&destinationfile[i]);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);
						params->numberoffileconverted++;
					}
					else
					{
						sprintf(tempstr,"Error cannot create %s",&destinationfile[i]);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);
					}

					free(destinationfile);
					free(tempstr);
				}
			}
		}
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
	int loaderid;
	Main_Window* mw;
	int disklayout;
	XmlFloppyBuilder* rfb;
	cfgrawfile rfc;

	hfindfile=hxc_find_first_file(folder,file, &FindFileData); 
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
						sprintf(destinationfolder,"%s%c%s",destfolder,SEPARTOR,FindFileData.filename);

						//printf("Creating directory %s\n",destinationfolder);
						hxc_mkdir(destinationfolder);

						fullpath=(unsigned char*)malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
						sprintf((char*)fullpath,"%s%c%s",folder,SEPARTOR,FindFileData.filename);

						floppycontext->hxc_printf(MSG_INFO_1,(char*)"Entering directory %s",FindFileData.filename);

						tempstr=(unsigned char*)malloc(1024);
						sprintf((char*)tempstr,"Entering directory %s",FindFileData.filename);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);

						if(browse_and_convert_directory(floppycontext,(char*)fullpath,destinationfolder,file,output_file_format,params))
						{
							free(destinationfolder);
							free(fullpath);
							free(tempstr);
							hxc_find_close(hfindfile);
							return 1;
						}
						free(destinationfolder);
						free(fullpath);
						floppycontext->hxc_printf(MSG_INFO_1,(char*)"Leaving directory %s",FindFileData.filename);
						
						sprintf((char*)tempstr,"Leaving directory %s",FindFileData.filename);
						params->windowshwd->strout_convert_status->value((const char*)tempstr);
						free(tempstr);
						
					}
				}
				else
				{
					floppycontext->hxc_printf(MSG_INFO_1,(char*)"converting file %s, %dB",FindFileData.filename,FindFileData.size);
					if(FindFileData.size)
					{

						fullpath=(unsigned char*)malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
						sprintf((char*)fullpath,"%s%c%s",folder,SEPARTOR,FindFileData.filename);

						if(!params->rawfilemode)
							loaderid=hxcfe_autoSelectLoader(floppycontext,(char*)fullpath,0);

						if(loaderid>=0 || params->rawfilemode)
						{

							if(!params->rawfilemode)
							{
								thefloppydisk = hxcfe_floppyLoad(floppycontext,(char*)fullpath,loaderid,&ret);
							}
							else
							{
								mw = (Main_Window*)guicontext->main_window;

								disklayout = mw->rawloader_window->choice_disklayout->value();
								if(disklayout>0)
								{
									rfb = hxcfe_initXmlFloppy(guicontext->hxcfe);
									if(rfb)
									{
										hxcfe_selectXmlFloppyLayout(rfb,disklayout-1);
										thefloppydisk = hxcfe_generateXmlFileFloppy (rfb,(char*)fullpath);
										hxcfe_deinitXmlFloppy(rfb);
										ret = 0;
									}
								}
								else
								{
									getWindowState(mw->rawloader_window,&rfc);
									thefloppydisk = loadrawimage(floppycontext,&rfc,(char*)fullpath,&ret);
								}
							}

						}
						else
							ret=loaderid;

						free(fullpath);

						if(ret!=HXCFE_NOERROR || !thefloppydisk)
						{
							switch(ret)
							{
								case HXCFE_UNSUPPORTEDFILE:
									//printf("Load error!: Image file not yet supported!\n");
								break;
								case HXCFE_FILECORRUPTED:
									//printf("Load error!: File corrupted ? Read error ?\n");
								break;
								case HXCFE_ACCESSERROR:
									//printf("Load error!:  Read file error!\n");
								break;
								default:
									//printf("Load error! error %d\n",ret);
								break;
							}
						}
						else
						{
							destinationfile=(char*)malloc(strlen(FindFileData.filename)+strlen(destfolder)+2+99);
							sprintf(destinationfile,"%s%c%s",destfolder,SEPARTOR,FindFileData.filename);
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

							loaderid=hxcfe_getLoaderID(floppycontext,(char*)ff_type_list[output_file_format].plug_id);
							if(loaderid>=0)
							{
								if(!guicontext->autoselectmode)
								{
									hxcfe_floppySetInterfaceMode(guicontext->hxcfe,thefloppydisk,guicontext->interfacemode);
								}

								hxcfe_floppySetDoubleStep(guicontext->hxcfe,thefloppydisk,guicontext->doublestep);

								ret=hxcfe_floppyExport(floppycontext,thefloppydisk,destinationfile,loaderid);
							}
							else
								ret=loaderid;


							hxcfe_floppyUnload(floppycontext,thefloppydisk);

							tempstr=(unsigned char*)malloc(1024);
							
							i=strlen(destinationfile);
							do
							{
								i--;
							}while(i && destinationfile[i]!=SEPARTOR);

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

				bbool=hxc_find_next_file(hfindfile,folder,file,&FindFileData);	
			}
		}

	}

	hxc_find_close(hfindfile);

	return 0;
}

int convertthread(void* floppycontext,void* hw_context)
{

	HXCFLOPPYEMULATOR* floppyem;
	batch_converter_window *bcw;
	char * tempstr;

	floppyem=(HXCFLOPPYEMULATOR*)floppycontext;
	bcw=(batch_converter_window *)hw_context;

	memset(&bcparams,0,sizeof(batchconverterparams));
	strcat((char*)&bcparams.sourcedir,bcw->strin_src_dir->value());
	strcat((char*)&bcparams.destdir,bcw->strin_dst_dir->value());
	bcparams.windowshwd=bcw;

	if(bcw->chkbox_rawinputsfiles->value())
	{
		bcparams.rawfilemode = 1;
	}
	else
	{
		bcparams.rawfilemode = 0;
	}

	if(strlen(bcparams.sourcedir) && strlen(bcparams.destdir))
	{

		browse_and_convert_directory(	guicontext->hxcfe,
										bcparams.sourcedir,
										bcparams.destdir,
										(char*)"*.*",
										bcw->choice_file_format->value(),
										&bcparams);

		tempstr=(char*)malloc(1024);
		sprintf(tempstr,"%d files converted!",(int)bcparams.numberoffileconverted);
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
	s_param_bc_params * bcparams2;
	int filecount,i,j,k;
	char ** filelist;

	floppyem=(HXCFLOPPYEMULATOR*)floppycontext;
	bcparams2=(s_param_bc_params *)hw_context;
	bcw=bcparams2->bcw;

	memset(&bcparams,0,sizeof(batchconverterparams));
	strcat((char*)&bcparams.sourcedir,bcw->strin_src_dir->value());
	strcat((char*)&bcparams.destdir,bcw->strin_dst_dir->value());
	if(bcw->chkbox_rawinputsfiles->value())
	{
		bcparams.rawfilemode = 1;
	}
	else
	{
		bcparams.rawfilemode = 0;
	}
	bcparams.windowshwd=bcw;


	filecount=0;
	i=0;
	while(bcparams2->files[i])
	{
		if(bcparams2->files[i]==0xA)
		{
			filecount++;
		}
		i++;
	}

	filecount++;

	filelist =(char **) malloc(sizeof(char *) * (filecount + 1) );
	memset(filelist,0,sizeof(char *) * (filecount + 1) );

	i=0;
	j=0;
	k=0;
	do
	{
		while(bcparams2->files[i]!=0 && bcparams2->files[i]!=0xA)
		{
			i++;
		};

		filelist[k] = URIfilepathparser((char*)&bcparams2->files[j],i-j);

		i++;
		j=i;

		k++;

	}while(k<filecount);


	if(filecount)
	{
	
		draganddropconvert(	guicontext->hxcfe,
							filelist,
							bcparams.destdir,
							bcw->choice_file_format->value(),
							&bcparams);

		tempstr=(char*)malloc(1024);
		sprintf(tempstr,"%d files converted!",(int)bcparams.numberoffileconverted);
		bcparams.windowshwd->strout_convert_status->value((const char*)tempstr);
		free(tempstr);

		k=0;
		while(filelist[k])
		{
			free(filelist[k]);
			k++;
		}
	}

	free(filelist);

	free(bcparams2->files);
	
	bcw->bt_convert->activate();
	return 0;
}

void batch_converter_window_bt_convert(Fl_Button* bt, void*)
{
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();
	bcw->bt_convert->deactivate();
	hxc_createthread(guicontext->hxcfe,bcw,&convertthread,1);
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

void batch_converter_window_bt_cancel(Fl_Button* bt, void*)
{
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();
	
	bcparams.abort=0xFF;

	bcw->window->hide();
}

void batch_converter_window_inputasrawfile(Fl_Check_Button* cb, void*)
{
	batch_converter_window *bcw;
	Main_Window * mw;
	Fl_Window *dw;

	dw=((Fl_Window*)(cb->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	mw=(Main_Window *)guicontext->main_window;

	if(bcw->chkbox_rawinputsfiles->value())
	{
		mw->rawloader_window->window->show();
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
	
	s_param_bc_params * bcparams;
	
	dw=((Fl_Window*)(o->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(dnd->event() == FL_PASTE)
	{
		if(strlen(dnd->event_text()))
		{
			bcw->bt_convert->deactivate();

			bcparams=(s_param_bc_params *)malloc(sizeof(s_param_bc_params));
			if(bcparams)
			{
				memset(bcparams,0,sizeof(s_param_bc_params));

				bcparams->bcw = bcw;

				bcparams->files = (char*) malloc(strlen(dnd->event_text())+1);
				if(bcparams->files)
				{
					strcpy(bcparams->files,dnd->event_text());

					hxc_createthread(guicontext->hxcfe,bcparams,&draganddropconvertthread,1);
				}
				else
				{
					free(bcparams);
				}
			}
		}
	}
}
