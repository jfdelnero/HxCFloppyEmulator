/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator library
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File    : network.c
// Contains: Network support layer.
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdint.h>

#ifdef	WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close (s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#include <unistd.h>
#include <dirent.h>

#ifdef	OSX
#	include <mach-o/dyld.h>
#endif

#endif

#include "network.h"
#include "libhxcfe.h"
#include "libhxcadaptor.h"

void * network_connect(char * address, unsigned short port)
{
	hxc_tcp_stat * tcp_stat;
#ifdef WIN32
	int iResult;
#endif

	tcp_stat = malloc(sizeof(hxc_tcp_stat));
	if(tcp_stat)
	{
		memset(tcp_stat,0,sizeof(hxc_tcp_stat));

#ifdef WIN32
		iResult = WSAStartup(MAKEWORD(2,2), &tcp_stat->wsaData);
		if (iResult != NO_ERROR)
		{
			free(tcp_stat);
			return (void*)NULL;
		}
#endif

		tcp_stat->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tcp_stat->m_socket == INVALID_SOCKET)
		{
#ifdef WIN32
			WSACleanup();
#endif
			free(tcp_stat);
			return (void*)NULL;
		}

		tcp_stat->clientService.sin_family = AF_INET;
		tcp_stat->clientService.sin_addr.s_addr = inet_addr(address);
		tcp_stat->clientService.sin_port = htons(port);

		if (connect(tcp_stat->m_socket, (SOCKADDR*)&tcp_stat->clientService, sizeof(tcp_stat->clientService)) == SOCKET_ERROR)
		{
			closesocket(tcp_stat->m_socket);

#ifdef WIN32
			WSACleanup();
#endif

			free(tcp_stat);

			return (void*)NULL;
		}

		return (void*)tcp_stat;
	}

	return (void*)NULL;
}

int network_read(void * network_connection, unsigned char * buffer, int size, int timeout)
{
	int bytesRecv;
	int offset;
	hxc_tcp_stat * tcp_stat;

	tcp_stat = (hxc_tcp_stat *)network_connection;

	offset=0;

	while(offset < size)
	{
		bytesRecv = recv(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);
		if(bytesRecv == SOCKET_ERROR)
		{
			return -1;
		}
		else
		{
			offset += bytesRecv;
		}
	}

	return size;
}

int network_read2(void * network_connection, unsigned char * buffer, int size, int timeout)
{
	int bytesRecv;
	int offset;
	hxc_tcp_stat * tcp_stat;

	tcp_stat = (hxc_tcp_stat *)network_connection;

	offset=0;

	bytesRecv = recv(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);

	return bytesRecv;
}

int network_write(void * network_connection, unsigned char * buffer, int size, int timeout)
{
	int bytesSent;
	int offset;
	hxc_tcp_stat * tcp_stat;

	tcp_stat = (hxc_tcp_stat *)network_connection;

	offset = 0;
	while(offset < size)
	{
		bytesSent = send(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);
		if(bytesSent == SOCKET_ERROR)
		{
			return -1;
		}
		else
		{
			offset += bytesSent;
		}
	}

	return size;
}

int network_close(void * network_connection)
{
	hxc_tcp_stat * tcp_stat;

	tcp_stat = (hxc_tcp_stat *)network_connection;

	closesocket (tcp_stat->m_socket);

#ifdef WIN32
	WSACleanup();
#endif
	free(tcp_stat);

	return 0;
}
