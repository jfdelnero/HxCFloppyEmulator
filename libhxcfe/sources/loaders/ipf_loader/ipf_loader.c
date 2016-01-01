/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : ipf_loader.c
// Contains: IPF floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
#ifdef IPF_SUPPORT

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "ipf_loader.h"
#include "ipf_writer.h"

#include "libhxcadaptor.h"

#define BYTE  int8_t
#define WORD  int16_t
#define DWORD int32_t
#define LIB_TYPE 1
#define LIB_USER 1
#include "./thirdpartylibs/capslib/Comtype.h"
#include "./thirdpartylibs/capslib/CapsAPI.h"

#include "capslibloader.h"

typedef SDWORD (__cdecl* CAPSINIT)(void);
typedef SDWORD (__cdecl* CAPSADDIMAGE)(void);
typedef SDWORD (__cdecl* CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSUNLOCKIMAGE)(SDWORD);
typedef SDWORD (__cdecl* CAPSLOADIMAGE)(SDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
typedef SDWORD (__cdecl* CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSUNLOCKTRACK)(SDWORD id, UDWORD cylinder, UDWORD head);
typedef SDWORD (__cdecl* CAPSUNLOCKALLTRACKS)(SDWORD);
typedef SDWORD (__cdecl* CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
typedef SDWORD (__cdecl* CAPSREMIMAGE)(SDWORD id);

extern  CAPSINIT pCAPSInit;
extern  CAPSADDIMAGE pCAPSAddImage;
extern  CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory;
extern  CAPSUNLOCKIMAGE pCAPSUnlockImage;
extern  CAPSLOADIMAGE pCAPSLoadImage;
extern  CAPSGETIMAGEINFO pCAPSGetImageInfo;
extern  CAPSLOCKTRACK pCAPSLockTrack;
extern  CAPSUNLOCKTRACK pCAPSUnlockTrack;
extern  CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks;
extern  CAPSGETVERSIONINFO pCAPSGetVersionInfo;
extern  CAPSREMIMAGE pCAPSRemImage;

int IPF_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IPF_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"ipf") || hxc_checkfileext(imgfile,"ct") || hxc_checkfileext(imgfile,"ctr"))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IPF_libIsValidDiskFile : IPF file !");
		if(init_caps_lib())
		{
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"IPF_libIsValidDiskFile : No Caps lib available!");
			return HXCFE_INTERNALERROR;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IPF_libIsValidDiskFile : non IPF file !");
		return HXCFE_BADFILE;
	}
}

uint32_t trackcopy(unsigned char * dest,unsigned char * src,uint32_t overlap,uint32_t tracklen,unsigned char nooverlap)
{
	uint32_t i,j,k,dest_tracklen;


	k=0;
	j=0;

	for(i=0;i<overlap;i++)
	{
		if( (src[k>>3]>>(0x7-(k&0x7)))&1)
		{
			dest[j>>3]=dest[j>>3]|(0x80>>(j&0x7));
		}
		else
		{
			dest[j>>3]=dest[j>>3]&(~((0x80)>>(j&0x7)));
		}

		j++;
		if(j>=tracklen) j=0;

		k++;
		if(k>=tracklen) k=0;

	}

	dest_tracklen=tracklen;
/*	if(tracklen&0x7)
	{
	  j=j+(((tracklen&~0x7)+8)-tracklen);
	  dest_tracklen=tracklen+(((tracklen&~0x7)+8)-tracklen);
	}*/

	if(!nooverlap)
	{
		for(i=0;i<1;i++)
		{
			dest[j>>3]=dest[j>>3]&(~((0x80)>>(j&0x7)));
			j++;
			if(j>=tracklen) j=0;
			dest_tracklen++;
		}
	}

	for(i=overlap;i<tracklen;i++)
	{
		if( (src[k>>3]>>(0x7-(k&0x7)))&1)
		{
			dest[j>>3]=dest[j>>3]|(0x80>>(j&0x7));
		}
		else
		{
			dest[j>>3]=dest[j>>3]&(~((0x80)>>(j&0x7)));
		}

		j++;
		if(j>=dest_tracklen)
		{
			j=0;
		}

		k++;
		if(k>=tracklen)
		{
			k=0;
		}
	}

	return dest_tracklen;
}

