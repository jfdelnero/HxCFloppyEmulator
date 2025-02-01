/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fl_includes.h"
#include "fl_dnd_box.h"

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#include "fl_includes.h"

#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "floppy_streamer_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "edittool_window.h"
#include "parameters_gui.h"

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

#include "plugins_id.h"

batchconverterparams bcparams;
int abort_trigger;
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
	{ FF_ADF,"ADF - ADF sectors file format",PLUGIN_AMIGA_ADF,".adf"},
	{ FF_ADZ,"ADZ - ADZ sectors file format",PLUGIN_AMIGA_ADZ,".adz"},
	{ FF_JV3,"JV3 - JV3 TRS80 file format",PLUGIN_TRS80_JV3,".jv3"},
	{ FF_DMK,"DMK - DMK TRS80 file format",PLUGIN_TRS80_DMK,".dmk"},
	{ FF_VDK,"VDK - VDK Dragon format",PLUGIN_DRAGON3264_VDK,".vdk"},
	{ FF_TRD,"TRD - TRD Zx Spectrum format",PLUGIN_ZXSPECTRUM_TRD,".trd"},
	{ FF_SDDSPECCY,"SDD - SDD Speccy DOS file format",PLUGIN_SPECCYSDD,".sdd"},
	{ FF_V9T9,"TI99/4A V9T9 sectors file format",PLUGIN_TI994A_V9T9,".dsk"},
	{ FF_D88, "D88 - D88 PC88 file format",PLUGIN_NEC_D88,".d88"},
	{ FF_ST, "ST - ATARI ST ST file format",PLUGIN_ATARIST_ST,".st"},
	{ FF_MSA, "MSA - ATARI ST MSA file format",PLUGIN_ATARIST_MSA,".msa"},
	{ FF_DIM, "DIM - ATARI ST DIM file format",PLUGIN_ATARIST_DIM,".dim"},
	{ FF_STX, "STX - ATARI ST STX file format",PLUGIN_ATARIST_STX,".stx"},
	{ FF_STW, "STW - ATARI ST STW file format",PLUGIN_ATARIST_STW,".stw"},
	{ FF_FD, "FD - Thomson file image format",PLUGIN_THOMSON_FD,".fd"},
	{ FF_AMSTRADDSK, "DSK - Amstrad CPC DSK file format",PLUGIN_AMSTRADCPC_DSK,".dsk"},
	{ FF_ORICDSK, "DSK - Oric DSK file format",PLUGIN_ORIC_DSK,".dsk"},
	{ FF_HDDDA2_HFE,"HFE - HDDD A2 Support",PLUGIN_HXC_HDDD_A2,".hfe"},
	{ FF_EHFE,"HFE - Rev 2 - Experimental",PLUGIN_HXC_EXTHFE,".hfe"},
	{ FF_HFEV3,"HFE - Rev 3 - Experimental",PLUGIN_HXC_HFEV3,".hfe"},
	{ FF_HFESTREAM," Stream HFE - Rev 0 - Experimental",PLUGIN_HXC_HFESTREAM,".hfe"},
	{ FF_ARBG,"Arburg - Arburg RAW sectors file format",PLUGIN_ARBURG,".arburgfd"},
	{ FF_SKF,"Raw - Stream Kryoflux file format",PLUGIN_SKF,".raw"},
	{ FF_IPF,"IPF - IPF file format (W.I.P)",PLUGIN_IPF,".ipf"},
	{ FF_SCP,"SCP - SCP file format",PLUGIN_SCP,".scp"},
	{ FF_BMP,"Tracks BMP - BMP file image",PLUGIN_BMP,".bmp"},
	{ FF_PNG,"Tracks PNG - PNG file image",PLUGIN_PNG,".png"},
	{ FF_STREAM_BMP,"Stream Tracks BMP - BMP file image",PLUGIN_STREAM_BMP,".bmp"},
	{ FF_STREAM_PNG,"Stream Tracks PNG - PNG file image",PLUGIN_STREAM_PNG,".png"},
	{ FF_DISK_BMP,"Disk BMP - BMP file image",PLUGIN_DISK_BMP,".bmp"},
	{ FF_DISK_PNG,"Disk PNG - PNG file image",PLUGIN_DISK_PNG,".png"},
	{ FF_XML,"XML - XML file image",PLUGIN_GENERIC_XML,".xml"},
	{ FF_NORTHSTAR,"NSI - Northstar file image",PLUGIN_NORTHSTAR,".nsi"},
	{ FF_HEATHKIT,"H8D - Heathkit file image",PLUGIN_HEATHKIT,".h8d"},
	{ FF_HXCQD,"QD - HxC Quickdisk file image",PLUGIN_HXC_QD,".qd"},
	{ FF_APPLEII_DO,"DO - Apple II file image (Dos 3.3)",PLUGIN_APPLE2_DO,".do"},
	{ FF_APPLEII_PO,"PO - Apple II file image (ProDos)",PLUGIN_APPLE2_PO,".po"},
	{ FF_FDX68_RAWFDX,"FDX - FDX68 raw file image (ProDos)",PLUGIN_FDX68_FDX,".fdx"},
	{ -1,"",0,0}
};

