#pragma pack(1)
typedef struct bitmap_data_
{
	int xsize;
	int ysize;
	int nb_color;
	unsigned char * palette;
	unsigned long * data;
}bitmap_data;


typedef struct tagBITMAPFILEHEADER {
        unsigned short bfType;
        unsigned long  bfSize;
        unsigned short bfReserved1;
        unsigned short bfReserved2;
        unsigned long  bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  unsigned long   biSize;
  unsigned long   biWidth;
  unsigned long   biHeight;
  unsigned short  biPlanes;
  unsigned short  biBitCount;
  unsigned long   biCompression;
  unsigned long   biSizeImage;
  unsigned long   biXPelsPerMeter;
  unsigned long   biYPelsPerMeter;
  unsigned long   biClrUsed;
  unsigned long   biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved;
} RGBQUAD;

#pragma pack()

int bmp_load(char * file,bitmap_data * bdata);
int bmpRLE8b_write(char * file,bitmap_data * bdata);
int bmp24b_write(char * file,bitmap_data * bdata);
int bmp16b_write(char * file,bitmap_data * bdata);


