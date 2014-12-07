/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : cb_filesystem_generator_window.cxx
// Contains: Filesystem generator window callbacks
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>

#ifndef WIN32
#include <stdint.h>
#endif

#include "fl_includes.h"
#include "fl_dnd_box.h"
#include <FL/Fl_Select_Browser.H>

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "libhxcadaptor.h"
}

#include "main.h"

#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"
#include "sdhxcfe_cfg.h"
#include "loader.h"

#include "fileselector.h"

#include "utils.h"

void load_file_image(Fl_Widget * w, void * fc_ptr) ;
void save_file_image(Fl_Widget * w, void * fc_ptr) ;

extern s_gui_context * guicontext;

#ifdef WIN32
 #define intptr_t int
 #define SEPARATOR '\\'
 #define PATHSEPARATOR "\\"
#else
 #define SEPARATOR '/'
 #define PATHSEPARATOR "/"
#endif

typedef struct s_param_fs_params_
{
	char * files;
	filesystem_generator_window * fsw;
}s_param_fs_params;


static intptr_t s=15;

void fs_choice_cb(Fl_Widget *, void *v)
{
	s=(intptr_t)v;
}

void filesystem_generator_window_browser_fs(class Fl_Tree *,void *)
{

}

void printsize(char * buffer,int size)
{

	if(size < 1024)
	{
		sprintf(buffer,"%d B",size);
	}
	else
	{
		if(size < 1024*1024)
		{
			sprintf(buffer,"%d.%.2d KB",(size/1024),(((size-((size/1024)*1024))*100)/1024));
		}
		else
		{
			sprintf(buffer,"%d.%.2d MB",(size/(1024*1024)),(((size-((size/(1024*1024))*1024*1024))*100)/(1024*1024)));
		}
	}
}

char * getfilenamebase(char * fullpath,char * filenamebase)
{
	int len,i;

	len=strlen(fullpath);

	i=0;
	if(len)
	{
		i=len-1;
		while(i &&	(fullpath[i]!='\\' && fullpath[i]!='/' && fullpath[i]!=':') )
		{
			i--;
		}

		if( fullpath[i]=='\\' || fullpath[i]=='/' || fullpath[i]==':' )
		{
			i++;
		}

		if(i>len)
		{
			i=len;
		}
	}

	if(filenamebase)
	{
		strcpy(filenamebase,&fullpath[i]);
	}

	return &fullpath[i];
}

char * getfilenameext(char * fullpath,char * filenameext)
{
	char * filename;
	int len,i;

	filename=getfilenamebase(fullpath,0);

	len=strlen(filename);

	i=0;
	if(len)
	{
		i=len-1;

		while(i &&	( filename[i] != '.' ) )
		{
			i--;
		}

		if( filename[i] == '.' )
		{
			i++;
		}
		else
		{
			i=len;
		}

		if(i>len)
		{
			i=len;
		}
	}

	if(filenameext)
	{
		strcpy(filenameext,&filename[i]);
	}

	return &filename[i];
}

int getfilenamewext(char * fullpath,char * filenamewext)
{
	char * filename;
	char * ext;
	int len;

	filename=getfilenamebase(fullpath,0);
	ext=getfilenameext(fullpath,0);

	len=ext-filename;


	if(len && filename[len-1]=='.')
	{
		len--;
	}

	if(filenamewext)
	{
		memcpy(filenamewext,filename,len);
		filenamewext[len]=0;
	}

	return len;
}

int getpathfolder(char * fullpath,char * folder)
{
	int len;
	char * filenameptr;

	filenameptr=getfilenamebase(fullpath,0);

	len=filenameptr-fullpath;

	if(folder)
	{
		memcpy(folder,fullpath,len);
		folder[len]=0;
	}

	return len;
}

int checkfileext(char * path,char *ext)
{
	char pathext[16];
	char srcext[16];

	if(path && ext)
	{

		if( ( strlen(getfilenameext(path,0)) < 16 )  && ( strlen(ext) < 16 ))
		{
			getfilenameext(path,(char*)&pathext);
			hxc_strlower(pathext);

			strncpy((char*)srcext,ext,sizeof(srcext));
			hxc_strlower(srcext);

			if(!strncmp(pathext,srcext,sizeof(pathext)))
			{
				return 1;
			}
		}
	}
	return 0;
}

char * sec_strncat(char * dst,const char * src,unsigned int size)
{
	if(dst && src)
	{
		if(strlen(dst) + strlen(src) < size )
		{
			strncat(dst,src, size - strlen(dst) );
		}
	}

	return dst;
}

