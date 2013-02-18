int CUI_affiche(int MSGTYPE,char * chaine, ...);
BOOL CALLBACK DialogLogs(HWND  hwndDlg, UINT  message,WPARAM  wParam,LPARAM  lParam );

#define LOGFIFOSIZE 256
#define LOGSTRINGSIZE 1024
typedef struct logfifo_
{
	int in;
	int out;
	unsigned char * fifotab[LOGFIFOSIZE];
}logfifo;

