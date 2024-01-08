/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : gui_strings.cxx
// Contains: GUI texts
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "gui_strings.h"

string_entry strings_list[]=
{
	{ STR_COMMON_OK, "Ok" },
	{ STR_COMMON_CLOSE, "Close" },
#ifndef OEM_MODE
	{ STR_SDSETTINGSWINDOW_0001, "SDCard HxC Floppy Emulator settings" },
	{ STR_SDSETTINGSWINDOW_0002, "SD HxC Floppy Emulator Usage settings" },
#else
	{ STR_SDSETTINGSWINDOW_0001, "Floppy Emulator settings" },
	{ STR_SDSETTINGSWINDOW_0002, "Floppy Emulator Usage settings" },
#endif
	{ STR_SDSETTINGSWINDOW_0003, "User interface" },
	{ STR_SDSETTINGSWINDOW_0004, "UI Sound" },
	{ STR_SDSETTINGSWINDOW_0005, "Sound level" },
	{ STR_SDSETTINGSWINDOW_0006, "Head step sound" },
	{ STR_SDSETTINGSWINDOW_0007, "Track change sound level" },
	{ STR_SDSETTINGSWINDOW_0008, "Scroll text speed" },
	{ STR_SDSETTINGSWINDOW_0009, "Filename scrolling speed (used if the filename has more than 16 characters)" },
	{ STR_SDSETTINGSWINDOW_0010, "Standby" },
	{ STR_SDSETTINGSWINDOW_0011, "Device standby" },
	{ STR_SDSETTINGSWINDOW_0012, "Backlight standby" },
	{ STR_SDSETTINGSWINDOW_0013, "Load last loaded floppy" },
	{ STR_SDSETTINGSWINDOW_0014, "Disable drive selector" },
	{ STR_SDSETTINGSWINDOW_0015, "Force loading STARTUPA.HFE to drive A at power up" },
	{ STR_SDSETTINGSWINDOW_0016, "Force loading STARTUPB.HFE to drive B at power up" },
	{ STR_SDSETTINGSWINDOW_0017, "AUTO BOOT Mode / File Selector software startup" },
	{ STR_SDSETTINGSWINDOW_0018, "Enable auto boot mode" },
	{ STR_SDSETTINGSWINDOW_0019, "Force loading AUTOBOOT.HFE at power up" },
	{ STR_SDSETTINGSWINDOW_0020, "Enable index Pre increment" },
	{ STR_SDSETTINGSWINDOW_0021, "Indexed Mode" },
	{ STR_SDSETTINGSWINDOW_0022, "Enable indexed mode" },
	{ STR_SDSETTINGSWINDOW_0023, "HFE file interfacemode" },
	{ STR_SDSETTINGSWINDOW_0024, "Auto" },
	{ STR_SDSETTINGSWINDOW_0025, "Double Step" },
	{ STR_SDSETTINGSWINDOW_0026, "Mode" },
#ifndef OEM_MODE
	{ STR_SDSETTINGSWINDOW_0027, "SD HxC Floppy Emulator Interface setting" },
#else
	{ STR_SDSETTINGSWINDOW_0027, "Floppy Emulator Interface setting" },
#endif
	{ STR_SDSETTINGSWINDOW_0028, "Mode" },
	{ STR_SDSETTINGSWINDOW_0029, "Pin 2" },
	{ STR_SDSETTINGSWINDOW_0030, "Pin 34" },
	{ STR_SDSETTINGSWINDOW_0031, "2 drives emulation"},
	{ STR_SDSETTINGSWINDOW_0032, "Drive B input as Drives Motor ON"},
	{ STR_SDSETTINGSWINDOW_0033, "Drive A" },
	{ STR_SDSETTINGSWINDOW_0034, "Drive B" },
	{ STR_SDSETTINGSWINDOW_0035, "Load config file" },
	{ STR_SDSETTINGSWINDOW_0036, "Save config file" },

	{ STR_BATCHCONVERTWINDOW_0001, "Floppy Disk Images Batch converter" },
	{ STR_BATCHCONVERTWINDOW_0002, "Target file format :" },
	{ STR_BATCHCONVERTWINDOW_0003, "Source directory" },
	{ STR_BATCHCONVERTWINDOW_0004, "Target directory" },
	{ STR_BATCHCONVERTWINDOW_0005, "Select" },
	{ STR_BATCHCONVERTWINDOW_0006, "Select" },
	{ STR_BATCHCONVERTWINDOW_0007, "Convert" },
	{ STR_BATCHCONVERTWINDOW_0008, "Cancel" },
	{ STR_BATCHCONVERTWINDOW_0009, "Treat input files as RAW files" },

	{ STR_FSGENERATORWINDOW_0001, "DOS Floppy Disk File Browser" },
	{ STR_FSGENERATORWINDOW_0002, "Create Disk" },
	{ STR_FSGENERATORWINDOW_0004, "File system type :" },
	{ STR_FSGENERATORWINDOW_0005, "FS_Tree" },
	{ STR_FSGENERATORWINDOW_0006, "Delete" },
	{ STR_FSGENERATORWINDOW_0007, "Get Files" },
	{ STR_FSGENERATORWINDOW_0008, "Save/Export" },
	{ STR_FSGENERATORWINDOW_0009, "Load Image" },
	{ STR_FSGENERATORWINDOW_0010, "Disk Selector:" },

	{ STR_FLOPPYDUMPWINDOW_0001, "Floppy disk dump" },
	{ STR_FLOPPYDUMPWINDOW_0002, "Dump setting" },
	{ STR_FLOPPYDUMPWINDOW_0003, "Number of tracks" },
	{ STR_FLOPPYDUMPWINDOW_0004, "Retry" },
	{ STR_FLOPPYDUMPWINDOW_0005, "Drive A:" },
	{ STR_FLOPPYDUMPWINDOW_0006, "Drive B:" },
	{ STR_FLOPPYDUMPWINDOW_0007, "Double sided" },
	{ STR_FLOPPYDUMPWINDOW_0008, "Double step" },
	{ STR_FLOPPYDUMPWINDOW_0009, "Status" },
	{ STR_FLOPPYDUMPWINDOW_0011, "Read Disk" },
	{ STR_FLOPPYDUMPWINDOW_0012, "Floppy Map" },

	{ STR_EDITTOOLWINDOW_0001, "Track edition toolbar" },
	{ STR_EDITTOOLWINDOW_0002, "Copy" },
	{ STR_EDITTOOLWINDOW_0003, "Select start point" },
	{ STR_EDITTOOLWINDOW_0004, "Select end point" },
	{ STR_EDITTOOLWINDOW_0005, "Paste" },
	{ STR_EDITTOOLWINDOW_0006, "Fill" },
	{ STR_EDITTOOLWINDOW_0007, "Insert" },
	{ STR_EDITTOOLWINDOW_0008, "Delete" },
	{ STR_EDITTOOLWINDOW_0009, "Set Flakey pattern" },
	{ STR_EDITTOOLWINDOW_0010, "Set bitrate" },
	{ STR_EDITTOOLWINDOW_0011, "Direct edition" },
	{ STR_EDITTOOLWINDOW_0012, "Shift" },
	{ STR_EDITTOOLWINDOW_0013, "Erase Side 1" },
	{ STR_EDITTOOLWINDOW_0014, "Erase Side 0" },
	{ STR_EDITTOOLWINDOW_0015, "Set Disk Bitrate" },
	{ STR_EDITTOOLWINDOW_0016, "Set Disk RPM" },
	{ STR_EDITTOOLWINDOW_0017, "Add Track" },
	{ STR_EDITTOOLWINDOW_0018, "Remove Last Track" },
	{ STR_EDITTOOLWINDOW_0019, "Remove Odd Tracks" },
	{ STR_EDITTOOLWINDOW_0020, "Shift Tracks" },
	{ STR_EDITTOOLWINDOW_0021, "Swap sides" },
	{ STR_EDITTOOLWINDOW_0022, "Repair" },
	{ STR_EDITTOOLWINDOW_0023, "Reverse Tracks" },
	{ STR_EDITTOOLWINDOW_0024, "Add/Delete Side 1" },
	{ STR_EDITTOOLWINDOW_0025, "Insert Track" },
	{ STR_EDITTOOLWINDOW_0026, "Remove Track" },

#ifndef OEM_MODE
	{ STR_USBSETTINGSWINDOW_0001, "USB HxC Floppy Emulator settings and status" },
	{ STR_USBSETTINGSWINDOW_0002, "USB HxC Floppy Emulator status" },
#else
	{ STR_USBSETTINGSWINDOW_0001, "Floppy Emulator settings and status" },
	{ STR_USBSETTINGSWINDOW_0002, "Floppy Emulator status" },
#endif
	{ STR_USBSETTINGSWINDOW_0003, "Status" },
	{ STR_USBSETTINGSWINDOW_0004, "USB statistics and settings" },
	{ STR_USBSETTINGSWINDOW_0005, "Max settle time" },
	{ STR_USBSETTINGSWINDOW_0006, "Min settle time" },
	{ STR_USBSETTINGSWINDOW_0007, "Sync lost" },
	{ STR_USBSETTINGSWINDOW_0008, "Packet sent" },
	{ STR_USBSETTINGSWINDOW_0009, "Data sent" },
	{ STR_USBSETTINGSWINDOW_0010, "Data throughput" },
	{ STR_USBSETTINGSWINDOW_0011, "Reset" },
	{ STR_USBSETTINGSWINDOW_0012, "Packet size" },
	{ STR_USBSETTINGSWINDOW_0013, "Drive ID setting" },
	{ STR_USBSETTINGSWINDOW_0014, "DS2" },
	{ STR_USBSETTINGSWINDOW_0015, "MTRON" },
	{ STR_USBSETTINGSWINDOW_0016, "Twisted cable" },
	{ STR_USBSETTINGSWINDOW_0017, "Disable drive" },
	{ STR_USBSETTINGSWINDOW_0018, "DS0" },
	{ STR_USBSETTINGSWINDOW_0019, "DS1" },
	{ STR_USBSETTINGSWINDOW_0020, "Mode" },
	{ STR_USBSETTINGSWINDOW_0021, "Auto" },
	{ STR_USBSETTINGSWINDOW_0022, "Double Step" },

	{ STR_FLOPPYVIEWERWINDOW_0001, "Visual Floppy disk" },
	{ STR_FLOPPYVIEWERWINDOW_0002, "Track / Side selection" },
	{ STR_FLOPPYVIEWERWINDOW_0003, "Track number" },
	{ STR_FLOPPYVIEWERWINDOW_0004, "Side number" },
	{ STR_FLOPPYVIEWERWINDOW_0005, "Status" },
	{ STR_FLOPPYVIEWERWINDOW_0007, "Floppy Map" },
	{ STR_FLOPPYVIEWERWINDOW_0008, "View" },
	{ STR_FLOPPYVIEWERWINDOW_0009, "full y time scale (us)" },
	{ STR_FLOPPYVIEWERWINDOW_0010, "x offset (% of the track len)" },
	{ STR_FLOPPYVIEWERWINDOW_0011, "Track view mode" },
	{ STR_FLOPPYVIEWERWINDOW_0012, "Disk view mode" },
	{ STR_FLOPPYVIEWERWINDOW_0013, "full x time scale" },
	{ STR_FLOPPYVIEWERWINDOW_0014, "Track analysis format" },
	{ STR_FLOPPYVIEWERWINDOW_0015, "ISO MFM" },
	{ STR_FLOPPYVIEWERWINDOW_0016, "ISO FM" },
	{ STR_FLOPPYVIEWERWINDOW_0017, "AMIGA MFM" },
	{ STR_FLOPPYVIEWERWINDOW_0018, "MEMBRAIN" },
	{ STR_FLOPPYVIEWERWINDOW_0019, "TYCOM" },
	{ STR_FLOPPYVIEWERWINDOW_0020, "E-Emu" },
	{ STR_FLOPPYVIEWERWINDOW_0021, "Apple" },
	{ STR_FLOPPYVIEWERWINDOW_0022, "Arburg" },
	{ STR_FLOPPYVIEWERWINDOW_0023, "AED 6200P" },
	{ STR_FLOPPYVIEWERWINDOW_0024, "NORTHSTAR" },
	{ STR_FLOPPYVIEWERWINDOW_0025, "HEATHKIT" },
	{ STR_FLOPPYVIEWERWINDOW_0026, "DEC RX02" },
	{ STR_FLOPPYVIEWERWINDOW_0027, "Edit tools" },

	{ STR_RAWLOADERWINDOW_0001, "RAW File format configuration" },
	{ STR_RAWLOADERWINDOW_0002, "Reverse side" },
	{ STR_RAWLOADERWINDOW_0003, "Inter side sector numbering" },
	{ STR_RAWLOADERWINDOW_0004, "Tracks of a side grouped in the file" },
	{ STR_RAWLOADERWINDOW_0005, "Auto GAP3" },
	{ STR_RAWLOADERWINDOW_0006, "PRE-GAP lenght" },
	{ STR_RAWLOADERWINDOW_0007, "Format value" },
	{ STR_RAWLOADERWINDOW_0008, "Interleave" },
	{ STR_RAWLOADERWINDOW_0009, "Skew" },
	{ STR_RAWLOADERWINDOW_0010, "Side based" },
	{ STR_RAWLOADERWINDOW_0011, "Total Sector" },
	{ STR_RAWLOADERWINDOW_0012, "Total Size" },
	{ STR_RAWLOADERWINDOW_0013, "Number of Track" },
	{ STR_RAWLOADERWINDOW_0014, "Sector ID start" },
	{ STR_RAWLOADERWINDOW_0015, "Sector per track" },
	{ STR_RAWLOADERWINDOW_0016, "RPM :" },
	{ STR_RAWLOADERWINDOW_0017, "Bitrate :" },
	{ STR_RAWLOADERWINDOW_0018, "Load RAW file" },
	{ STR_RAWLOADERWINDOW_0019, "Create Empty Floppy" },
	{ STR_RAWLOADERWINDOW_0020, "Save config" },
	{ STR_RAWLOADERWINDOW_0021, "Load config" },
	{ STR_RAWLOADERWINDOW_0023, "Sector size" },
	{ STR_RAWLOADERWINDOW_0024, "Track type" },
	{ STR_RAWLOADERWINDOW_0025, "GAP3 lenght" },
	{ STR_RAWLOADERWINDOW_0026, "Number of side" },
	{ STR_RAWLOADERWINDOW_0027, "Predefined Disk Layout" },

	{ STR_MAINWINDOW_0001, "" },
	{ STR_MAINWINDOW_0002, "Load" },
	{ STR_MAINWINDOW_0003, "Load a floppy file image" },
	{ STR_MAINWINDOW_0004, "Load Raw image" },
	{ STR_MAINWINDOW_0005, "Load a custom raw floppy image /\ncreate a custom floppy" },
	{ STR_MAINWINDOW_0006, "Batch converter" },
	{ STR_MAINWINDOW_0007, "Convert multiple floppy files images" },
	{ STR_MAINWINDOW_0008, "Disk Browser" },
	{ STR_MAINWINDOW_0009, "Create / Browse a DOS floppy disk" },
	{ STR_MAINWINDOW_0010, "Export" },
	{ STR_MAINWINDOW_0011, "Export/save the loaded file image" },
#ifndef OEM_MODE
	{ STR_MAINWINDOW_0012, "SD HxC Floppy\nEmulator settings" },
	{ STR_MAINWINDOW_0013, "Configure the SD HxC Floppy Emulator" },
	{ STR_MAINWINDOW_0014, "USB HxC Floppy\nEmulator settings" },
	{ STR_MAINWINDOW_0015, "Configure the USB HxC Floppy Emulator" },
#else
	{ STR_MAINWINDOW_0012, "Floppy Emulator\nsettings" },
	{ STR_MAINWINDOW_0013, "Configure the Floppy Emulator" },
	{ STR_MAINWINDOW_0014, "" },
	{ STR_MAINWINDOW_0015, "" },
#endif
	{ STR_MAINWINDOW_0016, "Floppy disk dump" },
	{ STR_MAINWINDOW_0017, "Read a real disk" },
	{ STR_MAINWINDOW_0018, "Track Analyzer" },
	{ STR_MAINWINDOW_0019, "Low level tracks viewer" },
	{ STR_MAINWINDOW_0020, "" },

	{ 0x0000, 0x0000 }
};

const char * getString(unsigned int str_id)
{
	int i;

	i = 0;
	while ( strings_list[i].string && strings_list[i].string_id != str_id )
	{
		i++;
	}

	if(strings_list[i].string)
		return strings_list[i].string;

	return 0;
}