void filesystem_generator_window_bt_injectdir(Fl_Button* bt, void*)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;
	HXCFE_FLOPPY *floppy;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!fgw->FATAccessInProgress)
	{
		fgw->txtout_freesize->value(fgw->txtout_freesize->value());
		fgw->txtout_freesize->color(FL_WHITE);
		fgw->txtout_freesize->redraw();

		floppy=hxcfe_generateFloppy(guicontext->hxcfe,(char*)"",s,0);
		load_floppy(floppy,(char*)"Disk_Image");

		#ifdef STANDALONEFSBROWSER
		if(guicontext->loadedfloppy)
		{
			save_file_image(0,0);
		}
		#endif

		guicontext->updatefloppyfs = 1;
		guicontext->updatefloppyinfos++;
	}
}

void tick_fs(void *w) {

	HXCFE_TD * td;
	filesystem_generator_window *window;
	window=(filesystem_generator_window *)w;

	if(window->window->shown())
	{
		window->window->make_current();
		td=guicontext->td;
		if(td)
		{
			if(guicontext->updatefloppyfs)
			{
				if(guicontext->updatefloppyfs<0)
					browse_floppy_disk(window,-1);
				else
				{
					window->txtout_freesize->color(FL_WHITE);
					window->txtout_freesize->redraw();
					browse_floppy_disk(window, 1);
				}

				guicontext->updatefloppyfs=0;
			}

		}
	}

	Fl::repeat_timeout(0.1, tick_fs, w);
}


int displaydir(HXCFE_FSMNG  * fsmng,filesystem_generator_window *fgw,char * folder,int level)
{
	char fullpath[512];
	char fullpath2[512];
	int dirhandle;
	int ret;
	int dir;
	int totalsize;
	HXCFE_FSENTRY  dirent;

	totalsize = 0;

	memset(fullpath,0,sizeof(fullpath));
	memset(fullpath2,0,sizeof(fullpath2));

	dirhandle = hxcfe_openDir(fsmng,folder);
	if ( dirhandle > 0 )
	{
		do
		{
			dir = 0;
			ret = hxcfe_readDir(fsmng,dirhandle,&dirent);
			if(ret> 0)
			{
				if(dirent.isdir)
				{
					dir = 1;
				}
				else
				{
					totalsize = totalsize + dirent.size;
				}

				strncpy(fullpath,folder,sizeof(fullpath));
				sec_strncat(fullpath,dirent.entryname,sizeof(fullpath));

				if(dir)
				{
					if(fullpath[strlen(fullpath)-1] != '/')
					{
						sec_strncat(fullpath,"/",sizeof(fullpath));
					}
				}

				if( strcmp(dirent.entryname,"..") && strcmp(dirent.entryname,"."))
				{

					strcpy(fullpath2,fullpath);
					if(strlen(fullpath) && !dir)
					{
						strcat(fullpath2,"            |");
						printsize(&fullpath2[strlen(fullpath2)],dirent.size);
						strcat(fullpath2,"|");
					}

					fgw->fs_browser->add(fullpath2);
				}

				if(dir)
				{
					if( strcmp(dirent.entryname,"..") && strcmp(dirent.entryname,"."))
					{
						if(level < 32)
						{
							ret = displaydir(fsmng,fgw,fullpath,level+1);
							if(ret >= 0)
							{
								totalsize = totalsize + ret;
							}
							else
							{
								return ret;
							}

						}
						else
						{
							hxcfe_closeDir(fsmng,dirhandle);
							return totalsize;
						}
					}
				}
			}
			else
			{
				hxcfe_closeDir(fsmng,dirhandle);
				return totalsize;
			}

		}while(1);
	}
	return 0;
}

