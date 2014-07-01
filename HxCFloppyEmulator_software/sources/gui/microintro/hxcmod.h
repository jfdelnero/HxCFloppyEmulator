

#ifndef MODPLAY_DEF
#define MODPLAY_DEF

void * hxcmod_load(void * modmemory);
void hxcmod_fillbuffer(void * modcontext,unsigned short * buffer, unsigned long nbsample);
void hxcmod_unload(void * modcontext);

#endif

