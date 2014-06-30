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
// File : hxcmod.c
// Contains: a tiny mod player
//
// Written by: Jean François DEL NERO 
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
#include <malloc.h>

#include <string.h>
#include <stdio.h>
#include "hxcmod.h"

#define EFFECT_ARPEGGIO            0x0
#define EFFECT_PORTAMENTO_UP       0x1
#define EFFECT_PORTAMENTO_DOWN     0x2
#define EFFECT_TONE_PORTAMENTO     0x3
#define EFFECT_VIBRATO             0x4
#define EFFECT_VOLSLIDE_TONEPORTA  0x5
#define EFFECT_VOLSLIDE_VIBRATO    0x6
#define EFFECT_VOLSLIDE_TREMOLO    0x7
#define EFFECT_SET_PANNING         0x8
#define EFFECT_SET_OFFSET          0x9
#define EFFECT_VOLUME_SLIDE        0xA
#define EFFECT_JUMP_POSITION       0xB
#define EFFECT_SET_VOLUME          0xC
#define EFFECT_PATTERN_BREAK       0xD

#define EFFECT_EXTENDED		       0xE
#define EFFECT_E_FINE_PORTA_UP       0x1
#define EFFECT_E_FINE_PORTA_DOWN     0x2
#define EFFECT_E_GLISSANDO_CTRL      0x3
#define EFFECT_E_VIBRATO_WAVEFORM    0x4
#define EFFECT_E_SET_FINETUNE        0x5
#define EFFECT_E_PATTERN_LOOP        0x6
#define EFFECT_E_TREMOLO_WAVEFORM    0x7
#define EFFECT_E_SET_PANNING_2       0x8
#define EFFECT_E_RETRIGGER_NOTE      0x9
#define EFFECT_E_FINE_VOLSLIDE_UP    0xA
#define EFFECT_E_FINE_VOLSLIDE_DOWN  0xB
#define EFFECT_E_NOTE_CUT            0xC
#define EFFECT_E_NOTE_DELAY          0xD
#define EFFECT_E_PATTERN_DELAY       0xE
#define EFFECT_E_INVERT_LOOP         0xF
#define EFFECT_SET_SPEED           0xF0
#define EFFECT_SET_TEMPO           0xF2

void memcopy(void * dest,void *source,unsigned long size)
{
	unsigned long i;
	unsigned char * d,*s;
	d=(unsigned char*)dest;
	s=(unsigned char*)source;
	for(i=0;i<size;i++)
	{
		d[i]=s[i];
	}
}

void modregevent(modevent *mevent,modcontext * mod)
{
	mod->modev=mevent;
}