int getdir(filesystem_generator_window *fsw,HXCFE_FSMNG  * fsmng,char * folder,char * dstfolder,int level)
{
	char fullpath[1024];
	char fullpathdst[1024];
	char progresstxt[1024];
	int dirhandle;
	int filesize;
	int ret;
	int dir,i;
	HXCFE_FSENTRY  dirent;
	FILE * f;
	int floppyfile;

	unsigned char * membuf;

	dirhandle = hxcfe_openDir(fsmng,folder);
	if ( dirhandle > 0 )
	{
		if(!strlen(folder) || folder[strlen(folder)-1] != '/')
		{
			strcat(folder,"/");
		}

		do
		{
			dir = 0;
			ret = hxcfe_readDir(fsmng,dirhandle,&dirent);
			if(ret> 0)
			{
				if(dirent.isdir)
				{
					dir = 1;
				}

				strncpy(fullpathdst,dstfolder,sizeof(fullpathdst));

				if(fullpathdst[strlen(fullpathdst)-1] != SEPARATOR)
				{
					sec_strncat(fullpathdst,PATHSEPARATOR,sizeof(fullpathdst));
				}

				sec_strncat(fullpathdst,dirent.entryname,sizeof(fullpathdst));

				strncpy(fullpath,folder,sizeof(fullpath));
				sec_strncat(fullpath,dirent.entryname,sizeof(fullpath));

				if(dir)
				{
					if(fullpath[strlen(fullpath)-1] != '/')
					{
						sec_strncat(fullpath,"/",sizeof(fullpath));
					}

					if( strcmp(dirent.entryname,"..") && strcmp(dirent.entryname,"."))
					{
						hxc_mkdir(fullpathdst);
						if(getdir(fsw,fsmng,fullpath,fullpathdst,level+1)<0)
						{
							hxcfe_closeDir(fsmng,dirhandle);
							return 0;
						}
					}

				}
				else
				{
					membuf = 0;
					if(dirent.size)
					{
						membuf =(unsigned char*) malloc(dirent.size);
						memset(membuf,0,dirent.size);
					}

					sprintf(progresstxt,"Reading %s ...",fullpath);
					fsw->txtout_freesize->value(progresstxt);
					fsw->txtout_freesize->redraw();

					f = hxc_fopen(fullpathdst,"w+b");
					if(f)
					{
						if(membuf)
						{
							floppyfile = hxcfe_openFile(fsmng, fullpath);
							if(floppyfile>=0)
							{
								if(hxcfe_readFile( fsmng,floppyfile,membuf,dirent.size) != dirent.size )
								{
									sprintf(progresstxt,"Read file %s failed !!!",fullpath);
									fsw->txtout_freesize->value(progresstxt);
									fsw->txtout_freesize->color(FL_RED);
									fsw->txtout_freesize->redraw();
								}

								hxcfe_closeFile(fsmng, floppyfile);
							}
							else
							{
								sprintf(progresstxt,"Cannot open file %s !!!",fullpath);
								fsw->txtout_freesize->value(progresstxt);
								fsw->txtout_freesize->color(FL_RED);
								fsw->txtout_freesize->redraw();
							}

							if(!fwrite(membuf,dirent.size,1,f))
							{
								sprintf(progresstxt,"Write file %s failed !!!",fullpathdst);
								fsw->txtout_freesize->value(progresstxt);
								fsw->txtout_freesize->color(FL_RED);
								fsw->txtout_freesize->redraw();
							}

						}
						hxc_fclose(f);
					}
					else
					{
						sprintf(progresstxt,"Cannot create local file %s !!!",fullpathdst);
						fsw->txtout_freesize->value(progresstxt);
						fsw->txtout_freesize->color(FL_RED);
						fsw->txtout_freesize->redraw();
					}

					if(membuf)
						free(membuf);
				}
			}
			else
			{
				return 0;
			}

		}while(1);
	}
	else
	{
		sprintf(progresstxt,"Reading %s ...",folder);
		fsw->txtout_freesize->value(progresstxt);
		fsw->txtout_freesize->redraw();

		floppyfile = hxcfe_openFile(fsmng, folder);
		if(floppyfile>=0)
		{
			hxcfe_fseek(fsmng,floppyfile,0,SEEK_END);
			filesize = hxcfe_ftell(fsmng,floppyfile);
			hxcfe_fseek(fsmng,floppyfile,0,SEEK_SET);

			membuf = 0;
			if(filesize)
			{
				membuf =(unsigned char*) malloc(filesize);
				memset(membuf,0,filesize);
			}

			if(hxcfe_readFile( fsmng,floppyfile,membuf,filesize) != filesize )
			{
				sprintf(progresstxt,"Read file %s failed !!!",folder);
				fsw->txtout_freesize->value(progresstxt);
				fsw->txtout_freesize->color(FL_RED);
				fsw->txtout_freesize->redraw();
			}

			hxcfe_closeFile(fsmng, floppyfile);

			strncpy(fullpathdst,dstfolder,sizeof(fullpathdst));

			if(fullpathdst[strlen(fullpathdst)-1] != SEPARATOR)
			{
				sec_strncat(fullpathdst,PATHSEPARATOR,sizeof(fullpathdst));
			}

			i = strlen(folder);
			while(i && folder[i-1]!='/')
			{
				i--;
			}

			sec_strncat(fullpathdst, &folder[i],sizeof(fullpathdst));

			f = hxc_fopen(fullpathdst,"w+b");
			if(f)
			{
				if(membuf)
				{
					if(!fwrite(membuf,filesize,1,f))
					{
						sprintf(progresstxt,"Write file %s failed !!!",fullpathdst);
						fsw->txtout_freesize->value(progresstxt);
						fsw->txtout_freesize->color(FL_RED);
						fsw->txtout_freesize->redraw();
					}
				}
				hxc_fclose(f);
			}
			else
			{
				sprintf(progresstxt,"Cannot create local file %s !!!",fullpathdst);
				fsw->txtout_freesize->value(progresstxt);
				fsw->txtout_freesize->color(FL_RED);
				fsw->txtout_freesize->redraw();
			}

			if(membuf)
				free(membuf);
		}
	}

	return 0;
}