void batchconverter_set_progress_status(s_gui_context * guicontext, char * str, Fl_Color color, float value)
{
	Main_Window * mw;

	Fl::lock();

	if(!guicontext)
		goto error;

	mw = (Main_Window*) guicontext->main_window;

	if(!mw)
		goto error;

	mw->batchconv_window->progress_indicator->selection_color(color);
	mw->batchconv_window->progress_indicator->label(str);

	if(value>=0)
	{
		mw->batchconv_window->progress_indicator->value(value);
	}

	Fl::unlock();

	return;

error:
	Fl::unlock();

	return;

}

void batchconverter_set_status_string(batchconverterparams * params, char * str)
{
	Fl::lock();

	if(!params)
		goto error;

	if(!params->windowshwd)
		goto error;

	if(!params->windowshwd->strout_convert_status)
		goto error;

	params->windowshwd->strout_convert_status->value((const char*)str);

	Fl::unlock();

	return;

error:
	Fl::unlock();

	return;
}

int progress_callback_bc(unsigned int current,unsigned int total, void * user)
{
	s_gui_context * guicontext;
	Main_Window * mw;

	guicontext = (s_gui_context *)user;
	mw =(Main_Window*) guicontext->main_window;

	if(total)
	{
		Fl::lock();
		mw->batchconv_window->progress_indicator->value(((float)current/(float)total)*100);
		Fl::unlock();
	}

	return 0;
}