modcontext * modload(char * filename,char * modmemory,char * debug)
{
	muint	i, max;
	unsigned short t;
	sample	*sptr;
	FILE	*fp;
	modcontext * mod;

	mod=(modcontext *)malloc(sizeof(modcontext));
	if(mod)
	{
		memset(mod,0,sizeof(modcontext));

		mod->number_of_channels = 4;

		if(filename!=NULL)
		{
			if ((fp = fopen(filename, "rb")) == NULL) 
				return 0;

			if (fread(&(mod->song.title), 1, 1084, fp) != 1084) return 0;

			// traitement des mods 15 samples
			if (memcmp(&(mod->song.signature), "M.K.", 4) != 0 && memcmp(&(mod->song.signature), "FLT4", 4) != 0 &&  memcmp(&(mod->song.signature), "FLT8", 4) != 0) 
			{
				// decalage...
				memcopy(&(mod->song.signature), "M.K.", 4);
				memcopy(&(mod->song.length), &(mod->song.samples[15]), 130);
				memset(&(mod->song.samples[15]), 0, 480);
				if (fseek(fp, 600L, SEEK_SET) != 0) return 0;
			}
	
			//Chargement des patterns
			for (i = max = 0; i < 128; i++)
			{
				while (max <= mod->song.patterntable[i]) 
				{
					if ((mod->patterndata[max]=(note *)malloc(1024))==NULL) 
						return 0;
					if (fread(mod->patterndata[max++],1024,1,fp)!=1) 
						return 0;
				}
			}
			
			for (i = 0; i < 31; i++) 	mod->sampledata[i]=NULL;

			//chargement des samples
			for (i = 0, sptr = mod->song.samples; i <31; i++, sptr++) 
			{
				t= (sptr->length &0xFF00)>>8 | (sptr->length &0xFF)<<8;
				sptr->length=t*2;

				t= (sptr->reppnt &0xFF00)>>8 | (sptr->reppnt &0xFF)<<8;
				sptr->reppnt=t*2;

				t= (sptr->replen &0xFF00)>>8 | (sptr->replen &0xFF)<<8;
				sptr->replen=t*2;

	
				if (sptr->length == 0) continue;
			
				if ((mod->sampledata[i] =(char*)malloc(sptr->length)) == NULL) return 0;
			
				fread(mod->sampledata[i], sptr->length, 1, fp);
			
				if (sptr->replen + sptr->reppnt > sptr->length)	sptr->replen = sptr->length - sptr->reppnt;

		//for (max = 0; max < sptr->length; max++) sampledata[i][max] ^= 0x80;
			}
		}
		else
		{

			memcopy(&(mod->song.title),modmemory,1084);
	
			// traitement des mods 15 samples
			/*if (memcmp(&(song.signature), "M.K.", 4) != 0 && memcmp(&(song.signature), "FLT4", 4) != 0 &&  memcmp(&(song.signature), "FLT8", 4) != 0) 
			{
				// decalage...
				memcopy(&(song.signature), "M.K.", 4);
				memcopy(&(song.length), &(song.samples[15]), 130);
				memset(&(song.samples[15]), 0, 480);
				modmemory=modmemory+600;
			}
			else*/ 

			modmemory=modmemory+1084;
	
			//Chargement des patterns
			for (i = max = 0; i < 128; i++)
			{
				while (max <= mod->song.patterntable[i]) 
				{
					mod->patterndata[max] = (note*)malloc((256*mod->number_of_channels));
					if(mod->patterndata[max])
					{
						memcopy(mod->patterndata[max],modmemory,(256*mod->number_of_channels));
						modmemory=modmemory+(256*mod->number_of_channels);
						max++;
					}
					else
						return 0;
					//if (fread(patterndata[max++],1024,1,fp)!=1) return 0;
				}
			}

			for (i = 0; i < 31; i++)
				mod->sampledata[i]=0;

			//chargement des samples
			for (i = 0, sptr = mod->song.samples; i <31; i++, sptr++) 
			{
				t= (sptr->length &0xFF00)>>8 | (sptr->length &0xFF)<<8;
				sptr->length=t*2;

				t= (sptr->reppnt &0xFF00)>>8 | (sptr->reppnt &0xFF)<<8;
				sptr->reppnt=t*2;

				t= (sptr->replen &0xFF00)>>8 | (sptr->replen &0xFF)<<8;
				sptr->replen=t*2;

			
				if (sptr->length == 0) continue;
			
				if ((mod->sampledata[i] =(char*)malloc(sptr->length)) == 0) return 0;
			
				memcopy(mod->sampledata[i],modmemory,sptr->length);
				modmemory=modmemory+sptr->length;
					
			//	fread(sampledata[i], sptr->length, 1, fp);
			
				if (sptr->replen + sptr->reppnt > sptr->length)	sptr->replen = sptr->length - sptr->reppnt;

				//for (max = 0; max < sptr->length; max++) sampledata[i][max] ^= 0x80;
			}

		}

	}

	return mod;
}

int modplay(modcontext * mod)
{
	int i;
	
	if(mod)
	{
		mod->playrate = 44100;
		mod->tablepos = 0;
		mod->patternpos = 0;
		mod->song.speed = 6;
		mod->bpm = 125;
		mod->samplenb = 0;
		
		mod->patternticks = (((long)mod->song.speed * mod->playrate * 5)/ (2 * mod->bpm)) + 1;
		mod->patternticksaim = ((long)mod->song.speed * mod->playrate * 5) / (2 * mod->bpm);
		
		
		mod->sampleticksconst = 3546894UL / mod->playrate;
		//8448*428/playrate;

		for(i=0;i<(mod->number_of_channels);i++)
		{
			mod->channels[i].volume = 0; 
			mod->channels[i].period = 448;
			mod->channels[i].decalperiod2=0;
			mod->channels[i].oldoffsetconfig=0;
		}
		return 1;
	}

	return 0;
}

