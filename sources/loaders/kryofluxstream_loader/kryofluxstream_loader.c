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

#include <math.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "os_api.h"

int KryoFluxStream_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int found,track,side;
	struct stat staterep;
	char * filepath;
	FILE * f;
	s_oob_header oob;
	char filename[512];

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
			else
			{
				getfilenamebase(imgfile,(char*)&filename);
				strlower((char*)&filename);
				found=0;

				if(!strstr(filename,".0.raw") && !strstr(filename,".1.raw") )
				{
					return HXCFE_BADFILE;
				}

				f=fopen(imgfile,"rb");
				if(f)
				{
					fread(&oob,sizeof(s_oob_header),1,f);
					if( ( oob.Sign == OOB_SIGN ) && ( oob.Type>=1 && oob.Type<=4 ) )
					{
						found=1;
					}
					fclose(f);

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
	}
	
	return HXCFE_BADPARAMETER;
}

void settrackbit(unsigned char * dstbuffer,unsigned char byte,int bitoffset,int size)
{
	int i,j,k;

	k=0;
	i=bitoffset;
	for(j=0;j<size;j++)
	{
		if(byte&((0x80)>>(j&7)))
			dstbuffer[i>>3]=dstbuffer[i>>3]|( (0x80>>(i&7)));
		else
			dstbuffer[i>>3]=dstbuffer[i>>3]&(~(0x80>>(i&7)));

		i++;
	}
}

void computehistogram(unsigned long *indata,int size,unsigned long *outdata)
{
	int i;

	memset(outdata,0,sizeof(unsigned long) * (65536) );
	i=0;
	do
	{	
		if(indata[i]<0x10000)
		{
			outdata[indata[i]]++;
		}
		i++;
	}while(i<size);
}

typedef struct stathisto_
{
	unsigned long val;
	unsigned long occurence;
	float pourcent;
}stathisto;

int detectpeaks(HXCFLOPPYEMULATOR* floppycontext,unsigned long *histogram)
{
	int i,k;
	int total;
	int nbval;

	int ret;

	stathisto * stattab;

	total=0;
	for(i=0;i<65536;i++)
	{
		total = total + histogram[i];
	}

	nbval=0;
	for(i=0;i<65536;i++)
	{
		if(histogram[i]) nbval++;
	}

	stattab=malloc(sizeof(stathisto) * nbval );
	memset(stattab,0,sizeof(stathisto) * nbval );
	
	k=0;
	for(i=0;i<65536;i++)
	{
		if(histogram[i])
		{
			stattab[k].occurence=histogram[i];
			stattab[k].val=i;
			stattab[k].pourcent=((float)stattab[k].occurence/(float)total)*(float)100;		
			k++;
		}
	}
	
#ifdef KFSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"---- Stream values ----");
	for(i=0;i<nbval;i++)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"%d> %d %f",stattab[i].val,stattab[i].occurence,stattab[i].pourcent);
	}
	floppycontext->hxc_printf(MSG_DEBUG,"----------------------");
#endif
	
	i=0;
	while(i<nbval)
	{
		if(stattab[i].occurence>512)
			break;
		i++;
	}

	if(i<nbval)
		ret=stattab[i].val;
	else
		ret=96;

	if(ret<107 && ret>=87)
		return 96;

	if(ret<87 && ret>68)
		return 80;

	if(ret<55 && ret>42)
		return 48;

	free(stattab);

	return ret;
}

int getcell(int * pumpcharge,int value,int centralvalue)
{
	float pump;
	float div;
	float fdiv;
	
	pump=(float)(*pumpcharge)/2;
	
	div=(float)value/(float)pump;
	
	fdiv=(float)floor(div);
	
	if(div!=fdiv)
	{
		if(div-fdiv>0.5)
		{
			*pumpcharge=*pumpcharge-1;
				
			if(*pumpcharge<(centralvalue-(int)((float)centralvalue*0.2)) )
			{
				*pumpcharge=(centralvalue-(int)((float)centralvalue*0.2));
			}
			
			return (int)(fdiv+1);
		}
		else
		{
			*pumpcharge=*pumpcharge+1;

			if(*pumpcharge>(centralvalue+(int)((float)centralvalue*0.2)) )
			{
				*pumpcharge=(centralvalue+(int)((float)centralvalue*0.2));
			}

			return (int)(fdiv);
		}
	}
	else
	{
		return (int)(div);
	}	
}

