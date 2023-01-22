void init_flex(HXCFE_FSMNG * fsmng);
int32_t flex_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int32_t flex_umountImage(HXCFE_FSMNG * fsmng);
int32_t flex_getFreeSpace(HXCFE_FSMNG * fsmng);
int32_t flex_getTotalSpace(HXCFE_FSMNG * fsmng);
int32_t flex_openFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t flex_createFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t flex_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t flex_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t flex_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t flex_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t flex_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int32_t flex_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int32_t flex_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t flex_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin);

int32_t flex_openDir(HXCFE_FSMNG * fsmng, char * path);
int32_t flex_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent);
int32_t flex_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle);