int getnote(unsigned short period,int finetune,short * perdiodtable)
{
	short * fineperiodtable;
	int i;

	fineperiodtable = &perdiodtable[(finetune&0xF) * MAXNOTES];

    for(i = 0; i < MAXNOTES; i++)
    {
        if(period >= fineperiodtable[i])
        {
            return i;
        }
    }

	return MAXNOTES;
}

void fillbuffer(muchar * buffer, unsigned long nbsample,modcontext * mod)
{
	unsigned long i, j,e;
	unsigned long k;
	unsigned char c;
	short l,r;
	short ll,lr;
	short tl,tr;
	
	note	*nptr;
	channel *cptr;
	
	if(mod && buffer)
	{
		ll=0;
		lr=0;
		
		for (i = 0; i < nbsample; i++) 
		{
			//---------------------------------------
			if (mod->patternticks++ > mod->patternticksaim) 
			{
				nptr = mod->patterndata[mod->song.patterntable[mod->tablepos]];
				nptr = nptr + mod->patternpos;
				cptr = mod->channels;

				mod->patternpos += mod->number_of_channels;
				mod->patternticks = 0;
				mod->patterntickse=0;

				for(c=0;c<mod->number_of_channels;c++)
				{
					worknote((note*)(nptr+c), (channel*)(cptr+c),(char)(c+1),mod);
				}

				if (mod->patternpos == 64*mod->number_of_channels) 
				{
					mod->tablepos++;
					mod->patternpos = 0;
					if(mod->tablepos >= mod->song.length) mod->tablepos = 0;
				}

				if(mod->modev)
				{
					e=0;
					do
					{
						if(mod->modev[e].pattern==mod->tablepos && mod->modev[e].order==(unsigned short)(mod->patternpos/mod->number_of_channels))
						{
							mod->modev[e].bufferpos=mod->samplenb+i;
						}

					e++;
					}while(mod->modev[e].pattern!=0xffffffff);
				}

			}

			if (mod->patterntickse++ > (mod->patternticksaim/mod->song.speed)) 
			{
				
				nptr = mod->patterndata[mod->song.patterntable[mod->tablepos]];
				nptr = nptr + mod->patternpos;
				cptr = mod->channels;

				for(c=0;c<mod->number_of_channels;c++)
				{
					workeffect(nptr+c, cptr+c);	
				}
				
				mod->patterntickse=0;
			}
			//---------------------------------------

			l=0;
			r=0;
			
			for (j =0, cptr = mod->channels; (j < mod->number_of_channels) && (cptr->period!=0); j++, cptr++) 
			{
				k=0;
				if(((long)cptr->period - (long)cptr->decalperiod2 - (long)cptr->decalperiod - (long)cptr->vibraperiod )!=0)
				k = ((float)((mod->sampleticksconst * cptr->ticks) / (float)((long)cptr->period - (long)cptr->decalperiod2 - (long)cptr->decalperiod - (long)cptr->vibraperiod )))/*-cptr->oldk*/;

				cptr->ticks++;
				
				//if(j==1)
				{
					if(cptr->replen<=2)
					{
						if (k >= (cptr->length)) 
						{
							cptr->length = 0;
							cptr->reppnt = 0;
							cptr->ticks = 0;
							k=0;
						}
					}
					else
					{
						if (k >= (unsigned long)(cptr->replen+cptr->reppnt)) 
						{
							k=cptr->reppnt;
							cptr->ticks = (float)(((long)cptr->period - (long)cptr->decalperiod2 - (long)cptr->decalperiod - (long)cptr->vibraperiod )*k)/(float)(mod->sampleticksconst);
							//cptr->ticks = 0;
							//k=0;
						}
					}

					//if(j==0)
					{
					if(cptr->sampdata!=0 && (((j&3)==1) || ((j&3)==2) ))
						l += ( (cptr->sampdata[k] *  cptr->volume));
					
					if(cptr->sampdata!=0 && (((j&3)==0) || ((j&3)==3) ))
						r += ( (cptr->sampdata[k] *  cptr->volume));
					}

				}
			}
			

			tr=r;
			tl=l;
			
			l=(l+ll)>>1;
			r=(r+lr)>>1;
			
			buffer[(i*4)+1] = (l+(r>>1))>>8;
			buffer[(i*4)]   = (l+(r>>1))&0xff;

			buffer[(i*4)+3] = (r+(l>>1)) >>8;
			buffer[(i*4)+2] = (r+(l>>1))&0xff;

			ll=tl;
			lr=tr;
		}

		mod->samplenb=mod->samplenb+nbsample;
	}
}

