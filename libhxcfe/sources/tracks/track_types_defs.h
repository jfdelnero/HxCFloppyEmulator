
typedef struct isoibm_config_
{
	// format index
	int             indexformat;

	// post index gap4 config
	unsigned char   data_gap4a;
	unsigned int    len_gap4a;

	// index sync config
	unsigned char   data_isync;
	unsigned int    len_isync;

	// index mark coding
	unsigned char   data_indexmarkp1;
	unsigned char   clock_indexmarkp1;
	unsigned char   len_indexmarkp1;

	unsigned char   data_indexmarkp2;
	unsigned char   clock_indexmarkp2;
	unsigned char   len_indexmarkp2;

	// gap1 config

	unsigned char   data_gap1;
	unsigned int    len_gap1;

	// h sync config
	unsigned char   data_ssync;
	unsigned int    len_ssync;

	// d sync config
	unsigned char   data_dsync;
	unsigned int    len_dsync;

	// address mark coding
	unsigned char   data_addrmarkp1;
	unsigned char   clock_addrmarkp1;
	unsigned char   len_addrmarkp1;

	unsigned char   data_addrmarkp2;
	unsigned char   clock_addrmarkp2;
	unsigned char   len_addrmarkp2;

	// gap2 config
	unsigned char   data_gap2;
	unsigned int    len_gap2;

	// data mark coding
	unsigned char   data_datamarkp1;
	unsigned char   clock_datamarkp1;
	unsigned char   len_datamarkp1;

	unsigned char   data_datamarkp2;
	unsigned char   clock_datamarkp2;
	unsigned char   len_datamarkp2;

	// gap3 config
	unsigned char   data_gap3;
	unsigned int    len_gap3;

	unsigned char   data_gap4b;
	unsigned int    len_gap4b;

	unsigned char   track_id;
	unsigned char   side_id;
	unsigned char   sector_id;
	unsigned char   sector_size_id;

	unsigned short  crc_poly,crc_initial;

	unsigned char   posthcrc_glitch_data;
	unsigned char   posthcrc_glitch_clock;
	unsigned char   posthcrc_len;

	unsigned char   postdcrc_glitch_data;
	unsigned char   postdcrc_glitch_clock;
	unsigned char   postdcrc_len;

}isoibm_config;

static isoibm_config formatstab[]=
{    //     I         --gap4a --i sync --     index mark      --  gap1 --h sync -- --d sync --    add mark            -- gap2 --
	{
		IBMFORMAT_SD,

		0xFF,40, 				// post index gap4 config

		0x00, 6,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,26,				// gap1 config

		0x00, 6,				// h sync config

		0x00, 6,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,11,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0 			// post crc data glith
	},
	{
		IBMFORMAT_DD,

		0x4E,80,				// post index gap4 config

		0x00,12,				// index sync config

		0xC2,0x14,3,			// index mark coding
		0xFC,0xFF,1,

		0x4E,50,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0 			// post crc data glith
	},
	{
		ISOFORMAT_SD,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,16,				// gap1 config

		0x00,06,				// h sync config

		0x00,06,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,11,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ISOFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ISOFORMAT_DD11S,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,00,				// gap1 config

		0x00,03,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,5,					// gap3 config

		0x4E,0xFF,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		AMIGAFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,00,				// gap1 config

		0x00,02,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,2,			// address mark coding (0x4489 0x4489)
		0xFF,0xFF,0,

		0x00,16,				// gap2 config

		0xA1,0x0A,0,			// data mark coding
		0xFB,0xFF,0,

		0x00,0,					// gap3 config

		0xFF,0xFF,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		TYCOMFORMAT_SD,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,16,				// gap1 config

		0x00,04,				// h sync config

		0x00,04,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,6,					// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0x00,0xFF,0x00,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		MEMBRAINFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,1,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,1,			// data mark coding
		0xF8,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x8005,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		EMUFORMAT_SD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x8005,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		APPLE2_GCR5A3,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		APPLE2_GCR6A2,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ARBURG_DAT,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,56,				// gap1 config

		0x00,00,				// h sync config

		0x00,00,				// d sync config

		0x00,0x00,0,			// address mark coding
		0x00,0x00,0,

		0x00,00,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFF,0xFF,1,

		0x00,01,				// gap3 config

		0x00,255,				// gap4 config

		0x00,0x00,0x00,0x00,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ARBURG_SYS,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,56,				// gap1 config

		0x00,00,				// h sync config

		0x00,00,				// d sync config

		0x00,0x00,0,			// address mark coding
		0x00,0x00,0,

		0x00,00,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFF,0xFF,1,

		0x00,01,				// gap3 config

		0x00,255,				// gap4 config

		0x00,0x00,0x00,0x00,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},	
	{
		UKNCFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,27,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,24,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,39,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0xA1,0x0A,1,			// post crc header glith
		0xA1,0x0A,3				// post crc data glith
	},
	{
		0,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,

		0x00,12,

		0xA1,0xFF,3,
		0xFE,0xFF,1,

		0x4E,22,

		0xA1,0xFF,3,
		0xFB,0xFF,1,

		0x4E,84,

		0x4E,255,

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith

	}
};
