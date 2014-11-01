// generated by Fast Light User Interface Designer (fluid) version 1.0300

#include "rawfile_loader_window.h"

void rawfile_loader_window::cb_Close_i(Fl_Button* o, void*) {
  ((Fl_Window*)(o->parent()))->hide();
}
void rawfile_loader_window::cb_Close(Fl_Button* o, void* v) {
  ((rawfile_loader_window*)(o->parent()->user_data()))->cb_Close_i(o,v);
}

rawfile_loader_window::rawfile_loader_window() {
  { window = new Fl_Double_Window(458, 351, "RAW File format configuration");
    window->user_data((void*)(this));
    { chk_reversesides = new Fl_Check_Button(333, 58, 25, 25, "Reverse side");
      chk_reversesides->down_box(FL_DOWN_BOX);
      chk_reversesides->labelsize(12);
      chk_reversesides->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
    } // Fl_Check_Button* chk_reversesides
    { chk_intersidesectornum = new Fl_Check_Button(233, 96, 115, 25, "Inter side sector numbering");
      chk_intersidesectornum->down_box(FL_DOWN_BOX);
      chk_intersidesectornum->labelsize(12);
      chk_intersidesectornum->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      chk_intersidesectornum->align(Fl_Align(132|FL_ALIGN_INSIDE));
    } // Fl_Check_Button* chk_intersidesectornum
    { chk_side0track_first = new Fl_Check_Button(333, 17, 125, 25, "Tracks of a side grouped in the file");
      chk_side0track_first->down_box(FL_DOWN_BOX);
      chk_side0track_first->labelsize(12);
      chk_side0track_first->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      chk_side0track_first->align(Fl_Align(132|FL_ALIGN_INSIDE));
    } // Fl_Check_Button* chk_side0track_first
    { chk_autogap3 = new Fl_Check_Button(130, 210, 25, 25, "Auto GAP3");
      chk_autogap3->down_box(FL_DOWN_BOX);
      chk_autogap3->labelsize(12);
      chk_autogap3->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
    } // Fl_Check_Button* chk_autogap3
    { numin_pregap = new Fl_Value_Input(230, 216, 95, 20, "PRE-GAP length :");
      numin_pregap->labelsize(12);
      numin_pregap->textsize(12);
      numin_pregap->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      numin_pregap->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* numin_pregap
    { numin_formatvalue = new Fl_Value_Input(335, 174, 45, 20, "Format value");
      numin_formatvalue->labelsize(12);
      numin_formatvalue->textsize(12);
      numin_formatvalue->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      numin_formatvalue->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* numin_formatvalue
    { numin_interleave = new Fl_Value_Input(130, 136, 95, 20, "Interleave :");
      numin_interleave->labelsize(12);
      numin_interleave->textsize(12);
      numin_interleave->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      numin_interleave->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* numin_interleave
    { numin_skew = new Fl_Value_Input(230, 136, 95, 20, "Skew :");
      numin_skew->labelsize(12);
      numin_skew->textsize(12);
      numin_skew->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      numin_skew->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* numin_skew
    { chk_sidebasedskew = new Fl_Check_Button(333, 134, 25, 25, "Side based");
      chk_sidebasedskew->down_box(FL_DOWN_BOX);
      chk_sidebasedskew->labelsize(12);
      chk_sidebasedskew->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
    } // Fl_Check_Button* chk_sidebasedskew
    { strout_totalsector = new Fl_Output(130, 174, 95, 20, "Total Sector:");
      strout_totalsector->labelsize(12);
      strout_totalsector->textsize(12);
      strout_totalsector->user_data((void*)(this));
      strout_totalsector->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Output* strout_totalsector
    { strout_totalsize = new Fl_Output(230, 174, 95, 20, "Total Size:");
      strout_totalsize->labelsize(12);
      strout_totalsize->textsize(12);
      strout_totalsize->user_data((void*)(this));
      strout_totalsize->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Output* strout_totalsize
    { innum_nbtrack = new Fl_Value_Input(130, 20, 95, 20, "Number of Track :");
      innum_nbtrack->labelsize(12);
      innum_nbtrack->textsize(12);
      innum_nbtrack->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      innum_nbtrack->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* innum_nbtrack
    { innum_sectoridstart = new Fl_Value_Input(130, 99, 95, 20, "Sector ID start :");
      innum_sectoridstart->labelsize(12);
      innum_sectoridstart->textsize(12);
      innum_sectoridstart->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      innum_sectoridstart->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* innum_sectoridstart
    { innum_sectorpertrack = new Fl_Value_Input(130, 61, 95, 20, "Sector per track :");
      innum_sectorpertrack->labelsize(12);
      innum_sectorpertrack->textsize(12);
      innum_sectorpertrack->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      innum_sectorpertrack->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* innum_sectorpertrack
    { innum_rpm = new Fl_Value_Input(10, 99, 105, 20, "RPM :");
      innum_rpm->labelsize(12);
      innum_rpm->textsize(12);
      innum_rpm->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      innum_rpm->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* innum_rpm
    { innum_bitrate = new Fl_Value_Input(10, 60, 105, 20, "Bitrate :");
      innum_bitrate->labelsize(12);
      innum_bitrate->textsize(12);
      innum_bitrate->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      innum_bitrate->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* innum_bitrate
    { Fl_Button* o = new Fl_Button(172, 270, 95, 30, "Load RAW file");
      o->labelsize(12);
      o->callback((Fl_Callback*)raw_loader_window_bt_loadrawfile, (void*)(this));
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(274, 270, 95, 30, "Create Empty Floppy");
      o->labelsize(12);
      o->callback((Fl_Callback*)raw_loader_window_bt_createemptyfloppy, (void*)(this));
      o->align(Fl_Align(FL_ALIGN_WRAP));
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(10, 280, 70, 20, "Save config");
      o->labelsize(12);
      o->callback((Fl_Callback*)raw_loader_window_bt_savecfg, (void*)(this));
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(89, 280, 70, 20, "Load config");
      o->labelsize(12);
      o->callback((Fl_Callback*)raw_loader_window_bt_loadcfg, (void*)(this));
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(377, 270, 70, 30, "Close");
      o->labelsize(12);
      o->callback((Fl_Callback*)cb_Close);
    } // Fl_Button* o
    { choice_sectorsize = new Fl_Choice(230, 61, 95, 20, "Sector size :");
      choice_sectorsize->down_box(FL_BORDER_BOX);
      choice_sectorsize->labelsize(12);
      choice_sectorsize->textsize(11);
      choice_sectorsize->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      choice_sectorsize->align(Fl_Align(FL_ALIGN_TOP_LEFT));
      choice_sectorsize->when(FL_WHEN_CHANGED);
    } // Fl_Choice* choice_sectorsize
    { choice_tracktype = new Fl_Choice(10, 20, 105, 20, "Track type :");
      choice_tracktype->down_box(FL_BORDER_BOX);
      choice_tracktype->labelsize(12);
      choice_tracktype->textsize(11);
      choice_tracktype->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      choice_tracktype->align(Fl_Align(FL_ALIGN_TOP_LEFT));
      choice_tracktype->when(FL_WHEN_CHANGED);
    } // Fl_Choice* choice_tracktype
    { numin_gap3 = new Fl_Value_Input(10, 215, 105, 20, "GAP3 length :");
      numin_gap3->labelsize(12);
      numin_gap3->textsize(12);
      numin_gap3->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      numin_gap3->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    } // Fl_Value_Input* numin_gap3
    { choice_numberofside = new Fl_Choice(230, 20, 95, 20, "Number of side :");
      choice_numberofside->down_box(FL_BORDER_BOX);
      choice_numberofside->labelsize(12);
      choice_numberofside->textsize(11);
      choice_numberofside->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      choice_numberofside->align(Fl_Align(FL_ALIGN_TOP_LEFT));
      choice_numberofside->when(FL_WHEN_CHANGED);
    } // Fl_Choice* choice_numberofside
    { choice_disklayout = new Fl_Choice(10, 255, 150, 20, "Predefined Disk Layout:");
      choice_disklayout->down_box(FL_BORDER_BOX);
      choice_disklayout->labelsize(12);
      choice_disklayout->textsize(11);
      choice_disklayout->callback((Fl_Callback*)raw_loader_window_datachanged, (void*)(this));
      choice_disklayout->align(Fl_Align(FL_ALIGN_TOP_LEFT));
      choice_disklayout->when(FL_WHEN_CHANGED);
    } // Fl_Choice* choice_disklayout
    { hlptxt = new Fl_Output(5, 277, 442, 72);
      hlptxt->box(FL_NO_BOX);
      hlptxt->align(Fl_Align(37));
      hlptxt->deactivate();
    } // Fl_Output* hlptxt
    window->end();
  } // Fl_Double_Window* window
}