void browse_floppy_disk(filesystem_generator_window *fgw,int lastoperationerror)
{
	HXCFE_FSMNG  * fsmng;
	int totalsize;
	char statustxt[512];

	fgw->fs_browser->clear();
	fgw->fs_browser->selectmode(FL_TREE_SELECT_MULTI);
	if(guicontext->loadedfloppy)
	{

		fsmng = hxcfe_initFsManager(guicontext->hxcfe);
		if (fsmng)
		{
			if(hxcfe_mountImage(fsmng, guicontext->loadedfloppy) == HXCFE_NOERROR)
			{
				if(hxcfe_getTotalFsSpace(fsmng))
				{
					totalsize = displaydir(fsmng,fgw,(char*)"/",0);

					fgw->fs_browser->root_label("/");
					fgw->fs_browser->showroot(1);
					fgw->fs_browser->redraw();
					//fgw->fs_browser->show_self();


					if(totalsize>=0)
					{
						if(lastoperationerror>=0)
						{
							sprintf(statustxt,"| Disk size : ");
							printsize(&statustxt[strlen(statustxt)],hxcfe_getTotalFsSpace(fsmng));
							strcat(statustxt," | File size : ");
							printsize(&statustxt[strlen(statustxt)],totalsize);
							strcat(statustxt," | Left space: ");
							printsize(&statustxt[strlen(statustxt)],hxcfe_getFreeFsSpace(fsmng));
							strcat(statustxt," |");
				#ifdef STANDALONEFSBROWSER
							sec_strncat(statustxt," File : ",sizeof(statustxt));
							sec_strncat(statustxt,guicontext->last_loaded_image_path,sizeof(statustxt));
				#endif
							fgw->txtout_freesize->value((const char*)statustxt);
						}
						else
						{
							fgw->txtout_freesize->value(fgw->txtout_freesize->value());
							fgw->txtout_freesize->color(FL_RED);
							fgw->txtout_freesize->redraw();
						}
					}
					else
					{
						fgw->txtout_freesize->value("Some Errors found during the Image reading ! Disk damaged ?");
						fgw->txtout_freesize->color(FL_RED);
						fgw->txtout_freesize->redraw();

						fgw->fs_browser->root_label("/");
						fgw->fs_browser->showroot(0);
						fgw->fs_browser->redraw();

					}
				}
				else
				{
					fgw->txtout_freesize->value("Null sized DOS image ! Invalid disk !");
					fgw->txtout_freesize->color(FL_RED);
					fgw->txtout_freesize->redraw();

					fgw->fs_browser->root_label("/");
					fgw->fs_browser->showroot(0);
					fgw->fs_browser->redraw();
				}

				hxcfe_umountImage(fsmng);
			}
			else
			{
				fgw->txtout_freesize->value("Some Errors found during the Image mounting ! DOS Disk ? Disk damaged ?");
				fgw->txtout_freesize->color(FL_RED);
				fgw->txtout_freesize->redraw();

				fgw->fs_browser->root_label("/");
				fgw->fs_browser->showroot(0);
				fgw->fs_browser->redraw();

			}
			hxcfe_deinitFsManager(fsmng);
		}
	}
	else
	{
		fgw->fs_browser->root_label("/");
		fgw->fs_browser->showroot(1);
		fgw->fs_browser->redraw();
		//fgw->fs_browser->show_self();

		sprintf(statustxt,"No DOS Image Loaded !");
		fgw->txtout_freesize->value((const char*)statustxt);
	}
}

void filesystem_generator_window_bt_createdir(Fl_Button * bt,void *)
{

}

void filesystem_generator_window_bt_putfiles(Fl_Button * bt,void *)
{

}

void filesystem_generator_window_bt_delete(Fl_Button *bt,void *)
{
	char itempath[512];
	filesystem_generator_window *fgw;
	Fl_Window *dw;
	HXCFE_FSMNG  * fsmng;
	Fl_Tree_Item * flt_item;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!fgw->FATAccessInProgress)
	{
		fgw->txtout_freesize->color(FL_WHITE);
		fgw->txtout_freesize->redraw();

		guicontext->loaded_img_modified = 1;

		fsmng = hxcfe_initFsManager(guicontext->hxcfe);
		if (fsmng)
		{
			hxcfe_mountImage(fsmng, guicontext->loadedfloppy);

			flt_item = fgw->fs_browser->first_selected_item();
			while(flt_item)
			{
				fgw->fs_browser->item_pathname((char*)&itempath, sizeof(itempath), flt_item);

				if(strstr(itempath,"            |"))
					*strstr(itempath,"            |") = 0;

				hxcfe_deleteFile(fsmng, itempath+2);
				flt_item = fgw->fs_browser->next_selected_item(flt_item);
			}

			hxcfe_umountImage(fsmng);
			hxcfe_deinitFsManager(fsmng);
		}

		guicontext->updatefloppyfs = 1;
		guicontext->updatefloppyinfos++;
	}
}

