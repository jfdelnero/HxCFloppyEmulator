#include <FL/Fl_Box.H>

class Fl_DND_Box : public Fl_Box
{
	public:

		static void callback_deferred(void *v);
		Fl_DND_Box(int X, int Y, int W, int H, const char *L);

		~Fl_DND_Box();
		int event();
		const char* event_text();
		int event_length();
		int handle(int e);

	protected:

		// The event which caused Fl_DND_Box to execute its callback
		int evt;
		char *evt_txt;
		int evt_len;
};

extern void dnd_bc_cb(Fl_Widget *o, void *v);
