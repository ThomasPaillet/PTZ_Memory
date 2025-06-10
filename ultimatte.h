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

#ifndef __ULTIMATTE_H
#define __ULTIMATTE_H


#include <gtk/gtk.h>
#include "network_header.h"


typedef struct ultimatte_s {
	char ip_address[16];
	gboolean ip_address_is_valid;
	gboolean connected;

	SOCKET socket;
	struct sockaddr_in address;

	GThread *connection_thread;

	GtkWidget *ptz_name_drawing_area;

	gboolean will_be_destroyed;

//	char protocol_version[16];
} ultimatte_t;


void init_ultimatte (ultimatte_t *ultimatte);

gpointer connect_ultimatte (ultimatte_t *ultimatte);

void disconnect_ultimatte (ultimatte_t *ultimatte);


#endif

