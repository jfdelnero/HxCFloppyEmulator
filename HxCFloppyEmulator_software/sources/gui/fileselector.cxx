#include "fl_includes.h"

int fileselector(char * title,char * str,char *filename,char *filter,int save,int dir)
{
	char * path;
	Fl_Native_File_Chooser fnfc;

	fnfc.title(title);

	fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM|Fl_Native_File_Chooser::NEW_FOLDER|Fl_Native_File_Chooser::USE_FILTER_EXT);

	if(!dir)
	{
		if(save)
			fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		else
			fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
	}
	else
	{
		if(save)
			fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY);
		else
			fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	}

	fnfc.filter(filter);

	if(filename)
	{
		fnfc.preset_file(filename);
	}

	// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
		{
			break; // ERROR
		}
		case 1:
		{
			break; // CANCEL
		}
		default:
		{
			str[0] = 0;
			path=(char*)fnfc.filename();
			if(path)
			{
				snprintf(str,MAX_TMP_STR_SIZE,"%s",path);
			}
			return 0;
			break; // FILE CHOSEN
		}
	}
	return -1;
}

int select_dir(char * title,char * str)
{
	char * dir;
	Fl_Native_File_Chooser fnfc;

	fnfc.title(title);
	fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	fnfc.filter("\t*.*\n");

	// Show native chooser
	switch ( fnfc.show() )
	{
		case -1:
		{
			break; // ERROR
		}
		case 1:
		{
			break; // CANCEL
		}
		default:
		{
			str[0] = 0;
			dir=(char*)fnfc.filename();
			if(dir)
			{
				snprintf(str,MAX_TMP_STR_SIZE,"%s",dir);
			}

			return 0;

			break; // FILE CHOSEN
		}
	}

	return -1;
}
