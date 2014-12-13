
typedef struct _SECTORSEARCHTRACKCACHE
{
	int32_t nb_sector_cached;
	HXCFE_SECTCFG sectorcache[512];
}SECTORSEARCHTRACKCACHE;

#ifndef _HXCFE_SECTORACCESS_

typedef struct _HXCFE_SECTORACCESS
{
	HXCFE* hxcfe;
	HXCFE_FLOPPY *fp;
	int32_t bitoffset;
	int32_t cur_track;
	int32_t cur_side;

	SECTORSEARCHTRACKCACHE * track_cache;

	int32_t old_bitoffset;
}HXCFE_SECTORACCESS;

#define _HXCFE_SECTORACCESS_

#endif
