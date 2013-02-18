
void sdhxcfecfg_window_datachanged(Fl_Widget* w, void*);
int getInterfaceSetting(sdhxcfecfg_window *sdcfgw);

#pragma pack(1)
typedef struct sdhxcfecfgfile_
{
	char signature[16]; //"HXCFECFGV1.0"
	unsigned char step_sound;     //0x00 -> off 0xFF->on
	unsigned char ihm_sound;     //0x00 -> off 0xFF->on
	unsigned char back_light_tmr; //0x00 always off, 0xFF always on, other -> on x second
	unsigned char standby_tmr;    //0xFF disable, other -> on x second
	unsigned char disable_drive_select;
	unsigned char buzzer_duty_cycle;
	unsigned char number_of_slot;
	unsigned char slot_index;
	unsigned short update_cnt;
	unsigned char load_last_floppy;
	unsigned char buzzer_step_duration;
    unsigned char lcd_scroll_speed;
	unsigned char startup_mode; // 0x01 -> In normal mode auto load STARTUPA.HFE
								// 0x02 -> In normal mode auto load STARTUPB.HFE
								// 0x04 -> In slot mode use slot 0 at power up (ignore index)
								// 0x08 -> Pre increment index when inserting the sdcard (no button/lcd mode)
}sdhxcfecfgfile;
#pragma pack()

