
typedef struct s_trackdisplay_
{
	unsigned long xsize,ysize;
	unsigned long x_us,y_us;
	unsigned long x_start_us;
	unsigned long * framebuffer;
}s_trackdisplay;

s_trackdisplay * td_init(unsigned long xsize,unsigned long ysize,unsigned long x_us,unsigned long y_us,unsigned long x_start_us);
void td_draw_track(s_trackdisplay *td,SIDE * currentside);
void td_deinit(s_trackdisplay *td);
