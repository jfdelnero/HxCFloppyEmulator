void init_cpm(FSMNG * fsmng);
int cpm_mountImage(FSMNG * fsmng, FLOPPY *floppy);
int cpm_umountImage(FSMNG * fsmng);
int cpm_getFreeSpace(FSMNG * fsmng);
int cpm_getTotalSpace(FSMNG * fsmng);
int cpm_openFile(FSMNG * fsmng, char * filename);
int cpm_createFile(FSMNG * fsmng, char * filename);
int cpm_writeFile(FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int cpm_readFile( FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int cpm_deleteFile(FSMNG * fsmng, char * filename);
int cpm_closeFile( FSMNG * fsmng,int filehandle);
int cpm_createDir( FSMNG * fsmng,char * foldername);
int cpm_removeDir( FSMNG * fsmng,char * foldername);

int cpm_ftell( FSMNG * fsmng,int filehandle);
int cpm_fseek( FSMNG * fsmng,int filehandle,long offset,int origin);

int cpm_openDir(FSMNG * fsmng, char * path);
int cpm_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent);
int cpm_closeDir(FSMNG * fsmng, int dirhandle);