int IPF_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	unsigned int filesize;
	unsigned int i,j,k,l,m,len;
	unsigned char *fileimg;
	unsigned char oldlib;
	typedef struct CapsImageInfo CapsImageInfo_;
	struct CapsTrackInfoT1 ti;
	struct CapsTrackInfoT1 flakeyti;
	struct CapsVersionInfo cvi;

	unsigned char * temptrack;
	CapsImageInfo_ ci2;
	FILE * f;
	int img;
	int ret;
	int progresscnt;
	int overlap;
	int intrackflakeybit;
	uint32_t bitrate;
	int rpm,sizefactor;
	HXCFE_CYLINDER* currentcylinder;
	unsigned char flakeybyte;
	HXCFE_SIDE* currentside;

	UDWORD flag;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IPF_libLoad_DiskFile %s",imgfile);

	progresscnt = 0;

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{
		fileimg=(unsigned char*)malloc(filesize);

		if(fileimg!=NULL)
		{
			i=0;
			do
			{
				fread(fileimg+(i*1024),1024,1,f);
				i++;
			}while(i<((filesize/1024)+1));

		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Memory error!");
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"0 byte file!");
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	hxc_fclose(f);

	cvi.type = LIB_TYPE;
	pCAPSGetVersionInfo (&cvi, 0);

	flag = DI_LOCK_DENVAR/*|DI_LOCK_DENNOISE|DI_LOCK_NOISE*/|DI_LOCK_UPDATEFD|DI_LOCK_TYPE | DI_LOCK_INDEX;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CAPS: library version %d.%d (flags=%08X)", cvi.release, cvi.revision, cvi.flag);
	oldlib = (cvi.flag & (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT)) != (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT);
	sizefactor=8;

	if (!oldlib)
	{
		sizefactor=1;
		flag |= DI_LOCK_TRKBIT | DI_LOCK_OVLBIT;
	}

//	for (i = 0; i < 4; i++)
//		caps_cont[i] = pCAPSAddImage ();


	//flag=DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;


	img=pCAPSAddImage();

	if(img!=-1)
	{
		if(pCAPSLockImageMemory(img, fileimg,filesize,0)!=imgeUnsupported )
		{
			pCAPSLoadImage(img, flag);
			pCAPSGetImageInfo(&ci2, img);
			////// debug //////
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Image Info: %s",imgfile);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Type     : %.8x",ci2.type);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Release  : %.8x",ci2.release);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Revision : %.8x",ci2.revision);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Platform : %.8x",ci2.platform);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"minhead  : %d",ci2.minhead);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"maxhead  : %d",ci2.maxhead);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"mincylinder : %d",ci2.mincylinder);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"maxcylinder : %d",ci2.maxcylinder);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Date : %d/%d/%d",ci2.crdt.day,ci2.crdt.month,ci2.crdt.year);
			///////////////////

			if(ci2.type == ciitFDD)
			{
				floppydisk->floppySectorPerTrack=0;
				floppydisk->floppyNumberOfSide=(unsigned char)(ci2.maxhead-ci2.minhead)+1;
				floppydisk->floppyNumberOfTrack=(unsigned char)ci2.maxcylinder+1;
				floppydisk->floppyBitRate=250000;
				rpm=300;
				floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
				floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

				for(i=ci2.mincylinder;i<=ci2.maxcylinder;i++)
				{
					for(j=ci2.minhead;j<=ci2.maxhead;j++)
					{
						hxcfe_imgCallProgressCallback(imgldr_ctx,progresscnt,((ci2.maxcylinder-ci2.mincylinder)+1) * ((ci2.maxhead - ci2.minhead)+1));

						progresscnt++;
						ti.type = LIB_TYPE;
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"-----------------------------%d %d",i,j);
						ret=pCAPSLockTrack((struct CapsTrackInfo *)&ti, img, i, j, flag);
						if(ret==imgeOk)
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track Info  : %d %d",i,j);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Cylinder    : %d",ti.cylinder);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Head        : %d",ti.head);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sectorcnt   : %d",ti.sectorcnt);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"sectorsize  : %d",ti.sectorsize);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Type        : %.8X",ti.type);
							//	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"trackcnt    : %d\n",ti.trackcnt);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"tracklen     : %d %s",ti.tracklen*sizefactor,(ti.tracklen*sizefactor)&0x7?"non aligned !":"aligned");
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"overlap     : %d %s",ti.overlap*sizefactor,(ti.overlap*sizefactor)&0x7?"non aligned !":"aligned");

							if(!floppydisk->tracks[i])
							{
								floppydisk->tracks[i]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
								currentcylinder=floppydisk->tracks[i];
								currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
								currentcylinder->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
								memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);

								currentcylinder->floppyRPM=rpm;
							}

							currentcylinder->sides[j]=malloc(sizeof(HXCFE_SIDE));
							memset(currentcylinder->sides[j],0,sizeof(HXCFE_SIDE));
							currentside=currentcylinder->sides[j];

							currentside->flakybitsbuffer=0;
							currentside->track_encoding=AMIGA_MFM_ENCODING;

							intrackflakeybit=-1;

							////////////////////////////////////////////////////////////////
							// get track data & flakey bit(s)
							if(!( (ti.type & CTIT_MASK_TYPE) == ctitNoise) && ti.trackbuf)
							{

								currentside->tracklen=ti.tracklen*sizefactor;
								len=(currentside->tracklen>>3)+16;

								currentside->databuffer=malloc(len);
								memset(currentside->databuffer,0,len);

								//flakey bits in track ?
								if((ti.type & CTIT_FLAG_FLAKEY) && hxc_checkfileext(imgfile,"ipf") )
								{
									// This method is only valid with ipf files (fixed track size at each revolution...)
									temptrack=(unsigned char *)malloc(len);
									if(temptrack)
									{
										memset(temptrack,0,len);

										overlap=0;
										if(ti.overlap>=0)
											overlap=ti.overlap*sizefactor;

										currentside->tracklen=trackcopy(currentside->databuffer,ti.trackbuf,overlap,ti.tracklen,(unsigned char)((ti.overlap<0)?0xFF:0x00));

										currentside->flakybitsbuffer = malloc(len);
										if(currentside->flakybitsbuffer)
										{
											memset(currentside->flakybitsbuffer,0x00,len);

											pCAPSUnlockTrack(img,i, j);

											// try to read the track x time, and check for differences
											for(k=0;k<10;k++)
											{
												flakeyti.type = LIB_TYPE;
												ret=pCAPSLockTrack((struct CapsTrackInfo *)&flakeyti, img, i, j, flag);
												if(ret==imgeOk)
												{

													overlap=0;
													if(ti.overlap>=0)
														overlap=flakeyti.overlap;

													trackcopy(temptrack,flakeyti.trackbuf,overlap,flakeyti.tracklen,(unsigned char)((ti.overlap<0)?0xFF:0x00));

													for(l=0;l<ti.tracklen>>3;l++)
													{
														flakeybyte= temptrack[l] ^ currentside->databuffer[l];
														for(m=0;m<8;m=m+2)
														{
															// a flaybit detected ?
															if((flakeybyte&(0xC0>>m)) || ((currentside->databuffer[l]&(0xC0>>m))==(0xC0>>m) ) )
															{
																currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l] | (0xC0>>m);
																currentside->databuffer[l]=currentside->databuffer[l] & ~(0xC0>>m);
																intrackflakeybit=l;
															}
														}

													}
													pCAPSUnlockTrack(img,i, j);
												}
											}

											ti.type = LIB_TYPE;
											ret = pCAPSLockTrack((struct CapsTrackInfo *)&ti, img, i, j, flag);
										}

										free(temptrack);
									}

								}
								else
								{

									overlap=0;
									if(ti.overlap>0)
										overlap=ti.overlap*sizefactor;

									currentside->tracklen=trackcopy(currentside->databuffer,ti.trackbuf,overlap,ti.tracklen,(unsigned char)((ti.overlap<0)?0xFF:0x00));
								}
							}
							else
							{
								currentside->tracklen=12500*8;
								len=currentside->tracklen>>3;
								currentside->databuffer=malloc(len);
								memset(currentside->databuffer,0,len);
								currentside->flakybitsbuffer=malloc(len);
								memset(currentside->flakybitsbuffer,0xFF,len);
							}

							bitrate=((rpm/60)*currentside->tracklen)>>1;

							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Fixed bitrate    : %d",bitrate);
							currentside->timingbuffer=0;
							currentside->bitrate=bitrate;
							currentside->track_encoding=AMIGA_MFM_ENCODING;
							////////////////////////////////////////////////////////////////
							// get track timing
							if(ti.timebuf!=0)// && (ti.type & CTIT_FLAG_FLAKEY))
							{
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Variable bit rate!!!");
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"timelen     : %d",ti.timelen);
								len=ti.timelen*sizeof(uint32_t);
								if(currentside->tracklen&7) len=len+4;
								currentside->timingbuffer=malloc(len);
								memset(currentside->timingbuffer,0,len);
								k=0;
								do
								{
									currentside->timingbuffer[k]=((1000-ti.timebuf[k])*(bitrate/1000))+bitrate;
									k++;
								}while(k<ti.timelen);

								if(currentside->tracklen&7)
								{
									currentside->timingbuffer[currentside->tracklen>>3]=currentside->timingbuffer[ti.timelen-1];
								}

								currentside->bitrate=VARIABLEBITRATE;
							}

							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"timebuf     : %.8X",ti.timebuf);

							if(ti.type & CTIT_FLAG_FLAKEY)
							{
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Side %d: CTIT_FLAG_FLAKEY",i,j);
							}


							if((ti.type & CTIT_MASK_TYPE) ==  ctitNoise)
							{
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Side %d: cells are unformatted (random size)",i,j);
							}


							if(intrackflakeybit !=  -1)
							{
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"In track flakey bit found (last: %d)",intrackflakeybit);
							}

							pCAPSUnlockTrack(img,i, j);
							if(floppydisk->floppySectorPerTrack < (int32_t)ti.sectorcnt )
								floppydisk->floppySectorPerTrack = (int32_t)ti.sectorcnt;

							currentside->number_of_sector=ti.sectorcnt;

							len=currentside->tracklen>>3;
							if(currentside->tracklen&0x7) len++;


							currentside->indexbuffer=malloc(len);
							memset(currentside->indexbuffer,0,len);
							fillindex(-2500,currentside,2500,TRUE,0);
							//fillindex(0,currentside,3200,0);

						}
						else
						{

							if(!floppydisk->tracks[i])
							{
								floppydisk->tracks[i]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
								currentcylinder=floppydisk->tracks[i];
								currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
								currentcylinder->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
								memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);

								currentcylinder->floppyRPM=rpm;
							}
							else
							{
								currentcylinder=floppydisk->tracks[i];
							}

							currentcylinder->sides[j]=malloc(sizeof(HXCFE_SIDE));
							memset(currentcylinder->sides[j],0,sizeof(HXCFE_SIDE));
							currentside=currentcylinder->sides[j];

							// error -> random track
							currentside->timingbuffer=0;
							currentside->bitrate = 250000;
							currentside->tracklen=12500 * 8;
							currentside->databuffer=malloc(currentside->tracklen>>3);
							memset(currentside->databuffer,0x11,currentside->tracklen>>3);

							currentside->flakybitsbuffer=malloc(currentside->tracklen>>3);
							memset(currentside->flakybitsbuffer,0xFF,currentside->tracklen>>3);

							currentside->indexbuffer=malloc(currentside->tracklen>>3);
							memset(currentside->indexbuffer,0,currentside->tracklen>>3);
							//fillindex(-2500,currentside,2500,TRUE,0);

						}

					}

				}

			}

			pCAPSUnlockAllTracks(img);

		}

		pCAPSUnlockImage(img);


		pCAPSRemImage(img);

		free(fileimg);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"IPF Loader : tracks file successfully loaded and encoded!");
		return HXCFE_NOERROR;


	}


	return HXCFE_INTERNALERROR;
}

int IPF_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="SPS_IPF";
	static const char plug_desc[]="SPS IPF Loader";
	static const char plug_ext[]="ipf";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	IPF_libIsValidDiskFile,
		(LOADDISKFILE)		IPF_libLoad_DiskFile,
		(WRITEDISKFILE)		IPF_libWrite_DiskFile,
		(GETPLUGININFOS)	IPF_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}


#endif
