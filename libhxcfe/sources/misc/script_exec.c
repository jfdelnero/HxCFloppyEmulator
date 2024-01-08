/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// Contains: Script engine.
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#include "script_exec.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "libhxcfe.h"
#include "floppy_loader.h"

#include "libhxcadaptor.h"

#include "env.h"

#include "version.h"

#define SCRIPT_NO_ERROR            HXCFE_NOERROR
#define	SCRIPT_CMD_NOT_FOUND       HXCFE_INTERNALERROR
#define	SCRIPT_NOT_FOUND           HXCFE_ACCESSERROR
#define	SCRIPT_CMD_BAD_PARAMETER   HXCFE_BADPARAMETER
#define	SCRIPT_BAD_CMD             HXCFE_INTERNALERROR
#define	SCRIPT_INTERNAL_ERROR      HXCFE_INTERNALERROR
#define	SCRIPT_ACCESS_ERROR        HXCFE_ACCESSERROR
#define	SCRIPT_MEM_ERROR           HXCFE_INTERNALERROR

#ifndef _script_cmd_func_
typedef int (* CMD_FUNC)( script_ctx * ctx, char * line);
#define _script_cmd_func_
#endif

typedef struct cmd_list_
{
	char * command;
	CMD_FUNC func;
}cmd_list;

typedef struct label_list_
{
	char * label;
	unsigned int file_offset;
}label_list;

extern cmd_list script_commands_list[];

static int dummy_script_printf(void * ctx, int MSGTYPE, char * string, ... )
{
	return 0;
}

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

static int is_label(char * command)
{
	int i;

	i = 0;
	while(command[i])
	{
		i++;
	}

	if(i>1)
	{
		if(command[i - 1] == ':')
			return 1;
	}

	return 0;
}

static int is_variable(char * command)
{
	if(strlen(command)>1)
	{
		if(command[0] == '$' && command[1] && (command[1] != ' ' && command[1] != '\t') )
			return 1;
		else
			return 0;
	}

	return 0;
}

static int get_next_word(char * line, int offset)
{
	while( !is_end_line(line[offset]) && ( line[offset] == ' ' || line[offset] == '\t' ) )
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
	while( !is_end_line(line[offs]) && ( insidequote || !is_space(line[offs]) ) && (i < (DEFAULT_BUFLEN - 1)) )
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

	if( param )
	{
		param_cnt = 0;
		do
		{
			offs = copy_param(NULL, line, offs);

			offs = get_next_word( line, offs );

			if(line[offs] == 0 || line[offs] == '#' || line[offs] == '\r' || line[offs] == '\n')
				return -1;

			param_cnt++;
		}while( param_cnt < param );
	}

	return offs;
}

static int get_param( script_ctx * ctx, char * line, int param_offset,char * param)
{
	int offs;
	char var_str[DEFAULT_BUFLEN];

	offs = get_param_offset(line, param_offset);

	if(offs>=0)
	{
		if(line[offs] != '$')
		{
			offs = copy_param(param, line, offs);
		}
		else
		{
			copy_param(var_str, line, offs);

			if(!strcmp(var_str,"$LASTDATA"))
			{
				sprintf(param,"0x"LONGHEXSTR,ctx->last_data_value);
				return 1;
			}

			if(!strcmp(var_str,"$LASTFLAGS"))
			{
				sprintf(param,"0x%X",ctx->last_flags);
				return 1;
			}

			if(!strcmp(var_str,"$LASTERROR"))
			{
				sprintf(param,"%d",ctx->last_error_code);
				return 1;
			}

			if( !getEnvVarDat( (envvar_entry *)ctx->env, (char*)&var_str[1], param, DEFAULT_BUFLEN) )
			{
				copy_param(param, line, offs);
			}
		}

		return 1;
	}

	return -1;
}

static int get_param_str( script_ctx * ctx, char * line, int param_offset,char * param)
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

