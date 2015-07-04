// generated by Fast Light User Interface Designer (fluid) version 1.0300

#ifndef edittool_window_h
#define edittool_window_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
extern void edittool_window_bt_copy_callback(Fl_Button*, void*);
extern void edittool_window_bt_startpoint_callback(Fl_Button*, void*);
extern void edittool_window_bt_endpoint_callback(Fl_Button*, void*);
extern void edittool_window_bt_paste_callback(Fl_Button*, void*);
extern void edittool_window_bt_fill_callback(Fl_Button*, void*);
#include <FL/Fl_Int_Input.H>
extern void edittool_window_bt_insert_callback(Fl_Button*, void*);
extern void edittool_window_bt_delete_callback(Fl_Button*, void*);
extern void edittool_window_bt_flakeypattern_callback(Fl_Button*, void*);
extern void edittool_window_bt_setbitrate_callback(Fl_Button*, void*);
#include <FL/Fl_Light_Button.H>
extern void edittool_window_bt_shift_callback(Fl_Button*, void*);
extern void edittool_window_bt_erase_side1_callback(Fl_Button*, void*);
extern void edittool_window_bt_erase_side0_callback(Fl_Button*, void*);
extern void edittool_window_bt_setdiskbitrate_callback(Fl_Button*, void*);
extern void edittool_window_bt_setdiskrpm_callback(Fl_Button*, void*);
extern void edittool_window_bt_addtrack_callback(Fl_Button*, void*);
extern void edittool_window_bt_removetrack_callback(Fl_Button*, void*);
#include <FL/Fl_Box.H>
extern void edittool_window_bt_removeoddtracks_callback(Fl_Button*, void*);
extern void edittool_window_bt_shifttracks_callback(Fl_Button*, void*);
extern void edittool_window_bt_swapsides_callback(Fl_Button*, void*);

class trackedittool_window {
public:
  trackedittool_window();
  Fl_Double_Window *window;
  Fl_Button *bt_copy;
  Fl_Button *bt_startpoint;
  Fl_Button *bt_endpoint;
  Fl_Button *bt_paste;
  Fl_Button *bt_fill;
  Fl_Int_Input *edit_startpoint;
  Fl_Int_Input *edit_endpoint;
  Fl_Button *bt_insert;
  Fl_Button *bt_delete;
  Fl_Button *bt_setflakey;
  Fl_Button *bt_setbitrate;
  Fl_Light_Button *bt_directedition;
  Fl_Int_Input *edit_bitrate;
  Fl_Int_Input *edit_editbuffer;
  Fl_Button *bt_shift;
  Fl_Int_Input *edit_shiftbit;
  Fl_Int_Input *edit_fillflakey;
  Fl_Button *bt_erase1;
  Fl_Button *bt_erase0;
  Fl_Button *bt_setdiskbitrate;
  Fl_Int_Input *edit_bitrate2;
  Fl_Button *bt_rpm;
  Fl_Button *bt_addtrack;
  Fl_Button *bt_removetrack;
  Fl_Int_Input *edit_rpm;
  Fl_Button *bt_removeoddtracks;
  Fl_Button *bt_shifttracks;
  Fl_Int_Input *edit_shiftbittracks;
  Fl_Button *bt_swapsides;
};
#endif