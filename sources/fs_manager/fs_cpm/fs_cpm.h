void init_cpm(HXCFE_FSMNG * fsmng);
int cpm_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int cpm_umountImage(HXCFE_FSMNG * fsmng);
int cpm_getFreeSpace(HXCFE_FSMNG * fsmng);
int cpm_getTotalSpace(HXCFE_FSMNG * fsmng);
int cpm_openFile(HXCFE_FSMNG * fsmng, char * filename);
int cpm_createFile(HXCFE_FSMNG * fsmng, char * filename);
int cpm_writeFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int cpm_readFile( HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int cpm_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int cpm_closeFile( HXCFE_FSMNG * fsmng,int filehandle);
int cpm_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int cpm_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int cpm_ftell( HXCFE_FSMNG * fsmng,int filehandle);
int cpm_fseek( HXCFE_FSMNG * fsmng,int filehandle,long offset,int origin);

int cpm_openDir(HXCFE_FSMNG * fsmng, char * path);
int cpm_readDir(HXCFE_FSMNG * fsmng,int dirhandle,HXCFE_FSENTRY * dirent);
int cpm_closeDir(HXCFE_FSMNG * fsmng, int dirhandle);
