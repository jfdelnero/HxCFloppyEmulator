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
// File : usb_hxcfloppyemulator.c
// Contains: USB HxC FE support functions
//
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#endif

//#define DEBUGVB 1

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"

#include "ftdi_api.h"
#include "libhxcadaptor.h"
#include "variablebitrate.h"

// fonction permettant de changer l'etat des signaux ready, dskchg et de source de selection
// dans un buffer
void patchbuffer(uint8_t * buffer, uint32_t bufferlen,uint8_t patchvalue)
{
	uint32_t i;

	i=0;
	do
	{
		if(((buffer[i]&0xF)==0xC) || ((buffer[i]&0xF0)==0xC0))
		{
			buffer[i+1]=buffer[i+1]|patchvalue;
		}
		i=i+2;
	}while(i<bufferlen);
}


//permet de detecter sur un octet est une commande ou un code MFM
uint8_t iscmd(uint8_t c)
{
	uint8_t c2;

	c2=(c>>4)&0xF;
	if((c==0x7) || (c==0xD) || (c==0xC) || (c2==0x7) || (c2==0xD) || (c2==0xC))
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

// fonction generant des code mfm al�atoire dans un buffer MFM
// (emulation track non formate / weak bits)
void randomizebuffer(USBHXCFE * hw_context,uint8_t * buffer,uint8_t * randombuffer, uint32_t bufferlen)
{
	uint32_t i,j;
	uint8_t * rand_lut;
	uint8_t rand_mask;
	uint8_t data;

	rand_lut = hw_context->randomlut;

	i=0;
	do
	{
		rand_mask = randombuffer[i];

		if(rand_mask)
		{
			data = buffer[i];

			if(!iscmd(data))
			{
				data = (data & ~rand_mask) | ( rand_lut[rand()&0x3FF] & rand_mask );

				for(j=0;j<7;j++)
				{
					if((data & (0xC0>>j)) == (0xC0>>j))
					{
						data = data ^ (0x40>>j);
					}
				}

				buffer[i] = data;
			}
			else
			{
				i++;
			}
		}
		i++;
	}while(i<bufferlen);
}


// fonction de generation du prochain paquet USB a envoyer
int32_t FillBuffer(HXCFE* floppycontext,USBHXCFE * hw_context,uint8_t * paquetbuffer,int32_t * headmoved)
{
	int32_t t;
	int32_t trackposition;
	static int32_t l=0;
	int32_t floppypin34;
	int32_t floppypin2;
	int32_t writeprotect;
	int32_t amigaready;
	uint32_t bytecopied;
	uint32_t final_buffer_len;
	uint8_t * trackptr;
	uint8_t * randomtrackptr;
	uint32_t floppydisable;
	uint8_t ctrl_byte;


		if(hw_context->current_track<hw_context->number_of_track)
		{
			trackposition=hw_context->current_track;
			if(trackposition<0)trackposition=0;
		}
		else
		{
			trackposition=hw_context->number_of_track-1;
			if(trackposition<0)trackposition=0;
		}


		final_buffer_len=hw_context->precalcusbtrack[trackposition].tracklen;
		trackptr=hw_context->precalcusbtrack[trackposition].usbtrack;
		randomtrackptr=hw_context->precalcusbtrack[trackposition].randomusbtrack;

		// selection des bon etats pour les signaux ready et diskchg
		// selon la machine cible
		floppypin34=0;
		floppypin2=0;
		amigaready=0;
		writeprotect=0;

		switch(hw_context->interface_mode)
		{
		case AMIGA_DD_FLOPPYMODE: // amiga
		case AMIGA_HD_FLOPPYMODE: // amiga
			amigaready=1;
			floppypin34=1;

			if(hw_context->floppychanged)
			{
				floppypin2=1;

				if(l>200)
				{
					//hw_context->floppychanged=0;
					if(*headmoved)
					{
						*headmoved=0;
						floppypin2=0;
						hw_context->floppychanged=0;
						l=0;
					}

				}
				else
				{
					l++;
				}
			}
			else
			{
				floppypin2=0;
				*headmoved=0;
			}


			break;

		case IBMPC_DD_FLOPPYMODE: // pc / st 720k
		case ATARIST_DD_FLOPPYMODE:
			amigaready=0;
			floppypin2=0;
			if(hw_context->floppychanged)
			{
				floppypin34=1;
				writeprotect=1;
				if(l>100)
				{
					if(*headmoved)
					{
						*headmoved=0;
						floppypin34=0;
						hw_context->floppychanged=0;
						l=0;
					}

					writeprotect=0;
				}
				else
				{
					l++;
				}
			}
			else
			{
				floppypin34=0;
				*headmoved=0;
			}


			break;
		case IBMPC_HD_FLOPPYMODE: // pc / st 1.44MB
		case ATARIST_HD_FLOPPYMODE:
			amigaready=0;
			floppypin2=1;

			if(hw_context->floppychanged)
			{
				floppypin34=1;
				writeprotect=1;

				if(l>100)
				{
					if(*headmoved)
					{
						*headmoved=0;
						floppypin34=0;
						hw_context->floppychanged=0;
						l=0;
					}
					writeprotect=0;
				}
				else
				{
					l++;
				}
			}
			else
			{
				floppypin34=0;
				*headmoved=0;
			}
			break;

		case MSX2_DD_FLOPPYMODE:
		case CPC_DD_FLOPPYMODE: // cpc / korg dss1
			amigaready=0;
			floppypin2=0;
			floppypin34=1;
			break;


		case GENERIC_SHUGART_DD_FLOPPYMODE:
		case EMU_SHUGART_FLOPPYMODE:
			amigaready=0;
			floppypin2=0;
			floppypin34=1;
			 break;

		default:
			amigaready=0;
			floppypin2=0;
			floppypin34=1;
			break;
		}


	bytecopied=0;
	if(final_buffer_len && randomtrackptr && trackptr)
	{

		hw_context->trackbuffer_pos=hw_context->trackbuffer_pos % final_buffer_len;
		if((hw_context->trackbuffer_pos+hw_context->usbstats.packetsize)<final_buffer_len)
		{
			memcpy(paquetbuffer,trackptr+hw_context->trackbuffer_pos,hw_context->usbstats.packetsize);
			randomizebuffer(hw_context,paquetbuffer,randomtrackptr+hw_context->trackbuffer_pos, hw_context->usbstats.packetsize);

			hw_context->trackbuffer_pos=hw_context->trackbuffer_pos+hw_context->usbstats.packetsize;
			bytecopied=bytecopied+hw_context->usbstats.packetsize;
		}
		else
		{ // roll over
			t=(hw_context->trackbuffer_pos+hw_context->usbstats.packetsize)-final_buffer_len;
			memcpy(paquetbuffer,trackptr+hw_context->trackbuffer_pos,hw_context->usbstats.packetsize-t);
			randomizebuffer(hw_context,paquetbuffer,randomtrackptr+hw_context->trackbuffer_pos, hw_context->usbstats.packetsize-t);
			hw_context->trackbuffer_pos=0;
			memcpy(&paquetbuffer[(hw_context->usbstats.packetsize-t)],trackptr+hw_context->trackbuffer_pos,t);
			randomizebuffer(hw_context,&paquetbuffer[(hw_context->usbstats.packetsize-t)],randomtrackptr+hw_context->trackbuffer_pos,t);
			hw_context->trackbuffer_pos=hw_context->trackbuffer_pos+t;
			bytecopied=bytecopied+hw_context->usbstats.packetsize;
		}

		//on verifie qu'il n'y a pas de commande a cheval entre 2 buffers
		// si oui on corrige en ajoutant 1 octet au buffer
		if(iscmd(paquetbuffer[hw_context->usbstats.packetsize-1]))
		{
			if(!iscmd(paquetbuffer[hw_context->usbstats.packetsize-2]))
			{
				paquetbuffer[hw_context->usbstats.packetsize]=*(trackptr+hw_context->trackbuffer_pos);
				randomizebuffer(hw_context,&paquetbuffer[hw_context->usbstats.packetsize],randomtrackptr+hw_context->trackbuffer_pos,1);
				hw_context->trackbuffer_pos=hw_context->trackbuffer_pos+1;
				hw_context->trackbuffer_pos=hw_context->trackbuffer_pos % final_buffer_len;
				bytecopied++;
			}
		}


		hw_context->trackbuffer_pos=hw_context->trackbuffer_pos % final_buffer_len;


		// Derniere etape on change les etat des signaux floppy
		// ready, diskchg, etc...


			/*  index_signal<=          SRAM_DATA(0); -- change index state
				ready_signal<=          SRAM_DATA(1);
				diskchanged_signal<=    SRAM_DATA(2);
				writeprotect_signal<=   SRAM_DATA(3);
				amigareadymode<=        SRAM_DATA(4);
				FLOPPY_SELdisable<=     SRAM_DATA(5);
				FLOPPY_SELreg(0)<=      SRAM_DATA(6);
				FLOPPY_SELreg(1)<=      SRAM_DATA(7);*/

		if(hw_context->drive_select_source>3)
		{
			floppydisable=1;
		}
		else
		{
			floppydisable=0;
		}

		ctrl_byte=((hw_context->drive_select_source&0x3)<<6)|((floppydisable&0x1)<<5) | (amigaready<<4)| (writeprotect<<3) | (floppypin2<<2)|(floppypin34<<1);
		patchbuffer(paquetbuffer, bytecopied,ctrl_byte);
	}

	return bytecopied;
}


int32_t ftdichiplistener(HXCFE* floppycontext,USBHXCFE * hw_context)
{
	void* hw_handle;
	int32_t we_ret;
	int32_t init_failed,ftdierror;
	int32_t i;
	int32_t RxBytes,TxBytes;
	uint8_t * srambuffer;
	uint8_t * srambuffer2;
	uint8_t * input_buffer;
	int32_t byteread,bytetoread;
	int32_t checkalignement;
	int32_t trackpointer;
	int32_t byte_read;
	int32_t txbuffersize;
	uint32_t eventftdi,eventftdimask;
	uint32_t * rxevent;
	int32_t headmoved;
	int32_t bytetowrite;
	int32_t floppypin34;
	int32_t floppypin2;
	int32_t amigaready;
	uint32_t floppydisable;
	uint8_t ctrl_byte,current_track;

	floppycontext->hxc_printf(MSG_DEBUG,"thread ftdichiplistener");
	hw_handle=0;
	headmoved=0;

	srambuffer=(uint8_t*)malloc(SRAMSIZE);
	srambuffer2=(uint8_t*)malloc(SRAMSIZE);
	input_buffer=(uint8_t*)malloc(SRAMSIZE);
	if(!srambuffer || !input_buffer)
	{
		floppycontext->hxc_printf(MSG_ERROR,"srambuffer malloc error !");
		return -1;
	}

	hw_context->running=0;

	hw_context->usbstats.totaldataout=0;
	hw_context->usbstats.dataout=0;
	hw_context->usbstats.synclost=0;
	if(!hw_context->usbstats.packetsize)
		hw_context->usbstats.packetsize = atoi(hxcfe_getEnvVar( floppycontext, "USB_HXCFLOPPYEMULATOR_DEFAULT_PACKET_SIZE", NULL ));
	hw_context->usbstats.totalpacketsent=0;
	hw_context->usbstats.packetsent=0;
	hw_context->usbstats.totaldataout=0;

	rxevent=(uint32_t*)hxc_createevent(floppycontext,1);

	do // boucle principale
	{

		hw_context->status=STATUS_LOOKINGFOR;
		do // boucle autodetection/initialisation de la carte
		{

			if(hw_handle)
			{// driver ftdi ouvert-> on le ferme
				close_ftdichip(hw_handle);
				hw_handle=0;
			}
			init_failed=0;


			do
			{
				// ouverture driver ftdi
				open_ftdichip(&hw_handle);
				if(!hw_handle)
				{// probleme !
				 // il faut temporiser et retenter plus tard
					hxc_pause(500);
				}
				else
				{
					//ok
					//configuration du driver et du chip!
					setusbparameters_ftdichip(hw_handle,2048,2048);

					//4ms de timeout pour l'envoi des data cot� ft245
					setlatencytimer_ftdichip(hw_handle,4);

					purge_ftdichip(hw_handle,FT_PURGE_TX);
					purge_ftdichip(hw_handle,FT_PURGE_RX);

					//utilisation de la methode d'attente sur evenement
					// pour la reception
					eventftdimask= FT_EVENT_RXCHAR;
					seteventnotification_ftdichip(hw_handle,eventftdimask,rxevent);
				}

			}while(!hw_handle);

			// ici l'acces au ft245 est valid�
			// il faut maintenant verifier le cpld.

			// hardware test

			// on efface la SRAM de la carte
			memset(srambuffer,0x00,SRAMSIZE);
			write_ftdichip(hw_handle,srambuffer,SRAMSIZE);

			purge_ftdichip(hw_handle,FT_PURGE_RX);

			//memoire effacee -> la carte ne doit rien envoyer
			if(hxc_waitevent(floppycontext,1,400))
			{// ok on est parti en timeout
				//
				// maintenant on place une commande de synchro dans le buffer
				// et cette fois-ci la carte doit envoyer un octet regulierement

				i=0;
				do
				{
					memset(srambuffer,0x00,SRAMSIZE);
					srambuffer[0]=0xdd;
					srambuffer[1]=0xaa;
					srambuffer[2]=0x33;
					write_ftdichip(hw_handle,srambuffer,SRAMSIZE);

					we_ret=hxc_waitevent(floppycontext,1,1000);

					getfifostatus_ftdichip(hw_handle,&RxBytes,&TxBytes,&eventftdi);

					if(we_ret==1)//si timeout -> pas bon
					{
						init_failed=1;
					}

					i++;
				}
				while(i<8 && !init_failed); //(on fait 8 fois l'operation)

			}
			else
			{// erreur on a re�u des donnees ?! (ancienne version du cpld ?!)
				init_failed=1;
			}

		}while(init_failed);

#ifdef WIN32
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

		// detection terminee
		// on rentre dans la boucle de fonctionnement
		// normal
		hw_context->status=STATUS_ONLINE;
		purge_ftdichip(hw_handle,FT_PURGE_RX);
		checkalignement=5;
		ftdierror=0;
		do
		{

			byteread=0;
			if(checkalignement==0)
			{
				bytetoread=2;
			}
			else
			{
				bytetoread=1;
			}

			do
			{
				// attente de 1 ou 2 octets (bytetoread) de synchro.
				hxc_waitevent(floppycontext,1,2000);
				if (!getfifostatus_ftdichip(hw_handle,&TxBytes,&RxBytes,&eventftdi))
				{
					if((eventftdi&FT_EVENT_RXCHAR) && RxBytes)
					{
						byte_read=read_ftdichip(hw_handle,input_buffer,RxBytes);
						if (byte_read>=0)
						{
							if((bytetoread==1 && RxBytes>1) ||  (bytetoread==2 && RxBytes>2))
							{
								// pas le bon nombre d'octet recu
								// la synchro a ete perdue
								// on reinitialise l'ensemble
								ftdierror=1;
								hw_context->usbstats.synclost++;
							}
							byteread=byteread+RxBytes;
						}
						else
						{
							ftdierror=1;
						}

					}
				}
				else
				{
					ftdierror=1;
				}
			}while(byteread<bytetoread && (ftdierror==0));

			// l'alignement viens juste d'etre verifi�
			// on remet �a dans 5 tours
			if(checkalignement==0)
			{
				checkalignement=5;
			}

			// stop ou encore ?
			if(hw_context->stop_emulation)
			{
				hw_context->running=0;
			}
			else
			{
				if(hw_context->start_emulation)
				{
					hw_context->running=1;
				}
			}

			if(ftdierror==0)
			{
				trackpointer=0;//BytesReceived-1;

				if((input_buffer[trackpointer]&0x80)!=0)
				{
					// la tete a bougee
					headmoved=headmoved|1;
				}

				current_track=input_buffer[trackpointer]&0x7F;

				// stockage track courante
				if(hw_context->double_step)
				{
					hw_context->current_track=current_track>>1;
				}
				else
				{
					hw_context->current_track=current_track;
				}

				if(hw_context->running)
				{//Mode normal : il y a une image a envoyer

					checkalignement--;

					// recherche des prochaines donn�es a envoyer
					txbuffersize=FillBuffer(floppycontext,hw_context,srambuffer,&headmoved);

					// recherche d'une zone sans commande au debut
					// du buffer pour y inserer un commande de
					// synchro
					i=2;
					do
					{
						if(iscmd(srambuffer[i]))
						{
							i=i+2;
						}
					}while((i<txbuffersize) && iscmd(srambuffer[i]));


					// une commande de synchro
					// en debut de buffer
					srambuffer2[0]=0x33;
					srambuffer2[1]=current_track;

					if(!checkalignement)
					{
						// et si il faut verifier la synchro
						// une autre commande de synchro
						// un peu plus loin.
						memcpy(&srambuffer2[2],srambuffer,i+2);
						srambuffer2[i+2]=0x33;
						srambuffer2[i+3]=current_track;
						memcpy(&srambuffer2[i+4],&srambuffer[i],txbuffersize-(i));
						bytetowrite=txbuffersize+(2*2);
					}
					else
					{
						memcpy(&srambuffer2[2],&srambuffer[0],txbuffersize);
						bytetowrite=txbuffersize+2;
					}

					// envois du buffer vers la carte
					if(write_ftdichip(hw_handle,srambuffer2,bytetowrite)<0)
					{
						ftdierror=1;
					}

					// gestion des stats usb
					hw_context->usbstats.packetsent++;
					hw_context->usbstats.totalpacketsent++;
					hw_context->usbstats.totaldataout=hw_context->usbstats.totaldataout+bytetowrite;
					hw_context->usbstats.dataout=hw_context->usbstats.dataout+bytetowrite;

				}
				else
				{
					// mode "standby":
					// pas d'image a envoyer: on fait fonctionner la
					// carte au ralenti
					checkalignement--;
					memset(srambuffer,0,hw_context->usbstats.packetsize);
					srambuffer[0]=0xDD; // set bitrate
					srambuffer[1]=0xF0;
					srambuffer[2]=0x33; // synchro
					srambuffer[3]=current_track;//hw_context->current_track;;
					srambuffer[4]=0xAA;


					// selection des bon etats pour les signaux ready et diskchg
					// selon la machine cible
					floppypin34=0;
					floppypin2=0;
					amigaready=0;

					switch(hw_context->interface_mode)
					{
					case AMIGA_DD_FLOPPYMODE: // amiga
					case AMIGA_HD_FLOPPYMODE: // amiga
						amigaready=1;
						floppypin34=1;

						if(hw_context->floppychanged)
						{
							floppypin2=1;
						}
						else
						{
							floppypin2=0;
						}


						break;

					case IBMPC_DD_FLOPPYMODE: // pc / st 720k
					case ATARIST_DD_FLOPPYMODE:
						amigaready=0;
						floppypin2=0;
						if(hw_context->floppychanged)
						{
							floppypin34=1;

						}
						else
						{
							floppypin34=0;
						}


						break;
					case IBMPC_HD_FLOPPYMODE: // pc / st 1.44MB
					case ATARIST_HD_FLOPPYMODE:
						amigaready=0;
						floppypin2=1;

						if(hw_context->floppychanged)
						{
							floppypin34=1;

						}
						else
						{
							floppypin34=0;
						}
						break;

					case MSX2_DD_FLOPPYMODE:
					case CPC_DD_FLOPPYMODE: // cpc / korg dss1
						amigaready=0;
						floppypin2=0;
						floppypin34=1;
						break;


					case GENERIC_SHUGART_DD_FLOPPYMODE:
					case EMU_SHUGART_FLOPPYMODE:
						amigaready=0;
						floppypin2=0;
						floppypin34=1;
						 break;

					default:
						amigaready=0;
						floppypin2=0;
						floppypin34=1;
						break;
					}

					if(hw_context->drive_select_source>3)
					{
						floppydisable=1;
					}
					else
					{
						floppydisable=0;
					}

					ctrl_byte=((hw_context->drive_select_source&0x3)<<6)|((floppydisable&0x1)<<5) | (amigaready<<4)| (floppypin2<<2)|(floppypin34<<1);


					srambuffer[15]=0xCC;
					srambuffer[16]=ctrl_byte;

					if(!checkalignement)
						srambuffer[8]=0x33; // synchro

					if(write_ftdichip(hw_handle,srambuffer,hw_context->usbstats.packetsize)<0)
					{
						ftdierror=1;
					}
					hw_context->usbstats.packetsent++;
					hw_context->usbstats.totalpacketsent++;
					hw_context->usbstats.totaldataout=hw_context->usbstats.totaldataout+hw_context->usbstats.packetsize;
					hw_context->usbstats.dataout=hw_context->usbstats.dataout+hw_context->usbstats.packetsize;

				}
			}

		}while(!ftdierror);


	}while(1);

	return 0;
}

USBHXCFE* libusbhxcfe_init(HXCFE* floppycontext)
{
	int32_t i,j;
	uint8_t randomvalue;
	USBHXCFE* hwif;

	floppycontext->hxc_printf(MSG_INFO_0,"Starting CPLDFloppyEmulator Hw manager...");
	floppycontext->hxc_printf(MSG_INFO_1,"Loading %s...","ftd2xx.dll");

	if(!(ftdi_load_lib (floppycontext)<0))
	{
		hwif=malloc(sizeof(USBHXCFE));
		memset(hwif,0,sizeof(USBHXCFE));

		hwif->hw_handle=0;

		// generation d'une LUT pour le random MFM
		hwif->randomlut=(uint8_t*)malloc(1024);
		for(i=0;i<1024;i++)
		{
			hwif->randomlut[i]=0;

			for(j=0;j<4;j++)
			{
				do
				{
					randomvalue=(rand()&3);
					if(randomvalue>2) randomvalue=randomvalue^(rand()&3);
				}while(randomvalue>2);
				hwif->randomlut[i]=hwif->randomlut[i] | (randomvalue<<(j*2));
			}
		}
		////////////////////////////////////////////
		hwif->start_emulation=0;
		hwif->stop_emulation=0;
		hwif->drive_select_source=0;

		hxc_createevent(floppycontext,0);
		hxc_createthread(floppycontext,hwif,(THREADFUNCTION)&ftdichiplistener,1);

		floppycontext->hxc_printf(MSG_INFO_0,"Device ok !");

		return hwif;
	}
	else
	{
		return 0;
	}

}


int32_t libusbhxcfe_deInit(HXCFE* floppycontext,USBHXCFE * hwif)
{
	if(hwif)
		free(hwif);
	return 0;
}

int32_t libusbhxcfe_loadFloppy(HXCFE* floppycontext,USBHXCFE * hwif,HXCFE_FLOPPY * floppydisk)
{
#define BUFFERSIZE 2*128*1024
	uint32_t final_buffer_len;
	uint16_t i;
	uint8_t * final_buffer;
	uint8_t * final_randombuffer;

	#ifdef DEBUGVB
	FILE * fdebug;
	char fdebug_name[512];
	#endif

	if(hwif && floppydisk)
	{
		floppycontext->hxc_printf(MSG_INFO_0,"USB HxCFloppyEmulator :Convert track data...");
		if(hwif->running)
		{
			hwif->start_emulation=0;
			hwif->stop_emulation=1;
			do
			{
				hxc_pause(16);
			}while(hwif->running);
			hwif->stop_emulation=0;
		}

		for(i=0;i<hwif->number_of_track;i++)
		{
			if(hwif->precalcusbtrack[i].usbtrack)
				free(hwif->precalcusbtrack[i].usbtrack);

			if(hwif->precalcusbtrack[i].randomusbtrack)
				free(hwif->precalcusbtrack[i].randomusbtrack);

			hwif->precalcusbtrack[i].usbtrack = NULL;
			hwif->precalcusbtrack[i].randomusbtrack = NULL;
		}

		hwif->number_of_track=(uint8_t)floppydisk->floppyNumberOfTrack;
		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			final_buffer = NULL;
			final_randombuffer = NULL;

			if(floppydisk->tracks[i]->number_of_side==2)
			{
			final_buffer_len=GetNewTrackRevolution(floppycontext,
				floppydisk->tracks[i]->sides[0]->indexbuffer,
				floppydisk->tracks[i]->sides[0]->databuffer,
				floppydisk->tracks[i]->sides[0]->tracklen,
				floppydisk->tracks[i]->sides[1]->databuffer,
				floppydisk->tracks[i]->sides[1]->tracklen,
				floppydisk->tracks[i]->sides[0]->flakybitsbuffer,
				floppydisk->tracks[i]->sides[1]->flakybitsbuffer,
				floppydisk->tracks[i]->sides[0]->bitrate,
				floppydisk->tracks[i]->sides[0]->timingbuffer,
				floppydisk->tracks[i]->sides[1]->bitrate,
				floppydisk->tracks[i]->sides[1]->timingbuffer,
				&final_buffer,
				&final_randombuffer,
				0,
				0,
				0,
				0,
				0);
			}
			else
			{
			final_buffer_len=GetNewTrackRevolution(floppycontext,
				floppydisk->tracks[i]->sides[0]->indexbuffer,
				floppydisk->tracks[i]->sides[0]->databuffer,
				floppydisk->tracks[i]->sides[0]->tracklen,
				floppydisk->tracks[i]->sides[0]->databuffer,
				floppydisk->tracks[i]->sides[0]->tracklen,
				floppydisk->tracks[i]->sides[0]->flakybitsbuffer,
				floppydisk->tracks[i]->sides[0]->flakybitsbuffer,
				floppydisk->tracks[i]->sides[0]->bitrate,
				floppydisk->tracks[i]->sides[0]->timingbuffer,
				floppydisk->tracks[i]->sides[0]->bitrate,
				floppydisk->tracks[i]->sides[0]->timingbuffer,
				&final_buffer,
				&final_randombuffer,
				0,
				0,
				0,
				0,
				0);

			}

			if(final_buffer_len>0 && final_randombuffer && final_buffer)
			{
				hwif->precalcusbtrack[i].usbtrack = (uint8_t *)malloc(final_buffer_len);
				if(hwif->precalcusbtrack[i].usbtrack)
					memcpy(hwif->precalcusbtrack[i].usbtrack,final_buffer,final_buffer_len);

				hwif->precalcusbtrack[i].randomusbtrack = (uint8_t *)malloc(final_buffer_len);
				if(hwif->precalcusbtrack[i].randomusbtrack)
					memcpy(hwif->precalcusbtrack[i].randomusbtrack,final_randombuffer,final_buffer_len);

				hwif->precalcusbtrack[i].tracklen=final_buffer_len;

				floppycontext->hxc_printf(MSG_DEBUG,"USB Track %d Size: %d bytes",i,final_buffer_len);
			}

			if(final_randombuffer)
				free(final_randombuffer);

			if(final_buffer)
				free(final_buffer);

			#ifdef DEBUGVB
				if(hwif->precalcusbtrack[i].usbtrack && final_buffer_len)
				{
					sprintf(fdebug_name,"usb_track_%d.bin",i);
					fdebug=(FILE*) fopen(fdebug_name,"w+b");
					if(fdebug)
					{
						fwrite(hwif->precalcusbtrack[i].usbtrack,1,final_buffer_len,fdebug);
						fclose(fdebug);
					}
				}
			#endif
		}


		hwif->interface_mode=(uint8_t)floppydisk->floppyiftype;
		hwif->floppychanged=1;

		floppycontext->hxc_printf(MSG_INFO_0,"Starting emulation...");
		hwif->start_emulation=1;
	}
	return 0;
}

