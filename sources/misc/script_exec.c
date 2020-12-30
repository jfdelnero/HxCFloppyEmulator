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

#include "env.h"

#include "version.h"

#define MAX_CFG_STRING_SIZE 1024

typedef int (* KW_FUNC)(HXCFE * context, char * line, int cmd);

enum
{
	VERSION_CMD = 0,
	SET_ENV_CMD,
	PRINT_ENV_VAR_CMD
};

typedef struct kw_list_
{
	char * keyword;
	KW_FUNC func;
	int cmd;
}kw_list;

static int is_end_line(char c)
{
	if( c == 0 || c == '#' || c == '\r' || c == '\n' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_space(char c)
{
	if( c == ' ' || c == '\t' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int get_next_word(char * line, int offset)
{
	while( !is_end_line(line[offset]) && ( line[offset] == ' ' ) )
	{
		offset++;
	}

	return offset;
}

static int copy_param(char * dest, char * line, int offs)
{
	int i,insidequote;

	i = 0;
	insidequote = 0;
	while( !is_end_line(line[offs]) && ( insidequote || !is_space(line[offs]) ) && (i < (MAX_CFG_STRING_SIZE - 1)) )
	{
		if(line[offs] != '"')
		{
			if(dest)
				dest[i] = line[offs];

			i++;
		}
		else
		{
			if(insidequote)
				insidequote = 0;
			else
				insidequote = 1;
		}

		offs++;
	}

	if(dest)
		dest[i] = 0;

	return offs;
}

static int get_param_offset(char * line, int param)
{
	int param_cnt, offs;

	offs = 0;
	offs = get_next_word(line, offs);

	param_cnt = 0;
	do
	{
		offs = copy_param(NULL, line, offs);

		offs = get_next_word( line, offs );

		if(line[offs] == 0 || line[offs] == '#')
			return -1;

		param_cnt++;
	}while( param_cnt < param );

	return offs;
}

static int get_param(char * line, int param_offset,char * param)
{
	int offs;

	offs = get_param_offset(line, param_offset);

	if(offs>=0)
	{
		offs = copy_param(param, line, offs);

		return 1;
	}

	return -1;
}

static int extract_cmd(char * line, char * command)
{
	int offs,i;

	i = 0;
	offs = 0;

	offs = get_next_word(line, offs);

	if( !is_end_line(line[offs]) )
	{
		while( !is_end_line(line[offs]) && !is_space(line[offs]) && i < (MAX_CFG_STRING_SIZE - 1) )
		{
			command[i] = line[offs];
			offs++;
			i++;
		}

		command[i] = 0;

		return i;
	}

	return 0;
}

/*
static int get_hex_param(HXCFE * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];
	unsigned long param_value;

	i = get_param(line, 1,tmp_txt);

	if( i >= 0 )
	{
		param_value = strtol(tmp_txt,0,16);
		switch(cmd)
		{

		}
	}

	return 0;
}
*/

static int print_version_cmd(HXCFE * context, char * line,int cmd)
{
	context->hxc_printf(MSG_INFO_1,"v"STR_FILE_VERSION2);

	return 0;
}

/*
static int get_str_param(HXCFE * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];

	i = get_param(line, 1,tmp_txt);

	if( i >= 0 )
	{
		switch(cmd)
		{
		}
	}

	return 0;
}
*/

static int set_env_var_cmd(HXCFE * context, char * line,int cmd)
{
	int i,j,ret;
	char varname[MAX_CFG_STRING_SIZE];
	char varvalue[MAX_CFG_STRING_SIZE];

	ret = -1;

	i = get_param(line, 1,varname);
	j = get_param(line, 2,varvalue);

	if(i>=0 && j>=0)
	{
		ret = hxcfe_setEnvVar(context,(char*)&varname,(char*)&varvalue);
	}

	return ret;
}

static int print_env_var_cmd(HXCFE * context, char * line,int cmd)
{
	int i,ret;
	char varname[MAX_CFG_STRING_SIZE];
	char varvalue[MAX_CFG_STRING_SIZE];

	ret = -1;

	i = get_param(line, 1,varname);

	if(i>=0)
	{
		ret = hxcfe_setEnvVar(context,(char*)&varname,(char*)&varvalue);
		if(ret>=0)
		{
			context->hxc_printf(MSG_INFO_1,"%s = %s",varname,varvalue);
		}
	}

	return ret;
}

kw_list kwlist[] =
{
	{"version",             print_version_cmd,      VERSION_CMD},
	{"set",                 set_env_var_cmd,        SET_ENV_CMD},
	{"print_env_var",       print_env_var_cmd,      PRINT_ENV_VAR_CMD},

	//{"exec",          get_hex_param,      EXEC_CMD},

	{ 0, 0, 0 }
};

static int exec_cmd(HXCFE * context, char * command,char * line)
{
	int i;

	i = 0;
	while(kwlist[i].func)
	{
		if( !strcmp(kwlist[i].keyword,command) )
		{
			kwlist[i].func(context, line, kwlist[i].cmd);
			return 1;
		}

		i++;
	}

	return 0;
}

int hxcfe_execScriptLine( HXCFE * hxcfe,char * line )
{
	char command[MAX_CFG_STRING_SIZE];

	command[0] = 0;

	if( extract_cmd(line, command) )
	{
		if(strlen(command))
		{
			if( !exec_cmd(hxcfe, command,line))
			{
				hxcfe->hxc_printf(MSG_ERROR,"Line syntax error : %s",line);

				return 0;
			}
		}

		return 1;
	}

	return 0;
}

int hxcfe_execScriptFile( HXCFE* hxcfe, char * script_path )
{
	int err;
	FILE * f;
	char line[MAX_CFG_STRING_SIZE];

	err = HXCFE_INTERNALERROR;

	f = fopen(script_path,"r");
	if(f)
	{
		do
		{
			if(!fgets(line,sizeof(line),f))
				break;

			if(feof(f))
				break;

			hxcfe_execScriptLine(hxcfe, line);
		}while(1);

		fclose(f);

		err = HXCFE_NOERROR;
	}
	else
	{
		hxcfe->hxc_printf(MSG_ERROR,"Can't open %s !",script_path);
		err = HXCFE_ACCESSERROR;
	}

	return err;
}

int hxcfe_execScriptRam( HXCFE* hxcfe, unsigned char * script_buffer, int buffersize )
{
	int err = 0;
	int buffer_offset,line_offset;
	char line[MAX_CFG_STRING_SIZE];

	buffer_offset = 0;
	line_offset = 0;

	do
	{
		memset(line,0,MAX_CFG_STRING_SIZE);
		line_offset = 0;
		while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && script_buffer[buffer_offset]!='\n' && script_buffer[buffer_offset]!='\r' && (line_offset < MAX_CFG_STRING_SIZE - 1))
		{
			line[line_offset++] = script_buffer[buffer_offset++];
		}

		while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && (script_buffer[buffer_offset]=='\n' || script_buffer[buffer_offset]=='\r') )
		{
			buffer_offset++;
		}

		hxcfe_execScriptLine(hxcfe, line);

		if( (buffer_offset >= buffersize) || !script_buffer[buffer_offset])
			break;

	}while(buffer_offset < buffersize);

	return err;
}
