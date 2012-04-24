
typedef struct main_button_list_
{
	Fl_Button * button;
	char * label;
	char * desc;
}main_button_list;


// Main class...
class Main_Window : public Fl_Window {
  
	Fl_Button *button_load;
	Fl_Button *button_loadrawimg;
	Fl_Button *button_batchconverter;
	Fl_Button *button_createfs;
	Fl_Button *button_export;
	Fl_Button *button_sdhxcfesettings;
	Fl_Button *button_usbhxcfesettings;
	Fl_Button *button_floppydump;
	
	Fl_Box*  o;
	Fl_Window *window;

	int evt;

    char *evt_txt;
    int evt_len;

  public:
	batch_converter_window *batchconv_window;
	filesystem_generator_window *fs_window;
	floppy_dump_window *fdump_window;
	floppy_infos_window *infos_window;
	rawfile_loader_window *rawloader_window;
	sdhxcfecfg_window * sdcfg_window;
	usbhxcfecfg_window * usbcfg_window;

	Log_box * log_box;
	About_box *about_window;

	int	xsize;
	int	ysize;
	int	xpos_size;
	int	ypos_size;
	int	window_active;
	unsigned int txtindex;
	Fl_File_Chooser		*fc_load;
	Fl_File_Chooser		*fc_save;

	Fl_Output* file_name_txt;
	Fl_Progress* track_pos;
	Fl_Output* track_pos_str;

	Fl_Text_Buffer* buf;

	//virtual handle(int event);
	Main_Window();
	~Main_Window();
};

void load_file_image_pb(Fl_Widget * widget, void * ptr);
void menu_clicked(Fl_Widget * w, void * fc_ptr);
void save_file_image(Fl_Widget * w, void * fc_ptr);
void format_choice_cb(Fl_Widget *, void *v);