void filesystem_generator_window_bt_loadimage(Fl_Button *bt,void *)
{
	Fl_Window *dw;
	filesystem_generator_window *fgw;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!fgw->FATAccessInProgress)
	{
		load_file_image(0,0);
	}
}

void filesystem_generator_window_bt_saveexport(Fl_Button *bt,void *)
{
	if(!guicontext->loadedfloppy)
	{
		fl_alert("No floppy image loaded !\nPlease load an image or create a disk image\n");
	}
	else
	{
		save_file_image(0,0);
	}
}

void filesystem_generator_window_bt_close(Fl_Button *bt,void *)
{
	write_back_fileimage();

	((Fl_Window*)(bt->parent()))->hide();
}

int write_back_fileimage()
{
	if(guicontext->loaded_img_modified)
	{
		if(strlen(guicontext->last_loaded_image_path))
		{
#ifdef STANDALONEFSBROWSER
			hxcfe_floppyExport(guicontext->hxcfe,guicontext->loadedfloppy,guicontext->last_loaded_image_path,hxcfe_getLoaderID(guicontext->hxcfe,"HXC_HFE"));
#endif
		}
		guicontext->loaded_img_modified = 0;
	}

	return 0;
}

int load_indexed_fileimage(int index)
{
#ifdef STANDALONEFSBROWSER
	FILE * f;
	char filename[1024];
	int cur_index;

	char cur_directory[1024*4];
	char fullpath[1024*4];

	long fff_handle;
	int ret,i;
	filefoundinfo fi;
	unsigned char filebuffer[8*1024];
	sdhxcfecfgfile * filecfg;

	cur_index = index;

	hxc_getcurrentdirectory(cur_directory,sizeof(cur_directory));
	sprintf(fullpath,"%s%c%s",cur_directory,SEPARATOR,"HXCSDFE.CFG");

	memset(filebuffer,0,8*1024);
	filecfg = (sdhxcfecfgfile *)&filebuffer;
	f=hxc_fopen((char*)fullpath,"r+b");
	if(f)
	{
		fread(filebuffer,8*1024,1,f);
		hxc_fclose(f);
	}

	if(filecfg->indexed_mode)
	{

		sprintf(filename,"DSKA%.4d.HFE",cur_index);
		sprintf(fullpath,"%s%c%s",cur_directory,SEPARATOR,filename);

		write_back_fileimage();

		f = hxc_fopen (fullpath,"rb");
		if(f)
		{
			strcpy(guicontext->last_loaded_image_path,fullpath);
			hxc_fclose(f);
		}
		else
		{
			cur_index = 0;
			sprintf(filename,"DSKA%.4d.HFE",cur_index);
			sprintf(fullpath,"%s%c%s",cur_directory,SEPARATOR,filename);
			f = hxc_fopen (fullpath,"rb");
			if(f)
			{
				strcpy(guicontext->last_loaded_image_path,fullpath);
				fclose(f);
			}
			else
			{
				hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
				guicontext->loadedfloppy=0;

				guicontext->last_loaded_image_path[0]=0;
				cur_index = -1;
			}
		}

		if(cur_index>=0)
		{
			load_floppy_image(fullpath);
		}

		guicontext->updatefloppyinfos++;
		guicontext->updatefloppyfs = 1;

		return cur_index;
	}
	else
	{
		write_back_fileimage();

		hxc_getcurrentdirectory(cur_directory,sizeof(cur_directory));

		fff_handle = hxc_find_first_file(cur_directory,"*.hfe",&fi);
		if(fff_handle!=-1)
		{
			ret = 1;
			i = 0;
			while( i < cur_index && ret)
			{
				ret = hxc_find_next_file(fff_handle,cur_directory,"*.hfe",&fi);
				i++;
			}

			if(!ret)
			{
				hxc_find_close(fff_handle);
				cur_index = 0;

				fff_handle = hxc_find_first_file(cur_directory,"*.hfe",&fi);
				if(fff_handle != -1)
				{
					strncpy(filename,fi.filename,sizeof(filename));
				}
				else
				{
					guicontext->last_loaded_image_path[0]=0;
					cur_index = -1;
				}
			}
			else
			{
				strncpy(filename,fi.filename,sizeof(filename));
			}

			hxc_find_close(fff_handle);
		}
		else
		{
			hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
			guicontext->loadedfloppy=0;

			guicontext->last_loaded_image_path[0]=0;
			cur_index = -1;
		}

		if(cur_index>=0)
		{
			sprintf(fullpath,"%s%c%s",cur_directory,SEPARATOR,filename);
			if(load_floppy_image(fullpath) == HXCFE_NOERROR)
			{
				strcpy(guicontext->last_loaded_image_path,fullpath);
			}
		}

		guicontext->updatefloppyinfos++;
		guicontext->updatefloppyfs = 1;

		return cur_index;
	}

#else
	return -1;
#endif
}

