typedef struct uintro_context_
{
	unsigned short xsize;
	unsigned short ysize;


	unsigned short sprite_xsize;
	unsigned short sprite_ysize;

	unsigned long * framebuffer;

	float f1,f2;
}uintro_context;


uintro_context * uintro_init(unsigned short xsize,unsigned short ysize);
void uintro_getnextframe(uintro_context * democontext);
void uintro_deinit(uintro_context * democontext);
