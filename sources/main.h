
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

	HXCFE * hxcfe;
	HXCFE_FLOPPY * loadedfloppy;
	USBHXCFE * usbhxcfe;

	HXCFE_FLOPPY * trackviewerfloppy;
	int trackviewerfloppy_updateneeded;

	int backlight_tmr;
	int standby_tmr;
	int step_sound;
	int ui_sound;
	int	lcd_scroll;

	unsigned int txtindex;

	int xsize,ysize;
	unsigned char * mapfloppybuffer;

	HXCFE_TD * td;
	unsigned char * flayoutframebuffer;
	int updatefloppyinfos;
	int graphupdate;
	int pointer_mode;
	unsigned char * copybuffer;

	int updatefloppyfs;

	char last_loaded_image_path[4096];
	int loaded_img_modified;

	char xml_file_path[1024];

	void * main_window;

	float loadingprogess;

	int exporting;
	int loading;

	int exit;

}s_gui_context;
