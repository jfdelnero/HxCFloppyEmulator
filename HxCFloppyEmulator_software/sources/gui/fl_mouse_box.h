#include <FL/Fl_Box.H>

class Fl_Mouse_Box : public Fl_Box
{
    public:

        static void callback_deferred(void *v)
        {
            Fl_Mouse_Box *w = (Fl_Mouse_Box*)v;

            w->do_callback();
        }

        Fl_Mouse_Box(int X, int Y, int W, int H, const char *L = 0)
                : Fl_Box(X,Y,W,H,L), evt(FL_NO_EVENT), evt_txt(0), evt_len(0)
        {
            labeltype(FL_NO_LABEL);
            box(FL_NO_BOX);
            clear_visible_focus();
        }

        virtual ~Fl_Mouse_Box()
        {
            delete [] evt_txt;
        }

        int event()
        {
            return evt;
        }

        const char* event_text()
        {
            return evt_txt;
        }

        int event_length()
        {
            return evt_len;
        }

        int handle(int e)
        {
            switch(e)
            {
                case FL_ENTER:
                case FL_LEAVE:
                    evt = e;
                    return 1;


				case FL_PUSH:
                case FL_MOVE:
                    evt = e;

                    // make a copy of the DND payload
                    //evt_len = Fl::event_length();

                    //delete [] evt_txt;

                    //evt_txt = new char[evt_len+1];
                    //strcpy(evt_txt, Fl::event_text());

                    // If there is a callback registered, call it.
                    // The callback must access Fl::event_text() to
                    // get the string or file path that was dropped.
                    // Note that do_callback() is not called directly.
                    // Instead it will be executed by the FLTK main-loop
                    // once we have finished handling the DND event.
                    // This allows caller to popup a window or change widget focus.
                    if(callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
                        Fl::add_timeout(0.0, Fl_Mouse_Box::callback_deferred, (void*)this);
                    return 1;
            }

            return Fl_Box::handle(e);
        }

    protected:
        // The event which caused Fl_DND_Box to execute its callback
        int evt;

        char *evt_txt;
        int evt_len;
};

extern void mouse_di_cb(Fl_Widget *o, void *v);

