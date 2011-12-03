// test_hxcfelib.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "hxc_floppy_emulator.h"


int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	if(1)//MSGTYPE!=MSG_DEBUG)
	{
		va_list marker;
		va_start( marker, chaine );     
		
		vprintf(chaine,marker);

		printf("\n");
				
		va_end( marker ); 
	}
    return 0;
}

#ifdef __cplusplus
}
#endif

int main(int argc, char* argv[])
{
	char id[16];
	char desc[256];
	int i;
	FLOPPY* thefloppydisk;
	HXCFLOPPYEMULATOR * flopemu;
	
	printf("HxCFE Lib test\n");
	
	flopemu=hxcfe_init();

	hxcfe_set_outputfunc(flopemu,CUI_affiche);

	i=0;
	while(hxcfe_getcontainerid(flopemu,i,(char*)id,(char*)desc)==HXCFE_NOERROR)
	{
		//printf("%s \t\t %s\n",id,desc);
		printf("#define\tPLUGIN_%s\t\t\t\"%s\"\n",id,id);
		i++;
	}

	hxcfe_select_container(flopemu,"ATARIST_STX");


	do{
		hxcfe_select_container(flopemu,"ATARIST_STX");
		thefloppydisk=hxcfe_floppy_load(flopemu,"Vroom Data Disc(1992)(Lankhor).STX",0);
		
		hxcfe_select_container(flopemu,"HXCMFM_IMG");
		hxcfe_floppy_export(flopemu,thefloppydisk,"t.img");
		
		hxcfe_select_container(flopemu,"HXC_HFE");
		hxcfe_floppy_export(flopemu,thefloppydisk,"t2.img");
		
		hxcfe_floppy_unload(flopemu,thefloppydisk);
	}while(1);

	hxcfe_deinit(flopemu);
	return 0;
}

