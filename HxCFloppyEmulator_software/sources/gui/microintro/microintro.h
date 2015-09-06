
typedef struct uintro_context_
{
	unsigned short xsize;
	unsigned short ysize;

	unsigned short sprite_xsize;
	unsigned short sprite_ysize;

	unsigned int * framebuffer;
	unsigned int * background;
	unsigned int * blurbuffer;

	float f1,f2;

	float col_f1,col_f2,col_f3;
	float col_f1s,col_f2s,col_f3s;

	float sprt_f1,sprt_f2;

	unsigned long tick;
	unsigned char part;

	void * modctx;
}uintro_context;


uintro_context * uintro_init(unsigned short xsize,unsigned short ysize);
void uintro_reset(uintro_context * ui_context);
void uintro_getnextframe(uintro_context * democontext);
void uintro_getnext_soundsample(uintro_context * democontext,unsigned short* buffer,int size);
void uintro_deinit(uintro_context * democontext);
