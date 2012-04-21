
typedef struct s_trackdisplay_
{
	int xsize,ysize;
	int x_us,y_us;
	int x_start_us;
	unsigned long * framebuffer;
}s_trackdisplay;

s_trackdisplay * hxcfe_td_init(HXCFLOPPYEMULATOR* floppycontext,unsigned long xsize,unsigned long ysize,unsigned long x_us,unsigned long y_us,unsigned long x_start_us);
void hxcfe_td_draw_track(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk,int track,int side);
void hxcfe_td_deinit(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td);
