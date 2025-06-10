/*
 * copyright (c) 2025 Thomas Paillet <thomas.paillet@net-c.fr>

 * This file is part of PTZ-Memory.

 * PTZ-Memory is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PTZ-Memory is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with PTZ-Memory. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __NETWORK_HEADER_H
#define __NETWORK_HEADER_H


#ifdef _WIN32
	#include <winsock2.h>

	#define SHUT_RD SD_RECEIVE

	void WSAInit (void);

	void timersub (const struct timeval* tvp, const struct timeval* uvp, struct timeval* vvp);

#elif defined (__linux)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	#define SOCKET int
	#define INVALID_SOCKET -1
	#define closesocket close
	#define WSAInit()
	#define WSACleanup()
#endif


#endif
