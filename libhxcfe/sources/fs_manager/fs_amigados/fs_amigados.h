void init_amigados(HXCFE_FSMNG * fsmng);
int amigados_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int amigados_umountImage(HXCFE_FSMNG * fsmng);
int amigados_getFreeSpace(HXCFE_FSMNG * fsmng);
int amigados_getTotalSpace(HXCFE_FSMNG * fsmng);
int amigados_openFile(HXCFE_FSMNG * fsmng, char * filename);
int amigados_createFile(HXCFE_FSMNG * fsmng, char * filename);
int amigados_writeFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int amigados_readFile( HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int amigados_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int amigados_closeFile( HXCFE_FSMNG * fsmng,int filehandle);
int amigados_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int amigados_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int amigados_ftell( HXCFE_FSMNG * fsmng,int filehandle);
int amigados_fseek( HXCFE_FSMNG * fsmng,int filehandle,long offset,int origin);

int amigados_openDir(HXCFE_FSMNG * fsmng, char * path);
int amigados_readDir(HXCFE_FSMNG * fsmng,int dirhandle,HXCFE_FSENTRY * dirent);
int amigados_closeDir(HXCFE_FSMNG * fsmng, int dirhandle);
