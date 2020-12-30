/*
//
// Copyright (C) 2006-2021 Jean-François DEL NERO
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
// File : script_exec.c
// Contains: script execution functions
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "libhxcfe.h"
#include "floppy_loader.h"

#include "libhxcadaptor.h"

#include "version.h"

#include "env.h"

#define MAX_CFG_STRING_SIZE 1024

int hxcfe_setEnvVar( HXCFE* hxcfe, char * varname, char * varvalue )
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)hxcfe->envvar;

	if(!tmp_envvars)
	{
		tmp_envvars = malloc(sizeof(envvar_entry) );
		if(!tmp_envvars)
			return -1;

		memset( tmp_envvars,0,sizeof(envvar_entry));

		hxcfe->envvar = (void*)tmp_envvars;
	}

	// is the variable already there
	while(tmp_envvars[i].name)
	{
		if(!strcmp(tmp_envvars[i].name,varname) )
		{
			break;
		}
		i++;
	}

	if(tmp_envvars[i].name)
	{
		// the variable already exist - update it.
		if(tmp_envvars[i].varvalue)
		{
			free(tmp_envvars[i].varvalue);
			tmp_envvars[i].varvalue = NULL;
		}

		if(varvalue)
		{
			tmp_envvars[i].varvalue = malloc(strlen(varvalue)+1);

			if(!tmp_envvars[i].varvalue)
				return -1;

			memset(tmp_envvars[i].varvalue,0,strlen(varvalue)+1);
			if(varvalue)
				strcpy(tmp_envvars[i].varvalue,varvalue);
		}
	}
	else
	{
		// No variable found, alloc an new entry
		if(strlen(varname))
		{
			tmp_envvars[i].name = malloc(strlen(varname)+1);
			if(!tmp_envvars[i].name)
				return -1;

			memset(tmp_envvars[i].name,0,strlen(varname)+1);
			strcpy(tmp_envvars[i].name,varname);

			if(varvalue)
			{
				tmp_envvars[i].varvalue = malloc(strlen(varvalue)+1);

				if(!tmp_envvars[i].varvalue)
					return -1;

				memset(tmp_envvars[i].varvalue,0,strlen(varvalue)+1);
				if(varvalue)
					strcpy(tmp_envvars[i].varvalue,varvalue);
			}

			tmp_envvars = realloc(tmp_envvars,sizeof(envvar_entry) * (i + 1 + 1));
			memset(&tmp_envvars[i + 1],0,sizeof(envvar_entry));
		}
	}

	hxcfe->envvar = (void*)tmp_envvars;

	return 1;
}

char * hxcfe_getEnvVar( HXCFE* hxcfe, char * varname, char * varvalue)
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)hxcfe->envvar;
	if(!tmp_envvars)
		return NULL;

	// search the variable...
	while(tmp_envvars[i].name)
	{
		if(!strcmp(tmp_envvars[i].name,varname) )
		{
			break;
		}
		i++;
	}

	if(tmp_envvars[i].name)
	{
		if(varvalue)
			strcpy(varvalue,tmp_envvars[i].varvalue);

		return tmp_envvars[i].varvalue;
	}
	else
	{
		return NULL;
	}
}

int hxcfe_getEnvVarValue( HXCFE* hxcfe, char * varname)
{
	int value;
	char * str_return;

	value = 0;

	if(!varname)
		return 0;

	str_return = hxcfe_getEnvVar( hxcfe, varname, NULL);

	if(str_return)
	{
		if( strlen(str_return) > 2 )
		{
			if( str_return[0]=='0' && ( str_return[0]=='x' || str_return[0]=='X'))
			{
				value = (int)strtol(str_return, NULL, 0);
			}
			else
			{
				value = atoi(str_return);
			}
		}
		else
		{
			value = atoi(str_return);
		}
	}

	return value;
}

char * hxcfe_getEnvVarIndex( HXCFE* hxcfe, int index, char * varvalue)
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)hxcfe->envvar;
	if(!tmp_envvars)
		return NULL;

	// search the variable...
	while(tmp_envvars[i].name && i < index)
	{
		i++;
	}

	if(tmp_envvars[i].name)
	{
		if(varvalue)
			strcpy(varvalue,tmp_envvars[i].varvalue);

		return tmp_envvars[i].name;
	}
	else
	{
		return NULL;
	}
}

envvar_entry * duplicate_env_vars(envvar_entry * src)
{
	int i,j;
	envvar_entry * tmp_envvars;

	if(!src)
		return NULL;

	i = 0;
	// count entry
	while(src[i].name)
	{
		i++;
	}

	tmp_envvars = malloc(sizeof(envvar_entry) * (i + 1));
	if(tmp_envvars)
	{
		memset(tmp_envvars,0,sizeof(envvar_entry) * (i + 1));
		for(j=0;j<i;j++)
		{
			if(src[j].name)
			{
				tmp_envvars[j].name = malloc(strlen(src[j].name) + 1);
				strcpy(tmp_envvars[j].name,src[j].name);
			}

			if(src[j].varvalue)
			{
				tmp_envvars[j].varvalue = malloc(strlen(src[j].varvalue) + 1);
				strcpy(tmp_envvars[j].varvalue,src[j].varvalue);
			}
		}
	}

	return tmp_envvars;
}

void free_env_vars(envvar_entry * src)
{
	int i,j;

	if(!src)
		return;

	i = 0;
	// count entry
	while(src[i].name)
	{
		i++;
	}

	for(j=0;j<i;j++)
	{
		if(src[j].name)
		{
			free(src[j].name);
		}

		if(src[j].varvalue)
		{
			free(src[j].varvalue);
		}
	}

	free(src);

	return;
}