void filesystem_generator_window_sel_disk(class Fl_Counter * cnt,void *)
{
	int index;

	Fl_Window *dw;
	filesystem_generator_window *fgw;

	dw=((Fl_Window*)(cnt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!fgw->FATAccessInProgress)
	{
		index = load_indexed_fileimage((int)cnt->value());
		if(index<0) cnt->value(0);
		else cnt->value(index);

		guicontext->updatefloppyinfos++;
		guicontext->updatefloppyfs = 1;
	}
}

void dnd_fs_conv(const char *urls)
{

}

int addentry(filesystem_generator_window *fsw,HXCFE_FSMNG  * fsmng,  char * srcpath,char *dstpath)
{
	FILE * f;
	int size,file_handle,ret,dirhandle,wsize;
	struct stat entry;
	unsigned char * buffer;
	char fullpath[1024];
	char srcfullpath[1024];
	char progresstxt[1024];
	int ff;
	filefoundinfo ffi;

	sprintf(progresstxt,"Adding %s ...",dstpath);
	fsw->txtout_freesize->value(progresstxt);

	hxc_stat(srcpath,&entry);

	guicontext->loaded_img_modified = 1;

	if(entry.st_mode&S_IFDIR)
	{
		strncpy(fullpath,dstpath,sizeof(fullpath));

		// Is the folder already there ?
		ret = 0;
		dirhandle = hxcfe_openDir(fsmng,fullpath);
		if ( dirhandle <= 0 )
		{
			// No ... we need to add it.
			ret = hxcfe_createDir(fsmng,fullpath);
		}
		else
		{
			hxcfe_closeDir(fsmng,dirhandle);
		}

		if(!ret)
		{
			ff = hxc_find_first_file(srcpath,(char*)"*.*",&ffi);
			if(ff != -1)
			{
				do
				{
					if(ffi.isdirectory)
					{
						if(strcmp(ffi.filename,".") && strcmp(ffi.filename,".."))
						{
							strncpy(srcfullpath,srcpath,sizeof(srcfullpath));
							sec_strncat(srcfullpath,PATHSEPARATOR,sizeof(srcfullpath));
							sec_strncat(srcfullpath,ffi.filename,sizeof(srcfullpath));

							strncpy(fullpath,dstpath,sizeof(fullpath));
							sec_strncat(fullpath,"/",sizeof(fullpath));
							sec_strncat(fullpath,ffi.filename,sizeof(fullpath));
							if(addentry(fsw, fsmng, srcfullpath, fullpath)<0)
							{
								hxc_find_close(ff);
								return -1;
							}
						}
					}
					else
					{
						strncpy(srcfullpath,srcpath,sizeof(srcfullpath));
						sec_strncat(srcfullpath,PATHSEPARATOR,sizeof(srcfullpath));
						sec_strncat(srcfullpath,ffi.filename,sizeof(srcfullpath));

						strncpy(fullpath,dstpath,sizeof(fullpath));
						sec_strncat(fullpath,"/",sizeof(fullpath));
						sec_strncat(fullpath,ffi.filename,sizeof(fullpath));
						if(addentry(fsw, fsmng, srcfullpath, fullpath)<0)
						{
							hxc_find_close(ff);
							return -1;
						}
					}

					ret = hxc_find_next_file(ff,srcpath,(char*)"*.*",&ffi);

				}while(ret);

				ret = hxc_find_close(ff);
			}
		}
		else
		{
			sprintf(progresstxt,"Cannot create folder %s !!!",fullpath);
			fsw->txtout_freesize->value(progresstxt);
			fsw->txtout_freesize->color(FL_RED);

			return -1;
		}
	}
	else
	{
		f = hxc_fopen(srcpath,"rb");
		if (f)
		{
			fseek(f,0,SEEK_END);
			size = ftell(f);

			sprintf(progresstxt,"Adding %s (",dstpath);
			printsize(&progresstxt[strlen(progresstxt)],size);
			strcat(progresstxt,") ...");
			fsw->txtout_freesize->value(progresstxt);

			fseek(f,0,SEEK_SET);

			if(size < 32 * 1024*1024 )
			{
				buffer =(unsigned char*) malloc(size);
				if(buffer)
				{
					fread(buffer,size,1,f);
					strncpy(fullpath,dstpath,sizeof(fullpath));
					file_handle = hxcfe_createFile(fsmng,fullpath );
					if(file_handle>0)
					{
						wsize = hxcfe_writeFile(fsmng,file_handle,buffer,size);
						hxcfe_closeFile(fsmng,file_handle);
						if( wsize != size )
						{
							sprintf(progresstxt,"Error: %s Write failed !!! No more space ? ",fullpath);
							printsize(&progresstxt[strlen(progresstxt)],size);
							strcat(progresstxt," Needed ! ");

							printsize(&progresstxt[strlen(progresstxt)],hxcfe_getFreeFsSpace(fsmng));
							strcat(progresstxt," Free !");

							fsw->txtout_freesize->value(progresstxt);
							fsw->txtout_freesize->color(FL_RED);

							hxcfe_deleteFile(fsmng, fullpath);

							free(buffer);
							hxc_fclose(f);

							return -1;
						}
						else
						{
							fsw->txtout_freesize->color(FL_WHITE);
							fsw->txtout_freesize->redraw();
						}
					}
					else
					{
						sprintf(progresstxt,"Cannot create %s !!!",dstpath);
						fsw->txtout_freesize->value(progresstxt);
						fsw->txtout_freesize->color(FL_RED);

						free(buffer);
						hxc_fclose(f);

						return -1;
					}

					free(buffer);
				}
				else
				{
					sprintf(progresstxt,"No enough system memory !!!");
					fsw->txtout_freesize->value(progresstxt);
					fsw->txtout_freesize->color(FL_RED);

					hxc_fclose(f);
					return -1;

				}

			}
			else
			{
				sprintf(progresstxt,"File too big !!!");
				fsw->txtout_freesize->value(progresstxt);
				fsw->txtout_freesize->color(FL_RED);

				hxc_fclose(f);
				return -1;

			}

			hxc_fclose(f);
		}
		else
		{
			sprintf(progresstxt,"Cannot open source file %s !!!",srcpath);
			fsw->txtout_freesize->value(progresstxt);
			fsw->txtout_freesize->color(FL_RED);

			return -1;
		}
	}

	return 0;
}

int getfilesthread(void* floppycontext,void* hw_context)
{
	HXCFE_FSMNG  * fsmng;
	HXCFE* floppyem;
	filesystem_generator_window *fsw;
	s_param_fs_params * fsparams2;
	char itempath[512];
	Fl_Tree_Item * flt_item;

	floppyem=(HXCFE*)floppycontext;
	fsparams2=(s_param_fs_params *)hw_context;

	if(floppyem && fsparams2)
	{
		fsw=fsparams2->fsw;

		fsw->txtout_freesize->color(FL_WHITE);
		fsw->txtout_freesize->redraw();

		fsmng = hxcfe_initFsManager(guicontext->hxcfe);
		if (fsmng)
		{
			hxcfe_mountImage(fsmng, guicontext->loadedfloppy);

			flt_item = fsw->fs_browser->first_selected_item();
			if(flt_item)
			{
				while(flt_item)
				{
					fsw->fs_browser->item_pathname((char*)&itempath, sizeof(itempath), flt_item);

					if(strstr(itempath,"            |"))
						*strstr(itempath,"            |") = 0;

					getdir(fsw,fsmng,itempath+2,fsparams2->files,0);
					flt_item = fsw->fs_browser->next_selected_item(flt_item);
				}
			}
			else
			{
				getdir(fsw,fsmng,(char*)"/",fsparams2->files,0);
			}

			hxcfe_umountImage(fsmng);
			hxcfe_deinitFsManager(fsmng);
		}

		guicontext->updatefloppyfs = 1;
		guicontext->updatefloppyinfos++;

		free(fsparams2);

		fsw->FATAccessInProgress = 0;
		fsw->txtout_freesize->color(FL_WHITE);
		fsw->txtout_freesize->redraw();

	}

	return 0;
}

void filesystem_generator_window_bt_getfiles(Fl_Button *bt,void *)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;
	char dirstr[512];
	s_param_fs_params * fsparams;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!fgw->FATAccessInProgress)
	{
		if(guicontext->loadedfloppy && !fgw->FATAccessInProgress)
		{
			if(!select_dir((char*)"Select destination",(char*)&dirstr))
			{
				fgw->txtout_freesize->color(FL_WHITE);
				fgw->txtout_freesize->redraw();

				if(strlen(dirstr))
				{
					fsparams=(s_param_fs_params *)malloc(sizeof(s_param_fs_params));
					if(fsparams)
					{
						memset(fsparams,0,sizeof(s_param_fs_params));

						fsparams->fsw = fgw;

						fsparams->files = (char*) malloc(strlen(dirstr)+1);
						if(fsparams->files)
						{
							strcpy(fsparams->files,dirstr);
							fgw->FATAccessInProgress = 1;
							hxc_createthread(guicontext->hxcfe,fsparams,&getfilesthread,0);
						}
						else
						{
							free(fsparams);
						}
					}
				}
			}
		}
	}
}

