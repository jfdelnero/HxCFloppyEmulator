
	typedef struct batchconverterparams_
	{
		HXCFLOPPYEMULATOR * flopemu;
		//HWND windowshwd;
		batch_converter_window *windowshwd;
		char sourcedir[1024];
		char destdir[1024];
		char **filelist;
		int fileformat;
		unsigned long numberoffileconverted;
		int abort;
	}batchconverterparams;

	typedef struct ff_type_
	{
		int id;
		char * name;
		char * plug_id;
		char * ext;
	}ff_type;


	enum
	{
		FF_HFE=0,
		FF_MFM,
		FF_AFI,
		FF_VTR,
		FF_RAW,
		FF_IMD,
		FF_EHFE
	};

	ff_type ff_type_list[]=
	{
		{ FF_HFE,"HFE - SDCard HxC Floppy Emulator file format",PLUGIN_HXC_HFE,".hfe"},
		{ FF_MFM,"MFM - MFM/FM track file format",PLUGIN_HXC_MFM,".mfm"},
		{ FF_AFI,"AFI - Advanced file image format",PLUGIN_HXC_AFI,".afi"},
		{ FF_VTR,"VTR - VTrucco Floppy Emulator file format",PLUGIN_VTR_IMG,".vtr"},
		{ FF_RAW,"RAW - RAW sectors file format",PLUGIN_RAW_IMG,".img"},
		{ FF_IMD,"IMD - IMD sectors file format",PLUGIN_IMD_IMG,".imd"},
		{ FF_EHFE,"HFE - Rev 2 - Experimental",PLUGIN_HXC_EXTHFE,".hfe"},
		{ -1,"",0,0}			
	};
