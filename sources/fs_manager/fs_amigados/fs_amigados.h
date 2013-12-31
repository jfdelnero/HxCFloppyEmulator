void init_amigados(FSMNG * fsmng);
int amigados_mountImage(FSMNG * fsmng, FLOPPY *floppy);
int amigados_umountImage(FSMNG * fsmng);
int amigados_getFreeSpace(FSMNG * fsmng);
int amigados_getTotalSpace(FSMNG * fsmng);
int amigados_openFile(FSMNG * fsmng, char * filename);
int amigados_createFile(FSMNG * fsmng, char * filename);
int amigados_writeFile(FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int amigados_readFile( FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int amigados_deleteFile(FSMNG * fsmng, char * filename);
int amigados_closeFile( FSMNG * fsmng,int filehandle);
int amigados_createDir( FSMNG * fsmng,char * foldername);
int amigados_removeDir( FSMNG * fsmng,char * foldername);

int amigados_ftell( FSMNG * fsmng,int filehandle);
int amigados_fseek( FSMNG * fsmng,int filehandle,long offset,int origin);

int amigados_openDir(FSMNG * fsmng, char * path);
int amigados_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent);
int amigados_closeDir(FSMNG * fsmng, int dirhandle);