int draganddropfsthread(void* floppycontext,void* hw_context)
{
	HXCFE_FSMNG  * fsmng;
	HXCFE* floppyem;
	filesystem_generator_window *fsw;
	s_param_fs_params * fsparams2;
	char fullpath[1024];
	char basepath[1024];
	char itempath[512];
	int filecount,i,j,k;
	int dirhandle,ret;
	char ** filelist;
	struct stat entry;
	Fl_Tree_Item * flt_item;

	floppyem=(HXCFE*)floppycontext;
	fsparams2=(s_param_fs_params *)hw_context;

	if(floppyem && fsparams2)
	{
		fsw=fsparams2->fsw;

		fsw->txtout_freesize->color(FL_WHITE);
		fsw->txtout_freesize->redraw();

		filecount=0;
		i=0;
		while(fsparams2->files[i])
		{
			if(fsparams2->files[i]==0xA)
			{
				filecount++;
			}
			i++;
		}

		filecount++;

		filelist =(char **) malloc(sizeof(char *) * (filecount + 1) );
		if( filelist )
		{
			memset(filelist,0,sizeof(char *) * (filecount + 1) );

			i=0;
			j=0;
			k=0;
			do
			{
				while(fsparams2->files[i]!=0 && fsparams2->files[i]!=0xA)
				{
					i++;
				};

				filelist[k] = URIfilepathparser((char*)&fsparams2->files[j],i-j);

				i++;
				j=i;

				k++;

			}while(k<filecount);

			if(filecount)
			{
				if(guicontext->loadedfloppy)
				{

					// Get the base path
					memset(basepath,0,sizeof(basepath));
					flt_item = fsw->fs_browser->first_selected_item();
					if(flt_item)
					{
						fsw->fs_browser->item_pathname((char*)&itempath, sizeof(itempath), flt_item);
						strncpy(basepath,itempath+2,sizeof(basepath));
					}

					fsw->fs_browser->clear();
					fsw->fs_browser->selectmode(FL_TREE_SELECT_MULTI);

					fsmng = hxcfe_initFsManager(guicontext->hxcfe);
					if (fsmng)
					{
						hxcfe_mountImage(fsmng, guicontext->loadedfloppy);

						dirhandle = hxcfe_openDir(fsmng,basepath);
						if ( dirhandle <= 0 )
						{
							memset(basepath,0,sizeof(basepath));

						}
						else
						{
							hxcfe_closeDir(fsmng,dirhandle);
						}


						ret = 0;
						i=0;
						while( ( i < filecount ) && filelist[i] && (ret >= 0))
						{
							hxc_stat(filelist[i],&entry);

							sprintf(fullpath,"%s/%s",basepath,getfilenamebase(filelist[i],0));

							if(filelist[i] && strlen(filelist[i]) )
							{
								ret = addentry(fsw,fsmng,  filelist[i],fullpath);
							}

							i++;
						}

						fsw->fs_browser->root_label("/");
						fsw->fs_browser->showroot(1);

						hxcfe_umountImage(fsmng);
						hxcfe_deinitFsManager(fsmng);
					}
				}

				if(ret>=0)
					guicontext->updatefloppyfs = 1;
				else
					guicontext->updatefloppyfs = -1;

				guicontext->updatefloppyinfos++;

				k=0;
				while(filelist[k])
				{
					free(filelist[k]);
					k++;
				}

#ifndef STANDALONEFSBROWSER
				libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
#endif
			}

			free(filelist);
		}

		free(fsparams2);

		fsw->FATAccessInProgress = 0;
		fsw->txtout_freesize->color(FL_WHITE);
		fsw->txtout_freesize->redraw();
	}

	return 0;
}

void dnd_fs_cb(Fl_Widget *o, void *v)
{
	filesystem_generator_window *fgw;

	Fl_Window *dw;
	Fl_DND_Box *dnd = (Fl_DND_Box*)o;

	s_param_fs_params * fsparams;

	dw=((Fl_Window*)(o->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(dnd->event() == FL_PASTE)
	{
		if(guicontext->loadedfloppy && !fgw->FATAccessInProgress)
		{
			if(strlen(dnd->event_text()))
			{
				fsparams=(s_param_fs_params *)malloc(sizeof(s_param_fs_params));
				if(fsparams)
				{
					memset(fsparams,0,sizeof(s_param_fs_params));

					fsparams->fsw = fgw;

					fsparams->files = (char*) malloc(strlen(dnd->event_text())+1);
					if(fsparams->files)
					{
						strcpy(fsparams->files,dnd->event_text());
						fgw->FATAccessInProgress = 1;
						hxc_createthread(guicontext->hxcfe,fsparams,&draganddropfsthread,0);
					}
					else
					{
						free(fsparams);
					}
				}
			}
		}
	}
}
