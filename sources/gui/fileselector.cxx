#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

int fileselector(char * title,char * str,char *filename,char *filter,int save,int dir)
{
	char * path;
	Fl_Native_File_Chooser fnfc;
  
	fnfc.title(title);
	
	fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM|Fl_Native_File_Chooser::NEW_FOLDER);

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
			path=(char*)fnfc.filename();
			sprintf(str,"%s",path);
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
			dir=(char*)fnfc.filename();
			sprintf(str,"%s",dir);
			return 0;
			break; // FILE CHOSEN
		}
	}
	return -1;
}
