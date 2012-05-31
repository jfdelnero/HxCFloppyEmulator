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

#define BASEINDEX 1

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
		memset(&staterep,0,sizeof(struct stat));
		if(!hxc_stat(imgfile,&staterep))
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
					f=hxc_fopen(filepath,"rb");
					if(f)
					{
						fread(&oob,sizeof(s_oob_header),1,f);
						if(oob.Sign==OOB_SIGN)
						{
							found=1;
						}
						hxc_fclose(f);
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

				f=hxc_fopen(imgfile,"rb");
				if(f)
				{
					fread(&oob,sizeof(s_oob_header),1,f);
					if( ( oob.Sign == OOB_SIGN ) && ( oob.Type>=1 && oob.Type<=4 ) )
					{
						found=1;
					}
					hxc_fclose(f);

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

void settrackbit(unsigned char * dstbuffer,int dstsize,unsigned char byte,int bitoffset,int size)
{
	int i,j,k;

	k=0;
	i=bitoffset;
	for(j=0;j<size;j++)
	{
		if(byte&((0x80)>>(j&7)))
			dstbuffer[(i>>3)]=dstbuffer[(i>>3)]|( (0x80>>(i&7)));
		else
			dstbuffer[(i>>3)]=dstbuffer[(i>>3)]&(~(0x80>>(i&7)));

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

	int total250k;
	int total300k;
	int total500k;

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
	
	total250k=0;
	total300k=0;
	total500k=0;

	i=0;
	while(i<nbval)
	{
		ret=stattab[i].val;

		if(ret<107 && ret>=87)
			total250k=total250k+stattab[i].occurence;

		if(ret<87 && ret>68)
			total300k=total300k+stattab[i].occurence;

		if(ret<55 && ret>42)
			total500k=total500k+stattab[i].occurence;

		i++;
	}

	if(total500k>2048)
		return 48;

	if(total300k>2048)
		return 80;

	if(total250k>2048)
		return 96;


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
				
			if(*pumpcharge< ( centralvalue - ( ( centralvalue * 20 ) / 100 ) ) )
			{
				*pumpcharge = ( centralvalue - ( ( centralvalue * 20 ) / 100 ) );
			}
			
			return (int)(fdiv+1);
		}
		else
		{
			*pumpcharge=*pumpcharge+1;

			if(*pumpcharge> ( centralvalue + ( ( centralvalue * 20 ) / 100 ) ) )
			{
				*pumpcharge= ( centralvalue + ( ( centralvalue * 20 ) / 100 ) ) ;
			}

			return (int)(fdiv);
		}
	}
	else
	{
		return (int)(div);
	}	
}

typedef struct s_match_
{
	int yes;
	int no;
	unsigned long offset;
}s_match;


void exchange(s_match *  table, int a, int b)
{
    s_match temp;
	temp = table[a];
    table[a] = table[b];
    table[b] = temp;
}

void quickSort(s_match * table, int start, int end)
{
    int left = start-1;
    int right = end+1;
    const int pivot = table[start].yes;

    if(start >= end)
        return;

    while(1)
    {
        do right--; while(table[right].yes > pivot);
        do left++; while(table[left].yes < pivot);

        if(left < right)
            exchange(table, left, right);
        else break;
    }

    quickSort(table, start, right);
    quickSort(table, right+1, end);
}

char* AnalyzeAndFoundOverLap(HXCFLOPPYEMULATOR* floppycontext,s_track_dump* td,int centralvalue,int *start,int *end)
{
	#define NUMBEROFTRY 1536
	#define PERCENTERROR 20

	unsigned char * valid_page1;
	int time1,time2;
	unsigned long i,j,k,l,c;
	s_match * matchtab;
	int nb_pulses,nb_flakey_pulses;
	int number_of_entry;

	unsigned long tracktimeoffset_1,tracktimeoffset_2;
	int index_pos;
	int pourcent_error;

	index_pos=BASEINDEX;

	number_of_entry = NUMBEROFTRY;
	if( ( td->index_evt_tab[index_pos+1].dump_offset + ( number_of_entry / 2 ) ) >= td->nb_of_pulses )
	{
		number_of_entry = ( (td->index_evt_tab[index_pos+1].dump_offset - td->index_evt_tab[index_pos].dump_offset) ) / 2;
	}

	matchtab=malloc(sizeof(s_match) * number_of_entry );
	memset(matchtab,0,sizeof(s_match) * number_of_entry );

	for(k=0;k<number_of_entry;k++)
	{
		if(k&1)
			matchtab[k].offset= ( td->index_evt_tab[index_pos+1].dump_offset - ( k>>1 ) );
		else
			matchtab[k].offset= ( td->index_evt_tab[index_pos+1].dump_offset + ( k>>1 ) );

	}

	pourcent_error = PERCENTERROR;

	for(k=0;k<number_of_entry;k++)
	{
		j=matchtab[k].offset;
		i=td->index_evt_tab[index_pos].dump_offset;

		c = ( td->index_evt_tab[index_pos+1].dump_offset - td->index_evt_tab[index_pos].dump_offset ) + (number_of_entry*2);

		time1 = td->track_dump[i];
		time2 = td->track_dump[j];

		do
		{
			if( 
				( time2 <= ( time1 + ( ( time1 * pourcent_error ) / 100 ) ) ) &&
				( time2 >= ( time1 - ( ( time1 * pourcent_error ) / 100 ) ) )    )
			{
				matchtab[k].yes++;
				time1 = td->track_dump[++i];
				time2 = td->track_dump[++j];

				pourcent_error = PERCENTERROR;

			}
			else
			{
				if(time1<time2)
				{
					time1 = time1 + td->track_dump[++i];
				}
				else
				{
					time2 = time2 + td->track_dump[++j];
				}
				matchtab[k].no++;
				pourcent_error = 3;

			}

		}while(--c);

		if(matchtab[k].no<10)
			k=number_of_entry;
	}

	quickSort(matchtab, 0, number_of_entry-1);

	nb_pulses = ( td->index_evt_tab[index_pos+1].dump_offset - td->index_evt_tab[index_pos].dump_offset ) +
				( matchtab[number_of_entry-1].offset - td->index_evt_tab[index_pos+1].dump_offset );

	*start=matchtab[number_of_entry-1].offset;
	*end=matchtab[number_of_entry-1].offset + nb_pulses;
	if(*end>= td->nb_of_pulses) *end=td->nb_of_pulses-1;
	if(*start>= td->nb_of_pulses) *start=td->nb_of_pulses-1;

	nb_flakey_pulses = 0;

	valid_page1 = malloc( nb_pulses * sizeof(char) );
	if(valid_page1)
	{
		memset(valid_page1,0, nb_pulses * sizeof(char) );

		j= matchtab[number_of_entry-1].offset;
		i= td->index_evt_tab[index_pos].dump_offset;

		tracktimeoffset_1 = 0;
		tracktimeoffset_2 = 0;

		l=0;
		c = nb_pulses;

		pourcent_error = PERCENTERROR;
		time1 = td->track_dump[i];
		time2 = td->track_dump[j];

		tracktimeoffset_1 = time1;
		tracktimeoffset_2 = time2;

		do
		{
			if( 
				( time2 <= ( time1 + ( ( time1 * pourcent_error ) / 100 ) ) ) &&
				( time2 >= ( time1 - ( ( time1 * pourcent_error ) / 100 ) ) )    )
			{
				
				time1 = td->track_dump[++i];
				time2 = td->track_dump[++j];

				tracktimeoffset_1 = tracktimeoffset_1 + time1;
				tracktimeoffset_2 = tracktimeoffset_2 + time2;

				pourcent_error = PERCENTERROR;

				c--;
			}
			else
			{
				if(time1<time2)
				{
					valid_page1[i - td->index_evt_tab[index_pos].dump_offset]=-1;
					time1 = time1 + td->track_dump[++i];

					tracktimeoffset_1 = tracktimeoffset_1 + td->track_dump[i];

					c--;
				}
				else
				{
					valid_page1[i - td->index_evt_tab[index_pos].dump_offset]=-1;
					time2 = time2 + td->track_dump[++j];

					tracktimeoffset_2 = tracktimeoffset_2 + td->track_dump[j];
				}

				pourcent_error = 3;

				nb_flakey_pulses++;
			}

		}while(c);
	}


	floppycontext->hxc_printf(MSG_DEBUG,"Track Len: %d pulses - Track overlap : %d - Flakey/weak bits : %d",
								nb_pulses,
								matchtab[number_of_entry-1].offset-td->index_evt_tab[index_pos+1].dump_offset,
								nb_flakey_pulses);
	
	free(matchtab);

	return valid_page1;
}


SIDE* ScanAndDecodeStream(int initalvalue,unsigned long * track,char * flakey,int size,short rpm)
{
#define TEMPBUFSIZE 256*1024
	int pumpcharge;
	int i,j;
	unsigned long value;
	int cellcode;
	int centralvalue;
	int bitrate;

	int bitoffset;
	int tracksize;
	unsigned char *outtrack;
	unsigned char *flakeytrack;
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

	outtrack=(unsigned char*)malloc(TEMPBUFSIZE);
	flakeytrack=(unsigned char*)malloc(TEMPBUFSIZE);
	trackbitrate=(unsigned long*)malloc(TEMPBUFSIZE*sizeof(unsigned long));

	memset(outtrack,0,TEMPBUFSIZE);
	memset(flakeytrack,0,TEMPBUFSIZE);
	memset(trackbitrate,0,TEMPBUFSIZE*sizeof(unsigned long));

	for(i=0;i<TEMPBUFSIZE;i++)
	{
		trackbitrate[i] = 250000; //(int)(24027428/centralvalue);
	}

	bitoffset=0;
	i=0;
	do
	{
		value = track[i];
		
		cellcode = getcell(&pumpcharge,value,centralvalue);

		bitoffset = bitoffset + cellcode;

		settrackbit(outtrack,TEMPBUFSIZE,0xFF,bitoffset,1);

		if(flakey[i]<0)
			settrackbit(flakeytrack,TEMPBUFSIZE,0xFF,bitoffset,1);

		trackbitrate[(bitoffset>>3)]=(int)(24027428/pumpcharge);

		i++;
	}while(i<size);

	i=0;
	do
	{

		j=0;
		bitrate=0;
		while(((i+j)<size) && j<128)
		{
			bitrate = ( bitrate + trackbitrate[(i+j)] );
			j++;
		}
		bitrate=bitrate/128;

		j=0;
		while(((i+j)<size) && j<128)
		{
			trackbitrate[(i+j)] = bitrate;
			j++;
		}

		i = i + 128;

	}while(i<size);

	bitrate=(int)( 24027428 / centralvalue );
	hxcfe_track = tg_alloctrack(bitrate,ISOFORMAT_DD,rpm,bitoffset,3000,-3000,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_ALLOCTIMIMGBUFFER);

	
	if(bitoffset&7)
	{
		tracksize = ( bitoffset >> 3 ) + 1;
	}
	else
	{
		tracksize = ( bitoffset >> 3 );
	}

	if( tracksize >= TEMPBUFSIZE ) tracksize = TEMPBUFSIZE - 1;

	memcpy(hxcfe_track->databuffer,outtrack,tracksize);
	memcpy(hxcfe_track->flakybitsbuffer,flakeytrack,tracksize);
	memcpy(hxcfe_track->timingbuffer,trackbitrate, tracksize * sizeof(unsigned long));

	free(outtrack);
	free(flakeytrack);
	free(trackbitrate);

	return hxcfe_track;
}

SIDE* decodestream(HXCFLOPPYEMULATOR* floppycontext,char * file,short * rpm,float timecoef)
{
	double mck;
	double sck;
	double ick;
	int cellpos;
	int bitrate;
	unsigned long * histo;
	SIDE* currentside;
	
	char * flakeytab;
	int start,end;

	s_track_dump *track_dump;
	//int i,j;

	int index_pos;

	index_pos=BASEINDEX;
	
#ifdef KFSTREAMDBG
	floppycontext->hxc_printf(MSG_DEBUG,"decodestream %s",file);
#endif

	currentside=0;

	track_dump=DecodeKFStreamFile(floppycontext,file,timecoef);
	if(track_dump)
	{
	
		mck=((18432000 * 73) / 14) / 2;
		sck=mck/2;
		ick=mck/16;

		if(track_dump->nb_of_index>2)
		{
			histo=(unsigned long*)malloc(65536* sizeof(unsigned long));
			
			cellpos=track_dump->index_evt_tab[index_pos].dump_offset;
			
			computehistogram(&track_dump->track_dump[cellpos],track_dump->index_evt_tab[index_pos+1].dump_offset-track_dump->index_evt_tab[index_pos].dump_offset,histo);

			bitrate=detectpeaks(floppycontext,histo);

			*rpm=(int)((float)(ick*(float)60)/(float)(track_dump->index_evt_tab[index_pos+1].clk-track_dump->index_evt_tab[index_pos].clk));

			floppycontext->hxc_printf(MSG_DEBUG,"Track %s : %d RPM, Bitrate: %d",getfilenamebase(file,0),*rpm,(int)(24027428/bitrate) );
			
			flakeytab=AnalyzeAndFoundOverLap(floppycontext,track_dump,bitrate,&start,&end);

			currentside=ScanAndDecodeStream(bitrate,&track_dump->track_dump[start],flakeytab,(end-start),*rpm);

			free(histo);
			free(flakeytab);

			/*j=currentside->tracklen/8;
			if(currentside->tracklen&7) j++;
			for(i=0;i<j;i++)
			{
				floppycontext->hxc_printf(MSG_DEBUG,"D:%.2X\tM:%.2X\tBR:%.8d\tI:%.2X",
						currentside->databuffer[i],
						currentside->flakybitsbuffer[i],
						currentside->timingbuffer[i],
						currentside->indexbuffer[i]);
			}*/
			
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
	int minside,maxside,singleside;
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
	float timecoef;
	
	floppycontext->hxc_printf(MSG_DEBUG,"KryoFluxStream_libLoad_DiskFile");
	
	if(imgfile)
	{
		if(!hxc_stat(imgfile,&staterep))
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
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				doublestep=2;
				hxc_fclose(f);
			}

			singleside=0;
			sprintf(filepath,"%s%s",folder,"singleside");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				singleside=1;
				hxc_fclose(f);
			}

			timecoef=1;
			sprintf(filepath,"%s%s",folder,"rpm360rpm300");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)1.2;
				hxc_fclose(f);
			}

			sprintf(filepath,"%s%s",folder,"rpm300rpm360");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)0.833;
				hxc_fclose(f);
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
				f=hxc_fopen(filepath,"rb");
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
					hxc_fclose(f);
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
			if(singleside)
				nbside = 1;
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
					
					curside=decodestream(floppycontext,filepath,&rpm,timecoef);

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
