#pragma pack(1)
typedef struct sdhxcfecfgfile_
{
	char signature[16];                 //"HXCFECFGV1.0"
	unsigned char step_sound;           //0x00 -> off 0xFF->on
	unsigned char ihm_sound;            //0x00 -> off 0xFF->on
	unsigned char back_light_tmr;       //0x00 always off, 0xFF always on, other -> on x second
	unsigned char standby_tmr;          //0xFF disable, other -> on x second
	unsigned char disable_drive_select;
	unsigned char buzzer_duty_cycle;
	unsigned char number_of_slot;
	unsigned char slot_index;
	unsigned short update_cnt;
	unsigned char load_last_floppy;
	unsigned char buzzer_step_duration;
	unsigned char lcd_scroll_speed;
	unsigned char startup_mode;         // 0x01 -> In normal mode auto load STARTUPA.HFE
										// 0x02 -> In normal mode auto load STARTUPB.HFE
										// 0x04 -> In slot mode use slot 0 at power up (ignore index)
										// 0x08 -> Pre increment index when inserting the sdcard (no button/lcd mode)
	unsigned char enable_drive_b;       // 0xFF -> Drive B Disabled
	unsigned char indexed_mode;         // 0xFF -> Enabled, 0x00 -> Disabled.

	unsigned char cfg_from_cfg_drive0;
	unsigned char interfacemode_drive0;
	unsigned char pin02_cfg_drive0;
	unsigned char pin34_cfg_drive0;

	unsigned char cfg_from_cfg_drive1;
	unsigned char interfacemode_drive1;
	unsigned char pin02_cfg_drive1;
	unsigned char pin34_cfg_drive1;

	unsigned char drive_b_as_motor_on;

}sdhxcfecfgfile;
#pragma pack()

#define PIN_CFG_AUTO        -1
#define PIN_CFG_LOW         0
#define PIN_CFG_HIGH        1
#define PIN_CFG_NOTREADY    2
#define PIN_CFG_READY       3
#define PIN_CFG_NOTDENSITY  4
#define PIN_CFG_DENSITY     5
#define PIN_CFG_NOTDC1      6
#define PIN_CFG_DC1         7
#define PIN_CFG_NOTDC2      8
#define PIN_CFG_DC2         9
#define PIN_CFG_NOTDC3      10
#define PIN_CFG_DC3         11
#define PIN_CFG_NOTDC4      12
#define PIN_CFG_DC4         13
