void floppy_streamer_window_bt_read(Fl_Button* bt, void*);
void streamer_tick_infos(void *v);

void disk_streamer_window_callback(Fl_Widget *o, void *v);
void disk_streamer_window_bt_edit_callback(Fl_Widget *o, void *v);

typedef struct _streamthread
{
	floppy_streamer_window *window;
	void * guicontext;
}streamthread;

int StreamerThreadProc(void* floppycontext,void* context);
int StreamerThreadRxStatusProc(void* floppycontext,void* context);
int StreamerThreadRxDataProc(void* floppycontext,void* context);
