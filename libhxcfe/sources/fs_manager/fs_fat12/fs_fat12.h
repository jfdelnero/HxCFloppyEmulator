void init_fat12(HXCFE_FSMNG * fsmng);
int fat12_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int fat12_umountImage(HXCFE_FSMNG * fsmng);
int fat12_getFreeSpace(HXCFE_FSMNG * fsmng);
int fat12_getTotalSpace(HXCFE_FSMNG * fsmng);
int fat12_openFile(HXCFE_FSMNG * fsmng, char * filename);
int fat12_createFile(HXCFE_FSMNG * fsmng, char * filename);
int fat12_writeFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int fat12_readFile( HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int fat12_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int fat12_closeFile( HXCFE_FSMNG * fsmng,int filehandle);
int fat12_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int fat12_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int fat12_ftell( HXCFE_FSMNG * fsmng,int filehandle);
int fat12_fseek( HXCFE_FSMNG * fsmng,int filehandle,long offset,int origin);

int fat12_openDir(HXCFE_FSMNG * fsmng, char * path);
int fat12_readDir(HXCFE_FSMNG * fsmng,int dirhandle,HXCFE_FSENTRY * dirent);
int fat12_closeDir(HXCFE_FSMNG * fsmng, int dirhandle);