void worknote(note * nptr, channel * cptr,char t,modcontext * mod)
{
	muint sample, period, effect,operiod;
	muint count2,curnote,arpnote;

	sample = (nptr->sampperiod & 0xF0) | (nptr->sampeffect >> 4);
	period = ((nptr->sampperiod & 0xF) << 8) | nptr->period;
	effect = ((nptr->sampeffect & 0xF) << 8) | nptr->effect;
	
	operiod=cptr->period;
	
	if (period != 0 || sample != 0) 
	{
		if (sample != 0 && sample<32) 
		{
			cptr->sampnum = sample-1;
		}
		
		if (period != 0 || sample != 0)
		{
			cptr->sampdata =(char *) mod->sampledata[cptr->sampnum];
			cptr->length = mod->song.samples[cptr->sampnum].length;
			cptr->reppnt = mod->song.samples[cptr->sampnum].reppnt;
			cptr->replen = mod->song.samples[cptr->sampnum].replen;

			cptr->finetune = mod->song.samples[cptr->sampnum].finetune;
			
			if(effect>>8!=4 && effect>>8!=6)
			{
				cptr->vibraperiod=0;
				cptr->vibrapointeur=0;
			}

		}

		if((sample != 0) && (effect>>8) != EFFECT_VOLSLIDE_TONEPORTA)
		{
			cptr->volume = mod->song.samples[cptr->sampnum].volume;
		}
		
		if(( (effect>>8) != EFFECT_TONE_PORTAMENTO && (effect>>8)!=EFFECT_VOLSLIDE_TONEPORTA) /*&& (period != 0 || sample != 0)*/) 
		{
			cptr->decalperiod2=0;
			if (period!=0)
				cptr->ticks = 0;	
		}
		
		cptr->decalperiod=0;
		if(period!=0)
		{
			cptr->period = period;
			/*for (count2=0;count2<36; count2++) 
			{    
				if (period >= (periodtable[count2-1]-1) && period <= (periodtable[count2-1]+1) )
					cptr->period = count2 + ((mod->song.samples[cptr->sampnum].finetune)*36);  // if found store the counter as

			} */
		}

	}
	
	cptr->effect = 0;
	cptr->parameffect = 0;

	switch (effect >> 8) 
	{
		case EFFECT_ARPEGGIO:
			if(effect&0xff)
			{
				cptr->effect = EFFECT_ARPEGGIO;
				cptr->parameffect = effect&0xff;

				cptr->ArpIndex = 0;

				curnote = getnote(cptr->period,cptr->finetune,&periodtable);

				cptr->Arpperiods[0] = cptr->period;

				arpnote = curnote + (((cptr->parameffect>>4)&0xF));
				if(arpnote>=MAXNOTES) arpnote = MAXNOTES - 1;
				cptr->Arpperiods[1] = periodtable[((cptr->finetune&0xF)*MAXNOTES) + arpnote ];

				arpnote = curnote + (((cptr->parameffect)&0xF));
				if(arpnote>=MAXNOTES) arpnote = MAXNOTES - 1;
				cptr->Arpperiods[2] = periodtable[((cptr->finetune&0xF)*MAXNOTES) + arpnote];
			}
		break;

		case EFFECT_PORTAMENTO_UP:
			cptr->effect = EFFECT_PORTAMENTO_UP;
			cptr->parameffect = effect&0xff;
		break;

		case EFFECT_PORTAMENTO_DOWN:
			cptr->effect = EFFECT_PORTAMENTO_DOWN;
			cptr->parameffect=effect&0xff;
		break;

		case EFFECT_TONE_PORTAMENTO:
			cptr->effect = EFFECT_TONE_PORTAMENTO;
			if((effect&0xff)!=0)
			{
				cptr->portaspeed=(short)(effect&0xff);
			}

			if(period!=0)
			{
				cptr->portaperiod=period;
				cptr->period=operiod;
			}
		break;

		case EFFECT_VIBRATO:

			cptr->effect = EFFECT_VIBRATO;
			if( ( effect & 0xff ) != 0 )
				cptr->vibraparam = effect & 0xff;

		break;

		case EFFECT_VOLSLIDE_TONEPORTA:

			cptr->effect=EFFECT_VOLSLIDE_TONEPORTA;
			if( ( effect & 0xFF ) != 0 )	
				cptr->volumeslide = ( effect & 0xFF );

		break;

		case EFFECT_VOLSLIDE_VIBRATO:

			cptr->effect=EFFECT_VOLSLIDE_VIBRATO;
			if( (effect & 0xFF) != 0 )
				cptr->volumeslide = (effect & 0xFF);

		break;

		case EFFECT_SET_OFFSET:
			//k = ((sampleticksconst * cptr->ticks) / (periodtable[cptr->period-1] - cptr->decalperiod2 - cptr->decalperiod ))/*-cptr->oldk*/;
			if((effect & 0xFF)!=0)
			{
				cptr->ticks=((effect & 0xFF) * 256)*((cptr->period - cptr->decalperiod2 - cptr->decalperiod )/mod->sampleticksconst);
				cptr->oldoffsetconfig=(effect & 0xFF);
			}
			else
			{
				cptr->ticks=((cptr->oldoffsetconfig) * 256)*((cptr->period - cptr->decalperiod2 - cptr->decalperiod )/mod->sampleticksconst);

			}

			//cptr->sampdata=cptr->sampdata  + ((effect & 0xFF) * 256); 
		break;

		case EFFECT_VOLUME_SLIDE:
			cptr->effect = EFFECT_VOLUME_SLIDE;
			/*if((effect & 0xFF)!=0)*/	
			cptr->volumeslide = (effect & 0xFF);
		break;

		case EFFECT_JUMP_POSITION: 
			mod->tablepos = (effect & 0xFF);
		break;

		case EFFECT_SET_VOLUME: 
			cptr->volume = (effect & 0xFF);
		break;

		case EFFECT_PATTERN_BREAK: 
			mod->patternpos = ( ((effect>>4)&0xF)*10 + (effect&0xF) ) * mod->number_of_channels;
			mod->tablepos++;
		break;

		case EFFECT_EXTENDED: 
			switch((effect>>4)&0xF)
			{
				case EFFECT_E_FINE_PORTA_UP:
					t=cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod;
			
					cptr->decalperiod=cptr->decalperiod + (effect & 0xF);
			
					cptr->ticks= ( ( (cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod) *cptr->ticks)/t);
				break;

				case EFFECT_E_FINE_PORTA_DOWN:
					t=cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod;
			
					cptr->decalperiod=cptr->decalperiod + (effect & 0xF);
			
					cptr->ticks= ( ( (cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod) *cptr->ticks)/t);
				break;


				case EFFECT_E_FINE_VOLSLIDE_UP:
					cptr->volume += (effect & 0xF); 
					if( cptr->volume>64 )
						cptr->volume = 64;
				break;

				case EFFECT_E_FINE_VOLSLIDE_DOWN:
					cptr->volume -= (effect & 0xF); 
					if( cptr->volume > 200 )
						cptr->volume = 0;
				break;
				default:

				break;
			}
		break;

		case 0xF: 
			if((effect&0xFF)<0x21)
			{
				mod->song.speed=effect&0xFF;
				mod->patternticksaim = (long)mod->song.speed * ((mod->playrate * 5 ) / (((long)2 * (long)mod->bpm)));
			}

			if((effect&0xFF)>=0x21)
			{
				///	 HZ = 2 * BPM / 5
				mod->bpm=effect&0xFF;
				mod->patternticksaim = (long)mod->song.speed * ((mod->playrate * 5 ) / (((long)2 * (long)mod->bpm)));
			}

		break;

		default:
		// Unsupported effect
		break;
	
	}
	
	
}


