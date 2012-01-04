#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/filename.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>

#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_BMP_Image.H>

#include <FL/fl_ask.H>


extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
}

#include "gui_struct.h"
#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "main_gui.h"

HXCFLOPPYEMULATOR * flopemu;
FLOPPY * thefloppydisk;
HWINTERFACE * hwif;

guicontext * gui_context;

	track_type track_type_list[]=
	{
		{ FM_TRACK_TYPE,"FM",ISOFORMAT_SD},
		{ FMIBM_TRACK_TYPE,"IBM FM",IBMFORMAT_SD},
		{ MFM_TRACK_TYPE,"MFM",ISOFORMAT_DD},
		{ MFMIBM_TRACK_TYPE,"IBM MFM",IBMFORMAT_DD},
		//{ GCR_TRACK_TYPE,"GCR"},
		{ -1,"",0}
	};


int main(int argc, char **argv) {

	gui_context=(guicontext *)malloc(sizeof(guicontext));
	if(gui_context)
	{
		memset(gui_context,0,sizeof(guicontext));

		thefloppydisk=0;
		flopemu=hxcfe_init();

		hwif=(HWINTERFACE *)malloc(sizeof(HWINTERFACE));
		memset(hwif,0,sizeof(HWINTERFACE));
		HW_CPLDFloppyEmulator_init(flopemu,hwif);


		Main_Window * mw;
		mw=new Main_Window();
	}
}
