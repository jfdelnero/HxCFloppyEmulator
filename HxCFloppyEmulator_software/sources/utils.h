
char * URIfilepathparser(char * URI,int size);

#ifndef BMAPTYPEDEF
#define BMAPTYPEDEF

typedef  struct _bmaptype
{
   int type;
   int Xsize;
   int Ysize;
   int size;
   int csize;
   unsigned char * data;
   unsigned char * unpacked_data;
}bmaptype;

#endif

void splash_sprite(bmaptype * bmp,unsigned char * dest_buffer, int xsize, int ysize, int xpos, int ypos);
