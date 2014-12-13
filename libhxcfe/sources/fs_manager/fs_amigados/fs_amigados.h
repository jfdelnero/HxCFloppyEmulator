void init_amigados(HXCFE_FSMNG * fsmng);
int32_t amigados_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int32_t amigados_umountImage(HXCFE_FSMNG * fsmng);
int32_t amigados_getFreeSpace(HXCFE_FSMNG * fsmng);
int32_t amigados_getTotalSpace(HXCFE_FSMNG * fsmng);
int32_t amigados_openFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t amigados_createFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t amigados_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t amigados_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t amigados_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t amigados_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t amigados_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int32_t amigados_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int32_t amigados_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t amigados_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin);

int32_t amigados_openDir(HXCFE_FSMNG * fsmng, char * path);
int32_t amigados_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent);
int32_t amigados_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle);