static env_var_value str_to_int(char * str)
{
	env_var_value value;

	value = 0;

	if(str)
	{
		if( strlen(str) > 2 )
		{
			if( str[0]=='0' && ( str[1]=='x' || str[1]=='X'))
			{
				value = (env_var_value)STRTOVALUE(str, NULL, 0);
			}
			else
			{
				value = atoi(str);
			}
		}
		else
		{
			value = atoi(str);
		}
	}

	return value;
}

static env_var_value get_script_variable( script_ctx * ctx, char * varname)
{
	env_var_value value;

	if(!strcmp(varname,"$LASTDATA"))
	{
		return ctx->last_data_value;
	}

	if(!strcmp(varname,"$LASTFLAGS"))
	{
		return ctx->last_flags;
	}

	if(!strcmp(varname,"$LASTERROR"))
	{
		return (env_var_value)(ctx->last_error_code);
	}

	if(varname[0] == '$')
		value = getEnvVarValue( (envvar_entry *)ctx->env, (char*)&varname[1]);
	else
		value = str_to_int((char*)varname);

	return value;
}

static void set_script_variable( script_ctx * ctx, char * varname, env_var_value value)
{
	char tmp_str[64];

	if(!strcmp(varname,"$LASTDATA"))
	{
		ctx->last_data_value = value;

		return;
	}

	if(!strcmp(varname,"$LASTFLAGS"))
	{
		ctx->last_flags = value;

		return;
	}

	if(!strcmp(varname,"$LASTERROR"))
	{
		ctx->last_error_code = value;

		return;
	}

	if(varname[0] == '$' && varname[1])
	{
		sprintf(tmp_str,"0x"LONGHEXSTR,value);
		setEnvVarDat( (envvar_entry *)ctx->env, (char*)&varname[1], tmp_str );

		return;
	}
}

script_ctx * init_script(void * app_ctx, unsigned int flags, void * env)
{
	script_ctx * ctx;

	ctx = malloc(sizeof(script_ctx));

	if(ctx)
	{
		memset(ctx,0,sizeof(script_ctx));

		ctx->env = (envvar_entry*)env;

		setOutputFunc_script( ctx, dummy_script_printf );

		ctx->app_ctx = (void*)app_ctx;
		ctx->cur_label_index = 0;

		ctx->cmdlist = (void*)script_commands_list;

		ctx->script_file = NULL;
	}

	return ctx;
}

