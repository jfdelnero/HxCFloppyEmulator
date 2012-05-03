// generated by Fast Light User Interface Designer (fluid) version 1.0110

#ifndef floppy_infos_window_h
#define floppy_infos_window_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Value_Slider.H>
extern void disk_infos_window_callback(Fl_Value_Slider*, void*);
#include <FL/Fl_Output.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
extern void floppy_infos_ok(Fl_Button*, void*);
#include <FL/Fl_Round_Button.H>
extern void disk_infos_window_callback(Fl_Round_Button*, void*);

class floppy_infos_window {
public:
  floppy_infos_window();
  Fl_Double_Window *window;
  Fl_Value_Slider *track_number_slide;
  Fl_Value_Slider *side_number_slide;
  Fl_Output *x_pos;
  Fl_Output *y_pos;
  Fl_Output *global_status;
  Fl_Text_Display *object_txt;
  Fl_Group *floppy_map_disp;
  Fl_Value_Slider *x_time;
  Fl_Value_Slider *y_time;
  Fl_Value_Slider *x_offset;
  Fl_Round_Button *track_view_bt;
  Fl_Round_Button *disc_view_bt;
	Fl_Text_Buffer* buf;
	Fl_Text_Display * txt_displ;

};
#endif
