
typedef struct _SECTORSEARCHTRACKCACHE
{
	int nb_sector_cached;
	HXCFE_SECTCFG sectorcache[512];
}SECTORSEARCHTRACKCACHE;

#ifndef _HXCFE_SECTORACCESS_

typedef struct _HXCFE_SECTORACCESS
{
	HXCFE* hxcfe;
	HXCFE_FLOPPY *fp;
	int bitoffset;
	int cur_track;
	int cur_side;

	SECTORSEARCHTRACKCACHE * track_cache;

	int old_bitoffset;
}HXCFE_SECTORACCESS;

#define _HXCFE_SECTORACCESS_

#endif