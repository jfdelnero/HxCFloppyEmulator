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
// File : cache.c
// Contains: File cache layer
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2022 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "cache.h"

#define MAXRAMENTRIES 16

typedef struct ramfile_
{
	unsigned char * buf;
	int size;
	int offset;
}ramfile;

static ramfile ram[MAXRAMENTRIES];

void init_ramfiles()
{
	for(int i=0;i<MAXRAMENTRIES;i++)
	{
		memset(&ram[i],0,sizeof(ramfile));
	}
}

void erase_ramfile(int i)
{
	if(i>= MAXRAMENTRIES)
		return;

	if(ram[i].buf)
		free(ram[i].buf);

	ram[i].buf = NULL;
	ram[i].size = 0;
}

static void * fopen_ramfile( int i, char * mode )
{
	if(i>= MAXRAMENTRIES)
		return NULL;

	ram[i].offset = 0;

	return (void*)1;
}

static int fread_ramfile( void * buf, int size, int nb, int i )
{
	if(i>= MAXRAMENTRIES)
		return -1;

	if(ram[i].offset + size > ram[i].size)
		return -1;

	memcpy(buf, &ram[i].buf[ram[i].offset], size);

	ram[i].offset += size;

	return 1;
}

static int fwrite_ramfile( void * buf, int size, int nb, int i)
{
	unsigned char * ptr;

	if(i>= MAXRAMENTRIES)
		return -1;

	if( ram[i].offset + size > ram[i].size )
	{
		ptr = realloc( ram[i].buf, ram[i].offset + size );
		if( !ptr )
			return -1;

		ram[i].buf = ptr;
		ram[i].size = ram[i].offset + size;
	}

	memcpy(&ram[i].buf[ram[i].offset], buf, size);

	ram[i].offset += size;

	return 1;
}

static int fseek_ramfile( int i, int offset, int whence )
{
	if(i>= MAXRAMENTRIES)
		return -1;

	switch(whence)
	{
		case SEEK_SET:
			ram[i].offset = offset;
			if(ram[i].offset >= ram[i].size)
				ram[i].offset = ram[i].size ;
		return 0;
		case SEEK_END:
			ram[i].offset = ram[i].size;
		return 0;
	}

	return 1;
}

static int ftell_ramfile( int i )
{
	if(i>= MAXRAMENTRIES)
		return -1;

	return ram[i].offset;
}

static int fclose_ramfile( int i )
{
	if(i>= MAXRAMENTRIES)
		return -1;

	return 0;
}

void deinit_ramfiles()
{
	for(int i=0;i<MAXRAMENTRIES;i++)
	{
		erase_ramfile(i);
	}
}

