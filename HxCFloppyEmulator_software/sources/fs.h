extern "C"
{
	int hxc_open (const char *filename, int flags, ...);

	FILE *hxc_fopen (const char *filename, const char *mode);
	int hxc_fclose(FILE * f);
	int hxc_stat( const char *filename, struct stat *buf);
}
