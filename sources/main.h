
typedef struct s_gui_context_
{
	char bufferfilename[4096];
	char buffertext[4096];

	char * logfile;

	int loadstatus;

	unsigned char driveid;
	unsigned char autoselectmode;
	unsigned char doublestep;
	unsigned char twistedcable;
	int interfacemode;

	HXCFLOPPYEMULATOR * hxcfe;
	FLOPPY * loadedfloppy;
	USBHXCFE * usbhxcfe;

	int backlight_tmr;
	int standby_tmr;
	int step_sound;
	int ui_sound;
	int	lcd_scroll;

	unsigned int txtindex;

	int xsize,ysize;
	unsigned char * mapfloppybuffer;

	s_trackdisplay * td;
	int updatefloppyinfos;

}s_gui_context;