int open_file(file_cache * fc, char* path, int filesize,unsigned char fill)
{
	memset(fc,0,sizeof(file_cache));

	fc->ramfile = -1;
	fc->fill_val = fill;

	if(!strncmp(path,"ram", 3) && (path[3] >= '0') && (path[3] <= '9') && (path[4] == ':') )
	{
		fc->ramfile = path[3] - '0';
	}

	if( filesize < 0 )
	{   // Read mode
		if( fc->ramfile >= 0 )
		{
			fc->f = fopen_ramfile(fc->ramfile,"rb");
			if(fc->f)
			{
				if(fseek_ramfile(fc->ramfile,0,SEEK_END))
					goto error;

				fc->file_size = ftell_ramfile(fc->ramfile);

				if(fseek_ramfile(fc->ramfile,fc->current_offset,SEEK_SET))
					goto error;

				if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
					fc->cur_page_size = ( fc->file_size - fc->current_offset);
				else
					fc->cur_page_size = FILE_CACHE_SIZE;

				memset(&fc->cache_buffer,fill,FILE_CACHE_SIZE);

				if(fc->file_size)
				{
					if( fread_ramfile(&fc->cache_buffer,fc->cur_page_size,1,fc->ramfile) != 1 )
						goto error;
				}

				return 0;
			}
		}
		else
		{
			fc->f = fopen(path,"rb");
			if(fc->f)
			{
				if(fseek(fc->f,0,SEEK_END))
					goto error;

				fc->file_size = ftell(fc->f);

				if(fseek(fc->f,fc->current_offset,SEEK_SET))
					goto error;

				if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
					fc->cur_page_size = ( fc->file_size - fc->current_offset);
				else
					fc->cur_page_size = FILE_CACHE_SIZE;

				memset(&fc->cache_buffer,fill,FILE_CACHE_SIZE);

				if(fc->file_size)
				{
					if( fread(&fc->cache_buffer,fc->cur_page_size,1,fc->f) != 1 )
						goto error;
				}

				return 0;
			}
		}
	}
	else
	{
		if( fc->ramfile >= 0 )
		{
			// write mode
			fc->f = fopen_ramfile(fc->ramfile,"wb");
		}
		else
		{
			// write mode
			fc->f = fopen(path,"wb");
		}

		if(fc->f)
		{
			memset(&fc->cache_buffer,fill,FILE_CACHE_SIZE);

			while( fc->file_size < filesize)
			{

				if( fc->file_size + FILE_CACHE_SIZE > filesize )
				{
					if( fc->ramfile >= 0 )
					{
						if( fwrite_ramfile( &fc->cache_buffer, filesize - fc->file_size, 1, fc->ramfile ) != 1 )
							goto error;
					}
					else
					{
						if( fwrite( &fc->cache_buffer, filesize - fc->file_size, 1, fc->f ) != 1 )
							goto error;
					}

					fc->file_size += (filesize - fc->file_size);
				}
				else
				{
					if( fc->ramfile >= 0 )
					{
						if( fwrite_ramfile( &fc->cache_buffer, FILE_CACHE_SIZE, 1, fc->ramfile ) != 1 )
							goto error;
					}
					else
					{
						if( fwrite( &fc->cache_buffer, FILE_CACHE_SIZE, 1, fc->f ) != 1 )
							goto error;
					}

					fc->file_size += FILE_CACHE_SIZE;
				}
			}

			if( fc->ramfile >= 0 )
				fclose_ramfile(fc->ramfile);
			else
				fclose(fc->f);

			fc->current_offset = 0;

			if( fc->ramfile >= 0 )
			{
				fc->f = fopen_ramfile(fc->ramfile,"r+b");
				if(fc->f)
				{
					if(fseek_ramfile(fc->ramfile,fc->current_offset,SEEK_END))
						goto error;

					fc->file_size = ftell_ramfile(fc->ramfile);

					if(fseek_ramfile(fc->ramfile,fc->current_offset,SEEK_SET))
						goto error;

					if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
						fc->cur_page_size = ( fc->file_size - fc->current_offset);
					else
						fc->cur_page_size = FILE_CACHE_SIZE;

					if( fread_ramfile(&fc->cache_buffer,fc->cur_page_size,1,fc->ramfile) != 1 )
						goto error;

					if(fseek_ramfile(fc->ramfile,fc->current_offset,SEEK_SET))
						goto error;
				}
				else
				{
					goto error;
				}
			}
			else
			{
				fc->f = fopen(path,"r+b");
				if(fc->f)
				{
					if(fseek(fc->f,fc->current_offset,SEEK_END))
						goto error;

					fc->file_size = ftell(fc->f);

					if(fseek(fc->f,fc->current_offset,SEEK_SET))
						goto error;

					if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
						fc->cur_page_size = ( fc->file_size - fc->current_offset);
					else
						fc->cur_page_size = FILE_CACHE_SIZE;

					if( fread(&fc->cache_buffer,fc->cur_page_size,1,fc->f) != 1 )
						goto error;

					if(fseek(fc->f,fc->current_offset,SEEK_SET))
						goto error;
				}
				else
				{
					goto error;
				}
			}

			return 0;
		}
	}

	return -1;

error:
	if(fc->f)
		fclose(fc->f);

	fc->f = 0;

	return -1;
}

