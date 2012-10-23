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
// File : cb_filesystem_generator_window.cxx
// Contains: Filesystem generator window callbacks
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>

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
#include "loader.h"

#include "fileselector.h"

extern s_gui_context * guicontext;

#ifdef WIN32
#define intptr_t int
#endif

typedef struct s_param_fs_params_
{
	const char * files;
	filesystem_generator_window * fsw;
}s_param_fs_params;


static intptr_t s=0;

void fs_choice_cb(Fl_Widget *, void *v)
{
	s=(intptr_t)v;
}

void filesystem_generator_window_browser_fs(class Fl_Tree *,void *)
{

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
			
			strcpy((char*)srcext,ext);
			hxc_strlower(srcext);

			if(!strcmp(pathext,srcext))
			{
				return 1;
			}
		}
	}
	return 0;
}

void filesystem_generator_window_bt_injectdir(Fl_Button* bt, void*)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;
	FLOPPY *floppy;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();
	
	floppy=hxcfe_generateFloppy(guicontext->hxcfe,(char*)fgw->input_folder->value(),s,0);
	load_floppy(floppy);
	guicontext->updatefloppyfs++;
}

void tick_fs(void *w) {
	
	s_trackdisplay * td;
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
				guicontext->updatefloppyfs=0;
				browse_floppy_disk(window);
	//			update_graph(window);
		//		void browse_floppy_disk(filesystem_generator_window *fgw);
			}

		}
	}
				
	Fl::repeat_timeout(0.1, tick_fs, w);  
}


int displaydir(FSMNG  * fsmng,filesystem_generator_window *fgw,char * folder,int level)
{
	char fullpath[1024];
	int dirhandle;
	int ret;
	int dir;
	FSENTRY  dirent;

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

				strcpy(fullpath,folder);
				strcat(fullpath,dirent.entryname);
				if( strcmp(dirent.entryname,"..") && strcmp(dirent.entryname,"."))
				{
					fgw->fs_browser->add((char*)&fullpath);
				}

				if(dir)
				{	
					if(fullpath[strlen(fullpath)-1] != '/')
					{
						strcat(fullpath,"/");
					}

					if( strcmp(dirent.entryname,"..") && strcmp(dirent.entryname,"."))
					{
						if(displaydir(fsmng,fgw,fullpath,level+1)<0)
						{
							hxcfe_closeDir(fsmng,dirhandle);
							return 0;
						}
					}

				}
			}
			else
			{
				return 0;
			}
			 

		}while(1);
	}
	return 0;
}

void browse_floppy_disk(filesystem_generator_window *fgw)
{
	FSMNG  * fsmng;

	fgw->fs_browser->clear();
	fgw->fs_browser->selectmode(FL_TREE_SELECT_MULTI);
	if(guicontext->loadedfloppy)
	{
		
		fsmng = hxcfe_initFsManager(guicontext->hxcfe);
		if (fsmng)
		{
			hxcfe_selectFS(fsmng, 0);
			hxcfe_mountImage(fsmng, guicontext->loadedfloppy);

			
			displaydir(fsmng,fgw,"/",0);

			fgw->fs_browser->root_label("DISK");
			fgw->fs_browser->showroot(1);
			fgw->fs_browser->redraw();
			fgw->fs_browser->show_self();
			
			hxcfe_deinitFsManager(fsmng);
		}
	}
}

void filesystem_generator_window_bt_selectdir(Fl_Button* bt, void*)
{
	char dirstr[512];
	filesystem_generator_window *fgw;
	Fl_Window *dw;


	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!select_dir((char*)"Select source",(char*)&dirstr))
	{
		fgw->input_folder->value(dirstr);
	}
}

void filesystem_generator_bt_cancel(Fl_Button* bt, void*)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();
	
	fgw->window->hide();
}

void dnd_fs_conv(const char *urls)
{
	//loadfloppy((char*)urls);
}

int draganddropfsthread(void* floppycontext,void* hw_context)
{

	FSMNG  * fsmng;
	FILE * f;
	int size,file_handle;
	HXCFLOPPYEMULATOR* floppyem;
	filesystem_generator_window *fsw;
	s_param_fs_params * fsparams2;
	unsigned char * buffer;
	char fullpath[1024];	
	int filecount,i,j,k;
	char ** filelist;

	floppyem=(HXCFLOPPYEMULATOR*)floppycontext;
	fsparams2=(s_param_fs_params *)hw_context;
	fsw=fsparams2->fsw;

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

		filelist[k] = (char*)malloc((i-j)+3);
		memset( filelist[k] , 0 , (i-j)+3 );
		memcpy( filelist[k] , &fsparams2->files[j] , (i-j));
		i++;
		j=i;

		k++;

	}while(k<filecount);


	if(filecount)
	{
		if(guicontext->loadedfloppy)
		{

			fsw->fs_browser->clear();
			fsw->fs_browser->selectmode(FL_TREE_SELECT_MULTI);

			fsmng = hxcfe_initFsManager(guicontext->hxcfe);
			if (fsmng)
			{
				hxcfe_selectFS(fsmng, 0);
				hxcfe_mountImage(fsmng, guicontext->loadedfloppy);

				i=0;
				while(i < filecount)
				{
					f = hxc_fopen(filelist[i],"r+b");
					if (f)
					{
						fseek(f,0,SEEK_END);
						size = ftell(f);
						fseek(f,0,SEEK_SET);

						if(size < 32 * 1024*1024 )
						{
							buffer =(unsigned char*) malloc(size);
							if(buffer)
							{
								fread(buffer,size,1,f);
								sprintf(fullpath,"/");
								strcat(fullpath,getfilenamebase(filelist[i],0));
								file_handle = hxcfe_createFile(fsmng,fullpath );
								if(file_handle>0)
								{
									hxcfe_writeFile(fsmng,file_handle,(char*)buffer,size);
									hxcfe_closeFile(fsmng,file_handle);
								}

								free(buffer);
							}
						}

						hxc_fclose(f);
					}

					i++;
				}

				
				fsw->fs_browser->root_label("DISK");
				fsw->fs_browser->showroot(1);

				hxcfe_deinitFsManager(fsmng);
			}
		}

		guicontext->updatefloppyfs++;
		
		k=0;
		while(filelist[k])
		{
			free(filelist[k]);
			k++;
		}
	}

	free(filelist);
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
		if(guicontext->loadedfloppy)
		{

			fsparams=(s_param_fs_params *)malloc(sizeof(s_param_fs_params *));
			memset(fsparams,0,sizeof(s_param_fs_params));

			fsparams->fsw=fgw;
			fsparams->files=dnd->event_text();

			hxc_createthread(guicontext->hxcfe,fsparams,&draganddropfsthread,1);
		}
	}
}


