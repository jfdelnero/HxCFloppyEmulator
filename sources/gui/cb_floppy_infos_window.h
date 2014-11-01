void floppy_infos_window_bt_read(Fl_Button* bt, void*);
void tick_infos(void *v);

void disk_infos_window_callback(Fl_Widget *o, void *v);

typedef struct _infothread
{
	floppy_infos_window *window;
	void * guicontext;
}infothread;

int InfosThreadProc(void* floppycontext,void* context);