int32_t libusbhxcfe_ejectFloppy(HXCFE* floppycontext,USBHXCFE * hwif)
{
	return 0;
}

int32_t libusbhxcfe_getStats(HXCFE* floppycontext,USBHXCFE * hwif,USBStats* stats,int32_t clear)
{
	if(hwif)
	{
		if(stats)
		{
			memcpy(stats,&hwif->usbstats,sizeof(USBStats));
			hwif->usbstats.dataout=0;
			hwif->usbstats.packetsent=0;
		}

		if(clear)
		{
			hwif->usbstats.dataout=0;
			hwif->usbstats.packetsent=0;
			hwif->usbstats.totaldataout=0;
			hwif->usbstats.totalpacketsent=0;
			hwif->usbstats.synclost=0;
		}
		return hwif->status;
	}
	else
	{
		if(stats)
			memset(stats,0,sizeof(USBStats));

		return 0;
	}

}

int32_t libusbhxcfe_setInterfaceMode(HXCFE* floppycontext,USBHXCFE * hwif,int32_t interfacemode,int32_t doublestep,int32_t drive)
{
	if(hwif)
	{
		hwif->interface_mode = interfacemode;
		hwif->double_step = doublestep;
		hwif->drive_select_source = drive;
	}
	return 0;
}

int32_t libusbhxcfe_getInterfaceMode(HXCFE* floppycontext,USBHXCFE * hwif)
{
	if(hwif)
	{
		return hwif->interface_mode;
	}

	return 0;
}

int32_t libusbhxcfe_getDoubleStep(HXCFE* floppycontext,USBHXCFE * hwif)
{
	if(hwif)
	{
		return hwif->double_step;
	}

	return 0;
}

int32_t libusbhxcfe_getDrive(HXCFE* floppycontext,USBHXCFE * hwif)
{
	if(hwif)
	{
		return hwif->drive_select_source&7;
	}

	return 0;
}

int32_t libusbhxcfe_getCurTrack(HXCFE* floppycontext,USBHXCFE * hwif)
{
	if(hwif)
	{
		return hwif->current_track;
	}
	return 0;
}

int32_t libusbhxcfe_setUSBBufferSize(HXCFE* floppycontext,USBHXCFE * hwif,int32_t size)
{
	if(hwif)
	{
		if(size>=512 && size<=4096)
			hwif->usbstats.packetsize=size;
	}
	return 0;
}