int draganddropconvert(HXCFE* floppycontext,char ** filelist,char * destfolder,int output_file_format,batchconverterparams * params)
{
	int i,j,filenb,ret;
	char *destinationfile,* tempstr;
	HXCFE_FLOPPY * thefloppydisk;
	int loaderid,keepsrcext;
	cfgrawfile rfc;
	Main_Window* mw;
	int disklayout,path_str_size;
	HXCFE_XMLLDR* rfb;
	HXCFE_IMGLDR* imgldr_ctx;

	mw =(Main_Window*) guicontext->main_window;

	tempstr = (char*)malloc(MAX_TMP_STR_SIZE);
	if(!tempstr)
	{
		batchconverter_set_progress_status(guicontext,(char*)"Error", fl_rgb_color(255,10,10), -1);
		batchconverter_set_status_string(params, (char*)"Alloc Error !");

		return 0;
	}

	memset(tempstr,0,MAX_TMP_STR_SIZE);

	keepsrcext = hxcfe_getEnvVarValue( floppycontext, (char*)"BATCHCONVERT_KEEP_SOURCE_FILE_NAME_EXTENSION");

	thefloppydisk = 0;
	imgldr_ctx = hxcfe_imgInitLoader( floppycontext );
	if(imgldr_ctx)
	{
		hxcfe_imgSetProgressCallback(imgldr_ctx,progress_callback_bc,(void*)guicontext);

		filenb=0;
		while(filelist[filenb] && !abort_trigger)
		{
			batchconverter_set_progress_status(guicontext,(char*)"Reading", fl_rgb_color(50,255,50), -1);

			if(filelist[filenb])
			{
				snprintf((char*)tempstr,MAX_TMP_STR_SIZE,"%s",hxc_getfilenamebase(filelist[filenb],0,SYS_PATH_TYPE));
				batchconverter_set_status_string(params, tempstr);
			}

			if(!params->rawfilemode)
				loaderid = hxcfe_imgAutoSetectLoader(imgldr_ctx,filelist[filenb],0);
			else
				loaderid = 0;

			if(loaderid>=0 || params->rawfilemode)
			{
				if(!params->rawfilemode)
				{
					thefloppydisk = hxcfe_imgLoad(imgldr_ctx,filelist[filenb],loaderid,&ret);
				}
				else
				{
					mw = (Main_Window*)guicontext->main_window;

					disklayout = mw->rawloader_window->choice_disklayout->value();
					if(disklayout>=1)
					{
						rfb = hxcfe_initXmlFloppy(guicontext->hxcfe);
						if(rfb)
						{
							if(disklayout ==  1)
								hxcfe_setXmlFloppyLayoutFile(rfb,guicontext->xml_file_path);
							else
								hxcfe_selectXmlFloppyLayout(rfb,disklayout-2);
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
					batchconverter_set_progress_status(guicontext,(char*)"Error", fl_rgb_color(255,10,10), -1);

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
					batchconverter_set_progress_status(guicontext,(char*)"Writing", fl_rgb_color(220,255,10), -1);

					j=strlen(filelist[filenb]);
					while(filelist[filenb][j]!=DIR_SEPARATOR_CHAR && j)
					{
						j--;
					}

					if( ( (loaderid != hxcfe_imgGetLoaderID(imgldr_ctx,(char*)PLUGIN_SKF)) &&
						  (loaderid != hxcfe_imgGetLoaderID(imgldr_ctx,(char*)PLUGIN_PAULINE_STREAM)) ) || !j)
					{
						if(filelist[filenb][j]==DIR_SEPARATOR_CHAR)
							j++;
					}
					else
					{
						//Keep the parent folder name for KF stream file...
						if(j)
						{
							if(filelist[filenb][j]==DIR_SEPARATOR_CHAR)
								filelist[filenb][j] = '_';

							while(filelist[filenb][j]!=DIR_SEPARATOR_CHAR && j)
							{
								j--;
							}
							if(filelist[filenb][j]==DIR_SEPARATOR_CHAR) j++;
						}
					}

					path_str_size = strlen(&filelist[filenb][j]) + strlen(destfolder) + strlen(ff_type_list[output_file_format].ext) + 2;
					destinationfile = (char*)malloc(path_str_size);
					if(!destinationfile)
						goto error;

					snprintf(destinationfile,path_str_size,"%s%c%s",destfolder,DIR_SEPARATOR_CHAR,&filelist[filenb][j]);

					i=strlen(destinationfile);
					do
					{
						i--;
					}while(i && destinationfile[i]!='.');

					if(i)
					{
						if(keepsrcext)
							destinationfile[i]='_';
						else
							destinationfile[i] = 0;
					}

					ret=1;

					strcat(destinationfile,ff_type_list[output_file_format].ext);

					snprintf((char*)tempstr,MAX_TMP_STR_SIZE,"%s",hxc_getfilenamebase(destinationfile,0,SYS_PATH_TYPE));
					batchconverter_set_status_string(params, tempstr);

					loaderid = hxcfe_imgGetLoaderID(imgldr_ctx,(char*)ff_type_list[output_file_format].plug_id);
					if(ret>=0)
					{
						if(!guicontext->autoselectmode)
						{
							hxcfe_floppySetInterfaceMode(guicontext->hxcfe,thefloppydisk,guicontext->interfacemode);
						}

						hxcfe_floppySetDoubleStep(guicontext->hxcfe,thefloppydisk,guicontext->doublestep);

						ret = hxcfe_imgExport(imgldr_ctx,thefloppydisk,destinationfile,loaderid);
					}

					hxcfe_imgUnload(imgldr_ctx,thefloppydisk);
					thefloppydisk = NULL;

					if(!ret)
					{
						snprintf(tempstr,MAX_TMP_STR_SIZE,"%s created",hxc_getfilenamebase(destinationfile,NULL,SYS_PATH_TYPE));
						batchconverter_set_status_string(params, tempstr);
						params->numberoffileconverted++;
					}
					else
					{
						snprintf(tempstr,MAX_TMP_STR_SIZE,"Error cannot create %s",hxc_getfilenamebase(destinationfile,NULL,SYS_PATH_TYPE));
						batchconverter_set_status_string(params, tempstr);
					}

					free(destinationfile);
				}
			}

			filenb++;

		}

		batchconverter_set_progress_status(guicontext,(char*)"Done", fl_rgb_color(80,80,255), 100);

		hxcfe_imgDeInitLoader( imgldr_ctx );
	}

	free(tempstr);

	return 0;

error:
	if( tempstr )
		free( tempstr );

	if( imgldr_ctx )
		hxcfe_imgDeInitLoader( imgldr_ctx );

	if( thefloppydisk )
		hxcfe_imgUnload(imgldr_ctx,thefloppydisk);

	return -1;
}

int browse_and_convert_directory(HXCFE* floppycontext,char * folder,char * destfolder,char * file,int output_file_format,batchconverterparams * params)
{
	void* hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int ret,i,path_str_size;
	char * destinationfolder;
	char * destinationfile;
	HXCFE_FLOPPY * thefloppydisk;
	unsigned char * fullpath;
	char * tempstr;
	int loaderid,keepsrcext;
	Main_Window* mw;
	int disklayout;
	HXCFE_XMLLDR* rfb;
	cfgrawfile rfc;
	HXCFE_IMGLDR * imgldr_ctx;

	mw =(Main_Window*) guicontext->main_window;

	loaderid = -1;
	thefloppydisk = 0;

	imgldr_ctx = hxcfe_imgInitLoader( floppycontext );
	if(imgldr_ctx)
	{

		keepsrcext = hxcfe_getEnvVarValue( floppycontext, (char*)"BATCHCONVERT_KEEP_SOURCE_FILE_NAME_EXTENSION");

		hxcfe_imgSetProgressCallback(imgldr_ctx,progress_callback_bc,(void*)guicontext);

		hfindfile = hxc_find_first_file(folder,file, &FindFileData);
		if(hfindfile)
		{
			bbool=1;
			while(hfindfile && bbool && !abort_trigger)
			{
				if(!abort_trigger)
				{
					if(FindFileData.isdirectory)
					{
						if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
						{
							path_str_size = strlen(FindFileData.filename) + strlen(destfolder) + 2;

							destinationfolder=(char*)malloc(path_str_size);
							if(!destinationfolder)
								goto error;

							snprintf(destinationfolder,path_str_size,"%s" DIR_SEPARATOR "%s",destfolder,FindFileData.filename);

							//printf("Creating directory %s\n",destinationfolder);
							hxc_mkdir(destinationfolder);

							path_str_size = strlen(FindFileData.filename) + strlen(folder) + 2;
							fullpath = (unsigned char*)malloc(path_str_size);
							if(!fullpath)
								goto error;

							snprintf((char*)fullpath,path_str_size,"%s%c%s",folder,DIR_SEPARATOR_CHAR,FindFileData.filename);

							CUI_affiche(MSG_INFO_1,(char*)"Entering directory %s",FindFileData.filename);

							tempstr = (char*)malloc(MAX_TMP_STR_SIZE);
							if(tempstr)
							{
								snprintf(tempstr,MAX_TMP_STR_SIZE,"Entering directory %s",FindFileData.filename);

								batchconverter_set_status_string(params, tempstr);
							}

							if(browse_and_convert_directory(floppycontext,(char*)fullpath,destinationfolder,file,output_file_format,params))
							{
								free(destinationfolder);
								free(fullpath);
								if(tempstr)
									free(tempstr);
								hxc_find_close(hfindfile);

								batchconverter_set_progress_status(guicontext,(char*)"Done", fl_rgb_color(80,80,255), 100);

								return 1;
							}

							free(destinationfolder);
							free(fullpath);

							CUI_affiche(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);

							if(tempstr)
							{
								snprintf(tempstr,MAX_TMP_STR_SIZE,"Leaving directory %s",FindFileData.filename);

								batchconverter_set_status_string(params, tempstr);
								free(tempstr);
							}
						}
					}
					else
					{
						batchconverter_set_progress_status(guicontext,(char*)"Reading", fl_rgb_color(50,255,50), -1);

						CUI_affiche(MSG_INFO_1,"converting file %s, %dB",FindFileData.filename,FindFileData.size);
						if(FindFileData.size)
						{
							path_str_size = strlen(FindFileData.filename) + strlen(folder) + 2;

							fullpath = (unsigned char*)malloc(path_str_size);
							if(!fullpath)
								goto error;

							snprintf((char*)fullpath,path_str_size,"%s" DIR_SEPARATOR "%s",folder,FindFileData.filename);

							tempstr = (char*)malloc(MAX_TMP_STR_SIZE);
							if(tempstr)
							{
								snprintf(tempstr,MAX_TMP_STR_SIZE,"%s",FindFileData.filename);

								batchconverter_set_status_string(params, tempstr);

								free(tempstr);
							}

							if(!params->rawfilemode)
								loaderid = hxcfe_imgAutoSetectLoader(imgldr_ctx,(char*)fullpath,0);

							if(loaderid>=0 || params->rawfilemode)
							{
								if(!params->rawfilemode)
								{
									thefloppydisk = hxcfe_imgLoad(imgldr_ctx,(char*)fullpath,loaderid,&ret);
								}
								else
								{
									mw = (Main_Window*)guicontext->main_window;

									Fl::lock();
									disklayout = mw->rawloader_window->choice_disklayout->value();
									Fl::unlock();

									if(disklayout>=1)
									{
										rfb = hxcfe_initXmlFloppy(guicontext->hxcfe);
										if(rfb)
										{
											if(disklayout ==  1)
												hxcfe_setXmlFloppyLayoutFile(rfb,guicontext->xml_file_path);
											else
												hxcfe_selectXmlFloppyLayout(rfb,disklayout-2);

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
								batchconverter_set_progress_status(guicontext,(char*)"Error", fl_rgb_color(255,10,10), -1);

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
								batchconverter_set_progress_status(guicontext,(char*)"Writing", fl_rgb_color(220,255,10), -1);

								path_str_size = strlen(FindFileData.filename) + strlen(destfolder) + strlen(ff_type_list[output_file_format].ext) + 1 + 2;
								destinationfile = (char*)malloc(path_str_size);
								if(!destinationfile)
									goto error;

								snprintf(destinationfile,path_str_size,"%s%c%s",destfolder,DIR_SEPARATOR_CHAR,FindFileData.filename);

								i = strlen(destinationfile);
								do
								{
									i--;
								}while(i && destinationfile[i]!='.');

								if(i)
								{
									if(keepsrcext)
										destinationfile[i] = '_';
									else
										destinationfile[i] = 0;
								}

								//printf("Creating file %s\n",destinationfile);
								strcat(destinationfile,ff_type_list[output_file_format].ext);

								tempstr = (char*)malloc(MAX_TMP_STR_SIZE);
								if(tempstr)
								{
									snprintf(tempstr,MAX_TMP_STR_SIZE,"%s",hxc_getfilenamebase(destinationfile,0,SYS_PATH_TYPE));

									batchconverter_set_status_string(params, tempstr);

									free(tempstr);
									tempstr = NULL;
								}

								loaderid = hxcfe_imgGetLoaderID(imgldr_ctx,(char*)ff_type_list[output_file_format].plug_id);
								if(loaderid>=0)
								{
									if(!guicontext->autoselectmode)
									{
										hxcfe_floppySetInterfaceMode(guicontext->hxcfe,thefloppydisk,guicontext->interfacemode);
									}

									hxcfe_floppySetDoubleStep(guicontext->hxcfe,thefloppydisk,guicontext->doublestep);

									ret = hxcfe_imgExport(imgldr_ctx,thefloppydisk,destinationfile,loaderid);
								}
								else
									ret = loaderid;


								hxcfe_imgUnload(imgldr_ctx,thefloppydisk);

								tempstr = (char*)malloc(MAX_TMP_STR_SIZE);

								i=strlen(destinationfile);
								do
								{
									i--;
								}while(i && destinationfile[i]!=DIR_SEPARATOR_CHAR);

								if(!ret)
								{
									if(tempstr)
									{
										snprintf(tempstr,MAX_TMP_STR_SIZE,"%s created",&destinationfile[i]);

										batchconverter_set_status_string(params, tempstr);
									}
									params->numberoffileconverted++;
								}
								else
								{
									if(tempstr)
									{
										snprintf(tempstr,MAX_TMP_STR_SIZE,"Error cannot create %s",&destinationfile[i]);
										batchconverter_set_status_string(params, tempstr);
									}
								}

								free(destinationfile);
								destinationfile = NULL;
								free(tempstr);
								tempstr = NULL;
							}
						}
					}

					bbool = hxc_find_next_file(hfindfile,folder,file,&FindFileData);
				}
			}

		}

		hxc_find_close(hfindfile);

		hxcfe_imgDeInitLoader( imgldr_ctx );

		batchconverter_set_progress_status(guicontext,(char*)"Done", fl_rgb_color(80,80,255), 100);
	}

	return 0;

error:
	return 0;
}

int convertthread(void* floppycontext,void* hw_context)
{
	batch_converter_window *bcw;
	char * tempstr;

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

		browse_and_convert_directory(   guicontext->hxcfe,
										bcparams.sourcedir,
										bcparams.destdir,
										(char*)"*.*",
										bcw->choice_file_format->value(),
										&bcparams);

		tempstr=(char*)malloc(MAX_TMP_STR_SIZE);
		if(tempstr)
		{
			snprintf(tempstr,MAX_TMP_STR_SIZE,"%d files converted!",(int)bcparams.numberoffileconverted);
			batchconverter_set_status_string(&bcparams, tempstr);
			free(tempstr);
		}
	}

	Fl::lock();
	bcw->bt_convert->activate();
	Fl::unlock();
	return 0;
}

int draganddropconvertthread(void* floppycontext,void* hw_context)
{
	batch_converter_window *bcw;
	batchconverterparams bcparams;
	char * tempstr;
	s_param_bc_params * bcparams2;
	int filecount,i,j,k;
	char ** filelist;

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

	filelist = (char **) calloc( 1, sizeof(char *) * (filecount + 1) );

	if( !filelist )
	{
		return -1;
	}

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

		draganddropconvert( guicontext->hxcfe,
							filelist,
							bcparams.destdir,
							bcw->choice_file_format->value(),
							&bcparams);

		tempstr=(char*)malloc(MAX_TMP_STR_SIZE);
		if(tempstr)
		{
			snprintf(tempstr,MAX_TMP_STR_SIZE,"%d files converted!",(int)bcparams.numberoffileconverted);
			batchconverter_set_status_string(&bcparams, tempstr);

			free(tempstr);
		}

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
	abort_trigger = 0;

	hxcfe_setEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_SRC_DIR", (char*)bcw->strin_src_dir->value() );
	hxcfe_setEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_DST_DIR", (char*)bcw->strin_dst_dir->value() );
	hxcfe_setEnvVarValue(guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_TARGETFORMAT", bcw->choice_file_format->value() );
	save_ui_state(guicontext->hxcfe);

	hxc_createthread(guicontext->hxcfe,bcw,&convertthread,0);
}

void batch_converter_window_bt_select_src(Fl_Button* bt, void*)
{
	char dirstr[MAX_TMP_STR_SIZE];
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(!select_dir((char*)"Select source",(char*)&dirstr))
	{
		bcw->strin_src_dir->value(dirstr);
		hxcfe_setEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_SRC_DIR", dirstr );
		save_ui_state(guicontext->hxcfe);
	}
}

void batch_converter_window_progress_indicator(Fl_Progress * flp,void *)
{

}

void batch_converter_window_bt_select_dst(Fl_Button* bt, void*)
{
	char dirstr[MAX_TMP_STR_SIZE];
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	if(!select_dir((char*)"Select destination",(char*)&dirstr))
	{
		bcw->strin_dst_dir->value(dirstr);
		hxcfe_setEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_DST_DIR", dirstr );
		save_ui_state(guicontext->hxcfe);
	}
}

void batch_converter_window_bt_cancel(Fl_Button* bt, void*)
{
	batch_converter_window *bcw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	bcw=(batch_converter_window *)dw->user_data();

	abort_trigger = 0xFF;

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
					abort_trigger = 0;
					hxc_createthread(guicontext->hxcfe,bcparams,&draganddropconvertthread,0);
				}
				else
				{
					free(bcparams);
				}
			}
		}
	}
}