unsigned char get_byte(file_cache * fc,int offset, int * success)
{
	unsigned char byte;
	int ret;

	byte = fc->fill_val;
	ret = 1;

	if(fc)
	{
		if(offset < fc->file_size)
		{
			if( ( offset >= fc->current_offset ) &&
				( offset <  (fc->current_offset + FILE_CACHE_SIZE) ) )
			{
				byte = fc->cache_buffer[ offset - fc->current_offset ];
			}
			else
			{
				if( fc->dirty )
				{
					if( fc->ramfile >= 0 )
					{
						if( fseek_ramfile( fc->ramfile, fc->current_offset, SEEK_SET ) )
							goto error;

						if( fwrite_ramfile( &fc->cache_buffer, fc->cur_page_size, 1, fc->ramfile ) != 1 )
							goto error;
					}
					else
					{
						if( fseek(fc->f, fc->current_offset, SEEK_SET) )
							goto error;

						if( fwrite( &fc->cache_buffer, fc->cur_page_size, 1, fc->f ) != 1 )
							goto error;
					}

					fc->dirty = 0;
				}

				fc->current_offset = (offset & ~(FILE_CACHE_SIZE-1));

				if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
					fc->cur_page_size = ( fc->file_size - fc->current_offset );
				else
					fc->cur_page_size = FILE_CACHE_SIZE;

				if( fc->ramfile >= 0 )
					fseek_ramfile(fc->ramfile, fc->current_offset,SEEK_SET);
				else
					fseek(fc->f, fc->current_offset,SEEK_SET);

				memset(&fc->cache_buffer,fc->fill_val,FILE_CACHE_SIZE);

				if( fc->ramfile >= 0 )
				{
					if(fc->current_offset + FILE_CACHE_SIZE < fc->file_size)
						ret = fread_ramfile(&fc->cache_buffer,FILE_CACHE_SIZE,1,fc->ramfile);
					else
						ret = fread_ramfile(&fc->cache_buffer,fc->file_size - fc->current_offset,1,fc->ramfile);
				}
				else
				{
					if(fc->current_offset + FILE_CACHE_SIZE < fc->file_size)
						ret = fread(&fc->cache_buffer,FILE_CACHE_SIZE,1,fc->f);
					else
						ret = fread(&fc->cache_buffer,fc->file_size - fc->current_offset,1,fc->f);
				}

				byte = fc->cache_buffer[ offset - fc->current_offset ];
			}
		}
	}

	if(success)
	{
		*success = ret;
	}

	return byte;

error:

	return 0;
}

int set_byte(file_cache * fc,unsigned int offset, unsigned char byte)
{
	if(fc)
	{
		if(offset < fc->file_size)
		{
			if( ( offset >= fc->current_offset ) &&
				( offset <  (fc->current_offset + FILE_CACHE_SIZE) ) )
			{
				fc->cache_buffer[ offset - fc->current_offset ] = byte;
				fc->dirty = 1;
			}
			else
			{
				if( fc->dirty )
				{
					if( fc->ramfile >= 0 )
					{
						if( fseek_ramfile( fc->ramfile, fc->current_offset, SEEK_SET ) )
							goto error;

						if( fwrite_ramfile( &fc->cache_buffer, fc->cur_page_size, 1, fc->ramfile ) != 1 )
							goto error;
					}
					else
					{
						if( fseek(fc->f, fc->current_offset, SEEK_SET) )
							goto error;

						if( fwrite( &fc->cache_buffer, fc->cur_page_size, 1, fc->f ) != 1 )
							goto error;
					}

					fc->dirty = 0;
				}

				fc->current_offset = (offset & ~(FILE_CACHE_SIZE-1));

				if(fc->current_offset + FILE_CACHE_SIZE > fc->file_size)
					fc->cur_page_size = ( fc->file_size - fc->current_offset );
				else
					fc->cur_page_size = FILE_CACHE_SIZE;

				if( fc->ramfile >= 0 )
				{
					if(fseek_ramfile(fc->ramfile, fc->current_offset,SEEK_SET))
						goto error;

					memset(&fc->cache_buffer,fc->fill_val,FILE_CACHE_SIZE);

					fread_ramfile(&fc->cache_buffer,fc->cur_page_size,1,fc->ramfile);
				}
				else
				{
					if(fseek(fc->f, fc->current_offset,SEEK_SET))
						goto error;

					memset(&fc->cache_buffer,fc->fill_val,FILE_CACHE_SIZE);

					if( fread(&fc->cache_buffer,fc->cur_page_size,1,fc->f) != 1 )
					{
						// End of file ?
					}
				}

				fc->cache_buffer[ offset - fc->current_offset ] = byte;

				fc->dirty = 1;
			}
		}
		else
		{
			while( fc->file_size <= offset )
			{
				fc->file_size++;
				if( fc->cur_page_size < FILE_CACHE_SIZE )
					fc->cur_page_size++;

				set_byte(fc,fc->file_size - 1, 0x00);
			}

			set_byte(fc,offset, byte);
		}
	}

	return 1;

error:
	return 0;
}

