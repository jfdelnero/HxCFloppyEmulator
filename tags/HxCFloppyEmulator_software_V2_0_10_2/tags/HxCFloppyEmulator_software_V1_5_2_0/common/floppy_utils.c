#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"

unsigned long us2index(unsigned long startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder)
{
	uint32_t time,freq;

	if(!fillorder)
	{
		if(track->bitrate==VARIABLEBITRATE)
		{
			time=0;
			do
			{

				if(fill)track->indexbuffer[startindex>>3]=0xFF;
				freq=track->timingbuffer[startindex>>3];
				startindex++;
				if(startindex>=track->tracklen) startindex=0;

				time=time+((1000000000/freq)*1);
			}while(us>(time/1000));
			return startindex;
		}
		else
		{
			freq=track->bitrate;
			time=0;
			do
			{
				if(fill)track->indexbuffer[startindex>>3]=0xFF;
				startindex++;
				if(startindex>=track->tracklen) startindex=0;
				time=time+((1000000000/freq)*1);
			}while(us>(time/1000));
			return startindex;
		}
	}
	else
	{
		if(track->bitrate==VARIABLEBITRATE)
		{
			time=0;
			do
			{

				if(fill)track->indexbuffer[startindex>>3]=0xFF;
				freq=track->timingbuffer[startindex>>3];

				if(startindex)
					startindex--;
				else
					startindex=track->tracklen-1;
				
				time=time+((1000000000/freq)*1);
			}while(us>(time/1000));
			return startindex;
		}
		else
		{
			freq=track->bitrate;
			time=0;
			do
			{
				if(fill)track->indexbuffer[startindex>>3]=0xFF;

				if(startindex)
					startindex--;
				else
					startindex=track->tracklen-1;

				time=time+((1000000000/freq)*1);
			}while(us>(time/1000));
			return startindex;
		}
	}
};

unsigned long fillindex(unsigned long startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder)
{
	
	return us2index(startindex,track,us,fill,fillorder);
}

