
typedef struct hxc_tcp_stat_
{
#ifdef WIN32
	WSADATA wsaData;
#endif
	struct sockaddr_in clientService;
	SOCKET m_socket;
}hxc_tcp_stat;
