
typedef struct batchconverterparams_
{
	HXCFE * flopemu;
	//HWND windowshwd;
	batch_converter_window *windowshwd;
	char sourcedir[1024];
	char destdir[1024];
	char **filelist;
	int fileformat;
	unsigned long numberoffileconverted;
	int abort;
	int rawfilemode;
}batchconverterparams;

void dnd_bc_cb(Fl_Widget *o, void *v);
