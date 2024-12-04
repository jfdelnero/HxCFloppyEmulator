#define SINCOS_TABLE_SIZE 1024
#define SINCOS_TABLE_MAX  1024

typedef struct uintro_context_
{
	unsigned short xsize;
	unsigned short ysize;

	unsigned short sprite_xsize;
	unsigned short sprite_ysize;

	unsigned int * framebuffer;
	unsigned int * background;
	unsigned int * blurbuffer;

	int f1,f2;
	int col_f1,col_f1s;
	int col_f2,col_f2s;
	int col_f3,col_f3s;

	int sprt_f1,sprt_f2;

	unsigned long tick;
	unsigned char part;

	void * modctx;

	int sincos_table[SINCOS_TABLE_SIZE];
}uintro_context;


uintro_context * uintro_init(unsigned short xsize,unsigned short ysize);
void uintro_reset(uintro_context * ui_context);
void uintro_getnextframe(uintro_context * democontext);
void uintro_getnext_soundsample(uintro_context * democontext,short* buffer,int size);
void uintro_deinit(uintro_context * democontext);