SIDE* ScanAndDecodeStream(int initalvalue,unsigned long * track,int size,short rpm)
{
	int pumpcharge;
	int i;
	unsigned long value;
	int nbcell;
	int cellcode;
	int centralvalue;
	int bitrate;

	int bitoffset;
	unsigned char *outtrack;
	unsigned long *trackbitrate;
	
	SIDE* hxcfe_track;

	centralvalue=initalvalue;

	initalvalue=centralvalue;
	pumpcharge=initalvalue;
	
	// Sync the "PLL"
	i=(size/8)*6;
	do
	{
		value=track[i];
		getcell(&pumpcharge,value,centralvalue);
		i++;
	}while(i<size);

	nbcell=0;
	i=0;
	do
	{
		value=track[i];
		nbcell=nbcell+getcell(&pumpcharge,value,centralvalue);

		i++;
	}while(i<size);

#define TEMPBUFSIZE 400000
	outtrack=(unsigned char*)malloc(TEMPBUFSIZE);
	trackbitrate=(unsigned long*)malloc(TEMPBUFSIZE*sizeof(unsigned long));
	memset(outtrack,0,TEMPBUFSIZE);
	memset(trackbitrate,0,TEMPBUFSIZE*sizeof(unsigned long));

	bitoffset=0;
	i=0;
	do
	{
		value=track[i];
		
		cellcode=getcell(&pumpcharge,value,centralvalue);
		
		bitoffset=bitoffset+cellcode;

		settrackbit(outtrack,0xFF,bitoffset,1);

		if(bitoffset>nbcell) 
		{
//			for(;;);
		}
		i++;
	}while(i<size);

	bitrate=(int)(24027428/centralvalue ); //KF_SCLOCK) );
	hxcfe_track = tg_alloctrack(bitrate,ISOFORMAT_DD,rpm,nbcell,2500,0,0);
	
	if(nbcell&7)
		memcpy(hxcfe_track->databuffer,outtrack,(nbcell/8)+1);
	else
		memcpy(hxcfe_track->databuffer,outtrack,(nbcell/8));

	free(outtrack);
	free(trackbitrate);

	return hxcfe_track;
}

SIDE* decodestream(HXCFLOPPYEMULATOR* floppycontext,char * file,short * rpm)
{
	double mck;
	double sck;
	double ick;
	int cellpos;
	int bitrate;
	unsigned long * histo;
	SIDE* currentside;
	
	s_track_dump *track_dump;
#ifdef KFSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"decodestream %s",file);
#endif

	currentside=0;

	track_dump=DecodeKFStreamFile(floppycontext,file);
	if(track_dump)
	{
	
		mck=((18432000 * 73) / 14) / 2;
		sck=mck/2;
		ick=mck/16;

		if(track_dump->nb_of_index>2)
		{
			histo=(unsigned long*)malloc(65536* sizeof(unsigned long));
			
			cellpos=track_dump->index_evt_tab[0].dump_offset;
			
			computehistogram(&track_dump->track_dump[cellpos],track_dump->index_evt_tab[1].dump_offset-track_dump->index_evt_tab[0].dump_offset,histo);

			bitrate=detectpeaks(floppycontext,histo);

			*rpm=(int)((float)(ick*(float)60)/(float)(track_dump->index_evt_tab[1].clk-track_dump->index_evt_tab[0].clk));

			floppycontext->hxc_printf(MSG_DEBUG,"Track %s : %d RPM, Bitrate: %d",getfilenamebase(file,0),*rpm,(int)(24027428/bitrate) );

			currentside=ScanAndDecodeStream(bitrate,&track_dump->track_dump[cellpos],track_dump->index_evt_tab[1].dump_offset-track_dump->index_evt_tab[0].dump_offset,*rpm);			

			free(histo);
			
		}

		FreeStream(track_dump);
	}

	return currentside;
}

int KryoFluxStream_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{	
	FILE * f;
	char * filepath;
	char * folder;
	char fname[512];
	int mintrack,maxtrack;
	int minside,maxside;
	short rpm;
	unsigned short i,j;
	int doublestep;
	CYLINDER* currentcylinder;

	int len;
	int found,track,side;
	struct stat staterep;
	s_oob_header oob;
	SIDE * curside;
	int nbtrack,nbside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libLoad_DiskFile");
	
	if(imgfile)
	{
		if(!stat(imgfile,&staterep))
		{
			
			len=getpathfolder(imgfile,0);
			folder=(char*)malloc(len+1);
			getpathfolder(imgfile,folder);

			if(staterep.st_mode&S_IFDIR)
			{
				sprintf(fname,"track");
			}
			else
			{
				getfilenamebase(imgfile,(char*)&fname);
				if(!strstr(fname,".0.raw") && !strstr(fname,".1.raw") )
				{
					free(folder);
					return HXCFE_BADFILE;
				}

				fname[strlen(fname)-8]=0;

			}

			filepath = malloc( strlen(imgfile) + 32 );

			doublestep=1;
			sprintf(filepath,"%s%s",folder,"doublestep");
			f=fopen(filepath,"rb");
			if(f)
			{
				doublestep=2;
				fclose(f);
			}
			
			track=0;
			side=0;
			found=0;
				
			mintrack=84;
			maxtrack=0;
			minside=1;
			maxside=0;

			do
			{
				sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,track,side);
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
					track=track+doublestep;
				}
			}while(track<84);

			if(!found)
			{
				free( folder );
				free( filepath );
				return HXCFE_BADFILE;
			}

			nbside=(maxside-minside)+1;
			nbtrack=(maxtrack-mintrack)+1;
			nbtrack=nbtrack/doublestep;

			floppycontext->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
			floppydisk->floppyNumberOfTrack=nbtrack;
			floppydisk->floppyNumberOfSide=nbside;
			floppydisk->floppySectorPerTrack=-1;

			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*doublestep;j=j+doublestep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{					
					sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,j,i);
					
					curside=decodestream(floppycontext,filepath,&rpm);

					if(!floppydisk->tracks[j/doublestep])
					{
						floppydisk->tracks[j/doublestep]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					}

					currentcylinder=floppydisk->tracks[j/doublestep];
					currentcylinder->sides[i]=curside;
				}
			}

			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			free( folder );
			free( filepath );
	
			return HXCFE_NOERROR;
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
