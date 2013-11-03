void init_fat12(FSMNG * fsmng);
int fat12_mountImage(FSMNG * fsmng, FLOPPY *floppy);
int fat12_umountImage(FSMNG * fsmng);
int fat12_getFreeSpace(FSMNG * fsmng);
int fat12_getTotalSpace(FSMNG * fsmng);
int fat12_openFile(FSMNG * fsmng, char * filename);
int fat12_createFile(FSMNG * fsmng, char * filename);
int fat12_writeFile(FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int fat12_readFile( FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int fat12_deleteFile(FSMNG * fsmng, char * filename);
int fat12_closeFile( FSMNG * fsmng,int filehandle);
int fat12_createDir( FSMNG * fsmng,char * foldername);
int fat12_removeDir( FSMNG * fsmng,char * foldername);

int fat12_ftell( FSMNG * fsmng,int filehandle);
int fat12_fseek( FSMNG * fsmng,int filehandle,long offset,int origin);

int fat12_openDir(FSMNG * fsmng, char * path);
int fat12_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent);
int fat12_closeDir(FSMNG * fsmng, int dirhandle);