static int extract_cmd( script_ctx * ctx, char * line, char * command)
{
	int offs,i;

	i = 0;
	offs = 0;

	offs = get_next_word(line, offs);

	if( !is_end_line(line[offs]) )
	{
		while( !is_end_line(line[offs]) && !is_space(line[offs]) && i < (DEFAULT_BUFLEN - 1) )
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

static int exec_cmd( script_ctx * ctx, char * command,char * line)
{
	int i;
	cmd_list * cmdlist;

	cmdlist = (cmd_list*)ctx->cmdlist;

	if(!cmdlist)
		return SCRIPT_INTERNAL_ERROR;

	i = 0;
	while(cmdlist[i].func)
	{
		if( !strcmp(cmdlist[i].command,command) )
		{
			return cmdlist[i].func(ctx,line);
		}

		i++;
	}

	return SCRIPT_CMD_NOT_FOUND;
}

static int add_label( script_ctx * ctx, char * label )
{
	int i,j;
	char tmp_label[MAX_LABEL_SIZE];

	if(ctx->cur_label_index < MAX_LABEL)
	{
		i = 0;
		while(i<(MAX_LABEL_SIZE - 1) && label[i] && label[i] != ':')
		{
			tmp_label[i] = label[i];
			i++;
		}
		tmp_label[i] = 0;

		i = 0;
		while(i<ctx->cur_label_index)
		{
			if( !strcmp( tmp_label, ctx->labels[i].label_name ) )
			{
				break;
			}
			i++;
		}

		j = i;

		i = 0;
		while(i<(MAX_LABEL_SIZE - 1) && label[i])
		{
			ctx->labels[j].label_name[i] = tmp_label[i];
			i++;
		}

		ctx->labels[j].label_name[i] = 0;
		ctx->labels[j].offset = ctx->cur_script_offset;

		if(ctx->cur_label_index == j)
		{
			ctx->cur_label_index++;
		}
	}
	return 0;
}

static int goto_label( script_ctx * ctx, char * label )
{
	int i;
	char tmp_label[MAX_LABEL_SIZE];

	i = 0;
	while(i<(MAX_LABEL_SIZE - 1) && label[i] && label[i] != ':')
	{
		tmp_label[i] = label[i];
		i++;
	}
	tmp_label[i] = 0;

	i = 0;
	while(i<ctx->cur_label_index)
	{
		if( !strcmp( tmp_label, ctx->labels[i].label_name ) )
		{
			break;
		}
		i++;
	}

	if( i != ctx->cur_label_index)
	{
		ctx->cur_script_offset = ctx->labels[i].offset;

		if(ctx->script_file)
			fseek(ctx->script_file,ctx->cur_script_offset,SEEK_SET);

		return SCRIPT_NO_ERROR;
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Label %s not found\n", tmp_label );

		return SCRIPT_CMD_NOT_FOUND;
	}
}

///////////////////////////////////////////////////////////////////////////////

static int alu_operations( script_ctx * ctx, char * line)
{
	int i;
	int valid;
	env_var_value data_value;
	env_var_value value_1,value_2;

	char params_str[5][DEFAULT_BUFLEN];

	for(i=0;i<5;i++)
	{
		params_str[i][0] = 0;
	}

	valid = 0;
	for(i=0;i<5;i++)
	{
		get_param_str( ctx, line, i, (char*)&params_str[i] );
		if(strlen((char*)&params_str[i]))
			valid++;
	}

	data_value = 0;
	if( ( (valid == 3) || (valid == 5) ) && params_str[1][0] == '=' && params_str[0][0] == '$')
	{
		value_1 = get_script_variable( ctx, params_str[2]);

		if(valid == 5)
		{
			value_2 = get_script_variable( ctx, params_str[4]);

			if(!strcmp(params_str[3],"+"))
				data_value = value_1 + value_2;

			if(!strcmp(params_str[3],"-"))
				data_value = value_1 - value_2;

			if(!strcmp(params_str[3],"*"))
				data_value = value_1 * value_2;

			if(!strcmp(params_str[3],"/") && value_2)
				data_value = value_1 / value_2;

			if(!strcmp(params_str[3],"&"))
				data_value = value_1 & value_2;

			if(!strcmp(params_str[3],"^"))
				data_value = value_1 ^ value_2;

			if(!strcmp(params_str[3],"|"))
				data_value = value_1 | value_2;

			if(!strcmp(params_str[3],">>"))
				data_value = value_1 >> value_2;

			if(!strcmp(params_str[3],"<<"))
				data_value = value_1 << value_2;
		}
		else
		{
			data_value = value_1;
		}

		if(data_value)
			ctx->last_flags = 1;
		else
			ctx->last_flags = 0;

		set_script_variable( ctx, (char*)&params_str[0], data_value);

		return SCRIPT_NO_ERROR;
	}

	return SCRIPT_CMD_BAD_PARAMETER;
}

void setOutputFunc_script( script_ctx * ctx, SCRIPT_PRINTF_FUNC ext_printf )
{
	ctx->script_printf = ext_printf;

	return;
}

int execute_line_script( script_ctx * ctx, char * line )
{
	char command[DEFAULT_BUFLEN];

	command[0] = 0;

	if( extract_cmd(ctx, line, command) )
	{
		if(strlen(command))
		{
			if(!is_label(command))
			{
				if(!ctx->dry_run)
				{
					if(!is_variable(command))
					{
						ctx->last_error_code = exec_cmd(ctx,command,line);

						if( ctx->last_error_code == SCRIPT_CMD_NOT_FOUND )
						{
							ctx->script_printf( ctx, MSG_ERROR, "Command not found ! : %s\n", line );

							return ctx->last_error_code;
						}
					}
					else
					{
						ctx->last_error_code = alu_operations(ctx,line);
					}
				}
				else
					ctx->last_error_code = SCRIPT_NO_ERROR;
			}
			else
			{
				add_label(ctx,command);

				ctx->last_error_code = SCRIPT_NO_ERROR;
			}

			return ctx->last_error_code;
		}
	}

	ctx->last_error_code = SCRIPT_BAD_CMD;

	return ctx->last_error_code;
}

int execute_file_script( script_ctx * ctx, char * filename )
{
	int err;
	char line[DEFAULT_BUFLEN];

	err = SCRIPT_INTERNAL_ERROR;

	ctx->script_file = fopen(filename,"rb");
	if(ctx->script_file)
	{
		strncpy(ctx->script_file_path,filename,DEFAULT_BUFLEN);
		ctx->script_file_path[DEFAULT_BUFLEN-1] = 0;

		// Dry run -> populate the labels...
		ctx->dry_run++;
		do
		{
			if(!fgets(line,sizeof(line),ctx->script_file))
				break;

			ctx->cur_script_offset = ftell(ctx->script_file);

			if(feof(ctx->script_file))
				break;

			execute_line_script(ctx, line);
		}while(1);

		fseek(ctx->script_file,0,SEEK_SET);
		ctx->cur_script_offset = ftell(ctx->script_file);

		ctx->dry_run--;
		if(!ctx->dry_run)
		{
			if(strlen(ctx->pre_command))
			{
				err = execute_line_script(ctx, ctx->pre_command);
				if(err != SCRIPT_NO_ERROR)
				{
					fclose(ctx->script_file);
					return err;
				}
			}

			do
			{
				if(!fgets(line,sizeof(line),ctx->script_file))
					break;

				ctx->cur_script_offset = ftell(ctx->script_file);

				if(feof(ctx->script_file))
					break;

				err = execute_line_script(ctx, line);
			}while(1);
		}

		fclose(ctx->script_file);

		err = SCRIPT_NO_ERROR;
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Can't open %s !", filename );
		ctx->script_file_path[0] = 0;

		err = SCRIPT_ACCESS_ERROR;
	}

	return err;
}

int execute_ram_script( script_ctx * ctx, unsigned char * script_buffer, int buffersize )
{
	int err = 0;
	int buffer_offset,line_offset;
	char line[DEFAULT_BUFLEN];
	int cont;

	ctx->dry_run++;
	cont = 1;

	while( cont )
	{
		buffer_offset = 0;
		line_offset = 0;
		ctx->cur_script_offset = 0;

		do
		{
			memset(line,0,DEFAULT_BUFLEN);
			line_offset = 0;
			while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && script_buffer[buffer_offset]!='\n' && script_buffer[buffer_offset]!='\r' && (line_offset < DEFAULT_BUFLEN - 1))
			{
				line[line_offset++] = script_buffer[buffer_offset++];
			}

			while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && (script_buffer[buffer_offset]=='\n' || script_buffer[buffer_offset]=='\r') )
			{
				buffer_offset++;
			}

			ctx->cur_script_offset = buffer_offset;

			execute_line_script(ctx, line);

			buffer_offset = ctx->cur_script_offset;

			if( (buffer_offset >= buffersize) || !script_buffer[buffer_offset])
				break;

		}while(buffer_offset < buffersize);

		if( !ctx->dry_run || (ctx->dry_run > 1) )
		{
			cont = 0;
		}
		else
		{
			ctx->dry_run = 0;

			if(strlen(ctx->pre_command))
				execute_line_script(ctx, ctx->pre_command);
		}
	}

	return err;
}

