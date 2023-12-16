#include "fl_includes.h"

#include <FL/Fl_Box.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fl_dnd_box.h"

void Fl_DND_Box::callback_deferred(void *v)
{
	Fl_DND_Box *w = (Fl_DND_Box*)v;

	w->do_callback();
}

Fl_DND_Box::Fl_DND_Box(int X, int Y, int W, int H, const char *L = 0)
	: Fl_Box(X,Y,W,H,L)
{
	evt_txt = 0;
	evt_len = 0;
	evt = FL_NO_EVENT;
	labeltype(FL_NO_LABEL);
	box(FL_NO_BOX);
	clear_visible_focus();
}

Fl_DND_Box::~Fl_DND_Box()
{
	free(evt_txt);
	evt_txt = NULL;
}

int Fl_DND_Box::event()
{
	return evt;
}

const char* Fl_DND_Box::event_text()
{
	return evt_txt;
}

int Fl_DND_Box::event_length()
{
	return evt_len;
}

int Fl_DND_Box::handle(int e)
{
	switch(e)
	{
		case FL_DND_ENTER:
		case FL_DND_RELEASE:
		case FL_DND_LEAVE:
		case FL_DND_DRAG:
			evt = e;
			return 1;

		case FL_PASTE:
			evt = e;

			// make a copy of the DND payload
			evt_len = Fl::event_length();

			free(evt_txt);

			evt_txt = (char*)calloc( 1, evt_len+1 );
			if(evt_txt)
				strncpy(evt_txt, Fl::event_text(),evt_len);

			// If there is a callback registered, call it.
			// The callback must access Fl::event_text() to
			// get the string or file path that was dropped.
			// Note that do_callback() is not called directly.
			// Instead it will be executed by the FLTK main-loop
			// once we have finished handling the DND event.
			// This allows caller to popup a window or change widget focus.
			if(callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
				Fl::add_timeout(0.0, Fl_DND_Box::callback_deferred, (void*)this);
			return 1;
	}

	return Fl_Box::handle(e);
}