int write_buf(file_cache * fc,unsigned int offset, void * buf, int size)
{
	int i,ret;

	i = 0;

	while ( i < size )
	{
		ret = set_byte( fc, offset, ((unsigned char*)buf)[i] );

		if( ret <= 0 )
		{
			return ret;
		}

		offset++;
		i++;
	}

	return size;
}

int read_buf(file_cache * fc,unsigned int offset, void * buf, int size)
{
	int i,ret;

	i = 0;
	while ( i < size )
	{
		((unsigned char *)buf)[i] = get_byte( fc, offset, &ret);

		if( ret <= 0 )
		{
			return ret;
		}

		offset++;
		i++;
	}

	return size;
}


int16_t get_short(file_cache * fc,int offset, int * success)
{
	uint16_t val;

	val = 0;

	val = get_byte(fc, offset++, success);
	val |= (((unsigned short)get_byte(fc, offset, success))<<8);

	return (int16_t)(val);
}

int set_ushort(file_cache * fc,unsigned int offset, uint16_t data)
{
	set_byte(fc,offset, data & 0xFF);
	return set_byte(fc,offset+1, (data>>8) & 0xFF);
}

int set_ulong(file_cache * fc,unsigned int offset, uint32_t data)
{
	set_ushort(fc,offset, data & 0xFFFF);
	return set_ushort(fc,offset+2, (data>>16) & 0xFFFF);
}

uint16_t get_ushort(file_cache * fc,int offset, int * success)
{
	uint16_t val;

	val = 0;

	val = get_byte(fc, offset++, success);
	val |= (((unsigned short)get_byte(fc, offset, success))<<8);

	return val;
}

uint32_t get_ulong(file_cache * fc,int offset, int * success)
{
	uint32_t val;

	val = 0;

	val = get_byte(fc, offset++, success);
	val |= (((unsigned short)get_byte(fc, offset++, success))<<8);
	val |= (((unsigned short)get_byte(fc, offset++, success))<<16);
	val |= (((unsigned short)get_byte(fc, offset++, success))<<24);

	return val;
}

int32_t get_long(file_cache * fc,int offset, int * success)
{
	uint32_t val;

	val = get_ulong(fc, offset, success);

	return (int32_t)(val);
}

float get_float( file_cache * fc,int offset, int * success)
{
	uint32_t val;
	float fval;

	val = get_ulong(fc, offset, success);
	memcpy((void*)&fval,(void*)&val,4);

	return fval;
}

double get_double( file_cache * fc,int offset, int * success)
{
	int i;
	uint8_t val[8];
	double dval;

	for(i=0;i<8;i++)
		val[i] = get_byte(fc, offset + i, success);


	memcpy((void*)&dval,(void*)&val,8);

	return dval;
}

void close_file(file_cache * fc)
{
	if(fc)
	{
		if(fc->f)
		{
			if( fc->dirty )
			{
				if( fc->ramfile >= 0 )
				{
					if(fseek_ramfile( fc->ramfile, fc->current_offset, SEEK_SET))
						goto error;

					if( fwrite_ramfile( &fc->cache_buffer, fc->cur_page_size, 1, fc->ramfile ) != 1 )
						goto error;
				}
				else
				{
					if( fseek(fc->f, fc->current_offset, SEEK_SET) )
						goto error;

					if( fwrite( &fc->cache_buffer, fc->cur_page_size, 1, fc->f ) != 1 )
						goto error;
				}

				fc->dirty = 0;
			}

			if( fc->ramfile >= 0 )
			{
				fclose_ramfile(fc->ramfile);
			}
			else
			{
				fclose(fc->f);
			}

			fc->f = NULL;
		}
	}

	return;

error:
	if( fc->ramfile >= 0 )
	{
		fclose_ramfile(fc->ramfile);
	}
	else
	{
		fclose(fc->f);
	}

	return;
}