script_ctx * deinit_script(script_ctx * ctx)
{
	free(ctx);

	ctx = NULL;

	return ctx;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////// Generic commands/operations /////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int cmd_goto( script_ctx * ctx, char * line)
{
	int i;
	char label_str[DEFAULT_BUFLEN];

	i = get_param( ctx, line, 1, label_str );

	if(i>=0)
	{
		return goto_label( ctx, label_str );
	}

	return SCRIPT_CMD_BAD_PARAMETER;
}

static int cmd_if( script_ctx * ctx, char * line)
{
	//"if" command example :
	// if $VARIABLE > 0x2222 then goto label

	int i;
	int eval;
	int ret;
	int valid;
	char params_str[5][DEFAULT_BUFLEN];
	env_var_value value_1,value_2,tmp_val;
	int op_offset;

	ret = SCRIPT_CMD_BAD_PARAMETER;

	eval = 0;

	for(i=0;i<5;i++)
	{
		params_str[i][0] = 0;
	}

	valid = 0;
	for(i=0;i<5;i++)
	{
		get_param( ctx, line, i, (char*)&params_str[i] );
		if(strlen((char*)&params_str[i]))
			valid++;
	}

	i = 0;
	while( i < 5 && strcmp((char*)&params_str[i],"then") )
	{
		i++;
	}

	if( i < 5 )
	{
		if( i == 2)
		{
			value_1 = get_script_variable( ctx, params_str[1]);

			if(value_1)
				eval = 1;

			ret = SCRIPT_NO_ERROR;
		}

		if ( i == 4 )
		{
			value_1 = get_script_variable( ctx, params_str[1]);
			value_2 = get_script_variable( ctx, params_str[3]);

			if(!strcmp((char*)&params_str[2],">=") && ( (signed_env_var_value)value_1 >= (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"<=") && ( (signed_env_var_value)value_1 <= (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],">") && ( (signed_env_var_value)value_1 > (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"<") && ( (signed_env_var_value)value_1 < (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"==") && ( value_1 == value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"!=") && ( value_1 != value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"&") && ( value_1 & value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"^") && ( value_1 ^ value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"|") && ( value_1 | value_2 ) )
				eval = 1;

			tmp_val = value_1 >> value_2;
			if(!strcmp((char*)&params_str[2],">>") && tmp_val )
				eval = 1;

			tmp_val = value_1 << value_2;
			if(!strcmp((char*)&params_str[2],"<<") && tmp_val )
				eval = 1;

			ret = SCRIPT_NO_ERROR;
		}

		if( eval )
		{
			op_offset = get_param_offset(line, i + 1);

			if(op_offset >= 0)
			{
				ret = execute_line_script( ctx, (char*)&line[op_offset] );
			}
		}

		return ret;
	}

	return SCRIPT_CMD_BAD_PARAMETER;
}

static int cmd_return( script_ctx * ctx, char * line )
{
	if(ctx->script_file)
	{
		fseek(ctx->script_file,0,SEEK_END);
	}

	return SCRIPT_NO_ERROR;
}

static int cmd_system( script_ctx * ctx, char * line )
{
	int offs;
	int ret;

	offs = get_param_offset(line, 1);

	if(offs>=0)
	{
		ret = system(&line[offs]);

		if( ret != 1 )
			return SCRIPT_NO_ERROR;
		else
			return SCRIPT_CMD_NOT_FOUND;
	}

	return SCRIPT_CMD_BAD_PARAMETER;
}

static int cmd_print_env_var( script_ctx * ctx, char * line )
{
	int i;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	char * ptr;

	i = get_param( ctx, line, 1, varname );

	if(i>=0)
	{
		ptr = getEnvVarDat( (envvar_entry *)ctx->env, (char*)&varname, (char*)&varvalue, DEFAULT_BUFLEN );
		if(ptr)
		{
			ctx->script_printf( ctx, MSG_INFO_1, "%s = %s", varname, varvalue );

			return SCRIPT_NO_ERROR;
		}

		return SCRIPT_CMD_NOT_FOUND;
	}
	else
	{
		return SCRIPT_CMD_BAD_PARAMETER;
	}
}

static int cmd_version( script_ctx * ctx, char * line)
{
	ctx->script_printf( ctx, MSG_INFO_0, "Lib version : %s, Date : "__DATE__" "__TIME__"\n", "v"STR_FILE_VERSION2 );

	return SCRIPT_NO_ERROR;
}

static int cmd_print( script_ctx * ctx, char * line)
{
	int i,j,s;
	char tmp_str[DEFAULT_BUFLEN];
	char str[DEFAULT_BUFLEN*2];
	char * ptr;

	str[0] = 0;

	j = 1;
	do
	{
		ptr = NULL;

		i = get_param_offset(line, j);
		s = 0;

		if(i>=0)
		{
			get_param( ctx, line, j, (char *)&tmp_str );
			s = strlen(tmp_str);
			if(s)
			{
				if(tmp_str[0] != '$')
				{
					strncat((char*)str,tmp_str,sizeof(str) - 1);
					strncat((char*)str," ",sizeof(str) - 1);
				}
				else
				{
					ptr = getEnvVarDat( (envvar_entry *)ctx->env, &tmp_str[1], NULL, 0);
					if( ptr )
					{
						strncat((char*)str,ptr,sizeof(str) - 1);
						strncat((char*)str," ",sizeof(str) - 1);

					}
					else
					{
						strncat((char*)str,tmp_str,sizeof(str) - 1);
						strncat((char*)str," ",sizeof(str) - 1);
					}
				}
			}
		}

		j++;
	}while(s);

	ctx->script_printf( ctx, MSG_NONE, "%s\n", str );

	return SCRIPT_NO_ERROR;
}

static int cmd_pause( script_ctx * ctx, char * line)
{
	int i;
	char delay_str[DEFAULT_BUFLEN];

	i = get_param( ctx, line, 1, delay_str );

	if(i>=0)
	{
		hxc_pause(str_to_int(delay_str));

		return SCRIPT_NO_ERROR;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return SCRIPT_CMD_BAD_PARAMETER;
}

static int cmd_help( script_ctx * ctx, char * line)
{
	int i;

	cmd_list * cmdlist;

	cmdlist = (cmd_list*)ctx->cmdlist;

	ctx->script_printf( ctx, MSG_INFO_0, "Supported Commands :\n\n" );

	i = 0;
	while(cmdlist[i].func)
	{
		ctx->script_printf( ctx, MSG_NONE, "%s\n", cmdlist[i].command );
		i++;
	}

	return SCRIPT_NO_ERROR;
}

static int cmd_call( script_ctx * ctx, char * line )
{
	int offs;
	char path[DEFAULT_BUFLEN];
	char function[DEFAULT_BUFLEN];
	script_ctx * new_ctx;
	int ret;
	HXCFE * hxcfectx;

	hxcfectx = (HXCFE *)ctx->app_ctx;

	get_param( ctx, line, 1, (char*)&path );

	offs = get_param_offset(line, 1);

	if(offs>=0)
	{
		ret = SCRIPT_INTERNAL_ERROR;

		new_ctx = init_script((void*)hxcfectx,0x00000000,(void*)hxcfectx->envvar);
		if(new_ctx)
		{
			new_ctx->script_printf = ctx->script_printf;

			function[0] = 0;
			get_param( ctx, line, 2, (char*)&function );

			if(!strcmp(path,"."))
			{
				if(strlen(function))
				{
					snprintf(new_ctx->pre_command,sizeof(new_ctx->pre_command),"goto %s",function);

					ret = execute_file_script( new_ctx, (char*)&ctx->script_file_path );

					new_ctx->pre_command[0] = 0;

					if( ret == SCRIPT_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script not found ! : %s\n", path );
					}
				}
			}
			else
			{
				if(strlen(function))
				{
					snprintf(new_ctx->pre_command,sizeof(new_ctx->pre_command),"goto %s",function);

					ret = execute_file_script( new_ctx, (char*)&path );

					new_ctx->pre_command[0] = 0;

					if( ret == SCRIPT_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script/function not found ! : %s %s\n", path,function );
					}
				}
				else
				{
					ret = execute_file_script( new_ctx, (char*)&path );

					if( ret == SCRIPT_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script not found ! : %s\n", path );
					}
				}
			}

			deinit_script(new_ctx);
		}

		ctx->last_error_code = ret;

		return ret;
	}

	return SCRIPT_CMD_BAD_PARAMETER;
}

static int cmd_set_env_var( script_ctx * ctx, char * line )
{
	int i,j,ret;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];

	ret = SCRIPT_CMD_BAD_PARAMETER;

	i = get_param( ctx, line, 1, varname );
	j = get_param( ctx, line, 2, varvalue );

	if(i>=0 && j>=0)
	{
		if( setEnvVarDat( (envvar_entry *)ctx->env, (char*)&varname, (char*)&varvalue ) >= 0)
		{
			ret = SCRIPT_NO_ERROR;
		}
		else
			ret = SCRIPT_MEM_ERROR;
	}

	return ret;
}

static int cmd_rand( script_ctx * ctx, char * line)
{
	int i;
	uint32_t seed;

	char rand_seed[DEFAULT_BUFLEN];

	seed = ctx->rand_seed;

	i = get_param( ctx, line, 1, rand_seed );
	if(i>=0)
	{
		seed = str_to_int((char*)rand_seed);
	}

	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;

	ctx->rand_seed = seed;

	ctx->last_data_value = seed;

	return SCRIPT_NO_ERROR;
}

int is_valid_hex_quartet(char c)
{
	if( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
		return 1;
	else
		return 0;
}

int is_valid_hex_byte(char *str)
{
	if( is_valid_hex_quartet(str[0]) && is_valid_hex_quartet(str[1]) )
		return 1;
	else
		return 0;
}

char * arrayresize(char * array, int size, unsigned char c)
{
	int cursize;
	char * ptr;

	if(array)
	{
		if( array[0] == '0' && (array[1] == 'x' || array[1] == 'X') )
		{
			ptr = (array + 2);
		}
		else
		{
			ptr = (array);
		}
	}
	else
	{
		array = malloc(DEFAULT_BUFLEN);
		if(array)
		{
			memset(array,0,DEFAULT_BUFLEN);
		}

		ptr = array;
	}

	if(ptr)
	{
		cursize = 0;
		while(is_valid_hex_byte(&ptr[cursize*2]))
		{
			cursize++;
		}

		if( cursize < size  )
		{
			while( cursize < size )
			{
				sprintf(&ptr[(cursize*2) + 0],"%.2X",c);
				//ptr[(cursize*2) + 0] = '0';
				//ptr[(cursize*2) + 1] = '0';
				cursize++;
			}
		}
		else
		{
			ptr[ (size * 2) ] = 0;
		}
	}

	return array;
}


static int cmd_initarray( script_ctx * ctx, char * line)
{
	//initarray  $VARIABLE_1_TEST $BYTES $VALUE

	int i,j,ret;
	char varname[DEFAULT_BUFLEN];
	char varsize[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	char * ptr;
	int size;

	ret = SCRIPT_CMD_BAD_PARAMETER;

	strcpy(varvalue,"0");

	i = get_param_str( ctx, line, 1, varname );
	j = get_param( ctx, line, 2, varsize );
	get_param( ctx, line, 3, varvalue );

	if(i>=0 && j>=0)
	{
		size = atoi(varsize);

		if(size)
		{
			ptr = getEnvVarDat( (envvar_entry *)ctx->env,(char*)&varname, NULL, 0);
			if(ptr)
			{
				arrayresize(ptr, size, (unsigned char)(str_to_int((char*)&varvalue)&0xFF));
			}
			else
			{
				ptr = malloc( DEFAULT_BUFLEN );
				if(ptr)
				{
					memset(ptr,0x00, DEFAULT_BUFLEN);
					arrayresize(ptr, size, (unsigned char)(str_to_int((char*)&varvalue)&0xFF));
				}
			}

			if(ptr)
				setEnvVarDat( (envvar_entry *)ctx->env, (char*)&varname[1], ptr );
		}
		ret = SCRIPT_NO_ERROR;
	}

	return ret;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////// Floppy commands/operations //////////////////////////////
///////////////////////////////////////////////////////////////////////////////

cmd_list script_commands_list[] =
{
	{"print",                   cmd_print},
	{"help",                    cmd_help},
	{"?",                       cmd_help},
	{"version",                 cmd_version},
	{"pause",                   cmd_pause},
	{"set",                     cmd_set_env_var},
	{"print_env_var",           cmd_print_env_var},
	{"call",                    cmd_call},
	{"system",                  cmd_system},

	{"if",                      cmd_if},
	{"goto",                    cmd_goto},
	{"return",                  cmd_return},

	{"rand",                    cmd_rand},

	{"init_array",              cmd_initarray},

	{ 0, 0 }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int hxcfe_initScript(HXCFE * hxcfectx)
{
	hxcfectx->scriptctx = init_script((void*)hxcfectx,0x00000000,(void*)hxcfectx->envvar);

	if(!hxcfectx->scriptctx)
		return SCRIPT_INTERNAL_ERROR;
	else
		return SCRIPT_NO_ERROR;
}

void hxcfe_setScriptOutputFunc( HXCFE * hxcfectx, SCRIPT_PRINTF_FUNC ext_printf )
{
	setOutputFunc_script(hxcfectx->scriptctx, ext_printf);
}

int hxcfe_execScriptLine( HXCFE * hxcfe,char * line )
{
	if(!hxcfe)
		return SCRIPT_INTERNAL_ERROR;

	if(!hxcfe->scriptctx)
		return SCRIPT_INTERNAL_ERROR;

	return execute_line_script( hxcfe->scriptctx, line );
}

int hxcfe_execScriptFile( HXCFE* hxcfe, char * script_path )
{
	if(!hxcfe)
		return SCRIPT_INTERNAL_ERROR;

	if(!hxcfe->scriptctx)
		return SCRIPT_INTERNAL_ERROR;

	return execute_file_script( hxcfe->scriptctx, script_path );
}

int hxcfe_execScriptRam( HXCFE* hxcfe, unsigned char * script_buffer, int buffersize )
{
	if(!hxcfe)
		return SCRIPT_INTERNAL_ERROR;

	if(!hxcfe->scriptctx)
		return SCRIPT_INTERNAL_ERROR;

	return execute_ram_script( hxcfe->scriptctx, script_buffer, buffersize );
}

int hxcfe_deinitScript( HXCFE* hxcfe )
{
	if(!hxcfe)
		return SCRIPT_CMD_BAD_PARAMETER;

	if(!hxcfe->scriptctx)
		return SCRIPT_INTERNAL_ERROR;

	deinit_script(hxcfe->scriptctx);

	hxcfe->scriptctx = NULL;

	return SCRIPT_NO_ERROR;
}

int hxcfe_setEnvVar( HXCFE* hxcfe, char * varname, char * varvalue )
{
	if( setEnvVarDat( hxcfe->envvar, varname, varvalue ) >= 0 )
	{
		return SCRIPT_NO_ERROR;
	}
	else
	{
		return SCRIPT_MEM_ERROR;
	}
}

char * hxcfe_getEnvVar( HXCFE* hxcfe, char * varname, char * varvalue)
{
	return getEnvVarDat( hxcfe->envvar, varname, varvalue, DEFAULT_BUFLEN);
}

char * hxcfe_getEnvVarIndex( HXCFE* hxcfe, int index, char * varvalue)
{
	return getEnvVarDatIndex( hxcfe->envvar, index, varvalue, DEFAULT_BUFLEN);
}

int hxcfe_getEnvVarValue( HXCFE* hxcfe, char * varname)
{
	return getEnvVarValue( hxcfe->envvar, varname);
}

int hxcfe_setEnvVarValue( HXCFE* hxcfe, char * varname, int value )
{
	if( setEnvVarValue( hxcfe->envvar, varname, value ) >= 0 )
	{
		return SCRIPT_NO_ERROR;
	}
	else
	{
		return SCRIPT_MEM_ERROR;
	}
}
