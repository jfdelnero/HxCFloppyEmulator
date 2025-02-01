#define DEFAULT_TEXT_BUFFER_SIZE 4096

typedef struct s_gui_context_
{
	char bufferfilename[DEFAULT_TEXT_BUFFER_SIZE];
	char buffertext[DEFAULT_TEXT_BUFFER_SIZE];

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
	volatile int trackviewerfloppy_updateneeded;

	int backlight_tmr;
	int standby_tmr;
	int step_sound;
	int ui_sound;
	int lcd_scroll;

	unsigned int txtindex;

	int xsize,ysize;
	unsigned char * mapfloppybuffer;

	HXCFE_TD * td;
	unsigned char * flayoutframebuffer;
	volatile int updatefloppyinfos;
	volatile int updating;
	int pointer_mode;
	unsigned char * copybuffer;

	int updatefloppyfs;

	HXCFE_TD * td_stream;
	unsigned char * stream_frame_buffer;

	char last_loaded_image_path[DEFAULT_TEXT_BUFFER_SIZE];
	int loaded_img_modified;

	char xml_file_path[DEFAULT_TEXT_BUFFER_SIZE];

	void * main_window;

	float loadingprogess;

	int exporting;
	int loading;

	int exit;

	char pauline_ip_address[DEFAULT_TEXT_BUFFER_SIZE];

}s_gui_context;
