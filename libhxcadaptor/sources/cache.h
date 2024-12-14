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
// File : cache.h
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

#define FILE_CACHE_SIZE (64*1024)

typedef struct file_cache_
{
	void * f;
	int  ramfile;
	int  current_offset;
	int  cur_page_size;
	int  file_size;
	unsigned char cache_buffer[FILE_CACHE_SIZE];
	unsigned char fill_val;
	int dirty;
}file_cache;

int open_file(file_cache * fc, char* path, int filesize,unsigned char fill);
unsigned char get_byte(file_cache * fc,int offset, int * success);
int16_t get_short(file_cache * fc,int offset, int * success);
uint16_t get_ushort(file_cache * fc,int offset, int * success);
int32_t get_long(file_cache * fc,int offset, int * success);
uint32_t get_ulong(file_cache * fc,int offset, int * success);
float get_float( file_cache * fc,int offset, int * success);
double get_double( file_cache * fc,int offset, int * success);

int set_byte(file_cache * fc,unsigned int offset, unsigned char byte);
int set_ushort(file_cache * fc,unsigned int offset, uint16_t data);
int set_ulong(file_cache * fc,unsigned int offset, uint32_t data);

int read_buf(file_cache * fc,unsigned int offset, void * buf, int size);
int write_buf(file_cache * fc,unsigned int offset, void * buf, int size);

void close_file(file_cache * fc);

void init_ramfiles();
void erase_ramfile(int i);
void deinit_ramfiles();