void workeffect(note * nptr, channel * cptr)
{
	short t;

	switch(cptr->effect)
	{
		case EFFECT_ARPEGGIO:
			if(cptr->parameffect)
			{
				cptr->decalperiod = cptr->period - cptr->Arpperiods[cptr->ArpIndex];

				cptr->ArpIndex++;
				if(cptr->ArpIndex>2)
					cptr->ArpIndex = 0;
			}
		break;
		case EFFECT_PORTAMENTO_UP:

			cptr->period -= (((cptr->parameffect>>4)&0x0F)*16) + (cptr->parameffect&0x0F);

			if(cptr->period < 113 || cptr->period > 856)
				cptr->period = 113;

		break;

		case EFFECT_PORTAMENTO_DOWN:

			cptr->period += (((cptr->parameffect>>4)&0x0F)*16) + (cptr->parameffect&0x0F);

			if(cptr->period > 856)
				cptr->period = 856;

		break;
	
		case EFFECT_VOLSLIDE_TONEPORTA:
		case EFFECT_TONE_PORTAMENTO:

			t=cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod;

			if((cptr->period-cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod)<cptr->portaperiod)
			{		
				cptr->decalperiod2=cptr->decalperiod2 - ( (((cptr->portaspeed>>4)&0xF)*16) + (cptr->portaspeed & 0xF) );
				
				if((cptr->period-cptr->decalperiod2- cptr->decalperiod - cptr->vibraperiod)>cptr->portaperiod)
				{
					cptr->decalperiod2 = cptr->period - cptr->decalperiod - cptr->portaperiod - cptr->vibraperiod;
				}
					
			}

			if((cptr->period-cptr->decalperiod2- cptr->decalperiod- cptr->vibraperiod)>cptr->portaperiod)
			{
				cptr->decalperiod2=cptr->decalperiod2 + ( (((cptr->portaspeed>>4)&0xF)*16) + (cptr->portaspeed & 0xF) );;
				if((cptr->period-cptr->decalperiod2- cptr->decalperiod - cptr->vibraperiod)<cptr->portaperiod)
				{
						cptr->decalperiod2=(cptr->period - cptr->decalperiod - cptr->vibraperiod) - cptr->portaperiod;
				}
				
			}
		
			cptr->ticks= ( ( (cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod) *cptr->ticks)/t);

			if(cptr->effect==EFFECT_VOLSLIDE_TONEPORTA)
			{
				if(cptr->volumeslide>0x0F)
				{
					cptr->volume = cptr->volume + (cptr->volumeslide>>4);

					if(cptr->volume>63)
						cptr->volume = 63;
			
				}
				else
				{
					cptr->volume = cptr->volume - (cptr->volumeslide);

					if(cptr->volume>63)
						cptr->volume=0;
				}

			}
		break;
		
		case EFFECT_VOLSLIDE_VIBRATO:
		case EFFECT_VIBRATO:

			t = cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod;

			cptr->vibraperiod=((cptr->vibraparam&0xF)*sintable[cptr->vibrapointeur&0x1F])>>7;
		
			if(cptr->vibrapointeur>31) cptr->vibraperiod=-cptr->vibraperiod;

			cptr->vibrapointeur=(cptr->vibrapointeur+((cptr->vibraparam>>4)&0xf))&0x1F;
			
			cptr->ticks= ( ( ( cptr->period - cptr->decalperiod2 - cptr->decalperiod - cptr->vibraperiod) *cptr->ticks)/t);

			if(cptr->effect == EFFECT_VOLSLIDE_VIBRATO)
			{
				if(cptr->volumeslide>0xF)
				{
					cptr->volume=cptr->volume+(cptr->volumeslide>>4);

					if(cptr->volume>64)cptr->volume=64;				
				}
				else
				{
					cptr->volume=cptr->volume-(cptr->volumeslide);

					if(cptr->volume>63)cptr->volume=0;
				}
			}
			
		break;
		
		case EFFECT_VOLUME_SLIDE:

			if(cptr->volumeslide>0xF)
			{
				cptr->volume += (cptr->volumeslide>>4);

				if( cptr->volume > 64 )
					cptr->volume = 64;
			}
			else
			{
				cptr->volume -= (cptr->volumeslide&0xf);

				if( cptr->volume > 64 )
					cptr->volume = 0;
			}
		break;
		
		default:
		break;

	}
	
}


