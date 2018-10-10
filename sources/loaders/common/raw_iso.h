
typedef struct raw_iso_cfg_
{
	int32_t number_of_tracks;
	int32_t number_of_sides;
	int32_t number_of_sectors_per_track;
	int32_t sector_size;
	int32_t start_sector_id;
	int32_t pregap;
	int32_t gap3;
	int32_t interleave;
	int32_t skew_per_track;
	int32_t skew_per_side;
	int32_t bitrate;
	int32_t rpm;
	int32_t track_format;
	int32_t interface_mode;
	int32_t flip_sides;
	uint8_t fill_value;
	int32_t trk_grouped_by_sides;

}raw_iso_cfg;

int raw_iso_loader(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk, FILE * f_img , unsigned char * imagebuffer, int size, raw_iso_cfg * cfg);

void raw_iso_setdefcfg(raw_iso_cfg *rawcfg);
