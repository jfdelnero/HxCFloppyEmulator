BOOL CALLBACK DialogSDHxCFESettings(HWND  hwndDlg, UINT  message,WPARAM  wParam,LPARAM  lParam );


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
}sdhxcfecfgfile;

