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

#ifndef __UPDATE_NOTIFICATION_H
#define __UPDATE_NOTIFICATION_H


#ifdef _WIN32
	#include <winsock2.h>
#elif defined (__linux)
	#include <sys/socket.h>
#endif


#define UPDATE_NOTIFICATION_TCP_PORT 31004


extern struct sockaddr_in update_notification_address;


void init_update_notification (void);

void start_update_notification (void);

void stop_update_notification (void);


#endif

