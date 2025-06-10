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

#include "ultimatte.h"


void init_ultimatte (ultimatte_t *ultimatte)
{
	ultimatte->ip_address[0] = '\0';
	ultimatte->ip_address_is_valid = FALSE;
	ultimatte->connected = FALSE;

	memset (&ultimatte->address, 0, sizeof (struct sockaddr_in));
	ultimatte->address.sin_family = AF_INET;
	ultimatte->address.sin_port = htons (9998);

	ultimatte->connection_thread = NULL;

	ultimatte->will_be_destroyed = FALSE;
}

gboolean g_source_ultimatte_is_connected (ultimatte_t *ultimatte)
{
	gtk_widget_queue_draw (ultimatte->ptz_name_drawing_area);

	return G_SOURCE_REMOVE;
}

gboolean g_source_ultimatte_is_disconnected (ultimatte_t *ultimatte)
{
	g_thread_join (ultimatte->connection_thread);
	ultimatte->connection_thread = NULL;

	if (ultimatte->will_be_destroyed) g_free (ultimatte);
	else gtk_widget_queue_draw (ultimatte->ptz_name_drawing_area);

	return G_SOURCE_REMOVE;
}

gpointer connect_ultimatte (ultimatte_t *ultimatte)
{
	int recv_len;
	char buffer[4096];

	ultimatte->socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect (ultimatte->socket, (struct sockaddr *) &ultimatte->address, sizeof (struct sockaddr_in)) == 0) {
		ultimatte->connected = TRUE;

		g_idle_add ((GSourceFunc)g_source_ultimatte_is_connected, ultimatte);

		while ((recv_len = recv (ultimatte->socket, buffer, sizeof (buffer), 0)) > 0) {
			buffer[recv_len] = '\0';
		}

		closesocket (ultimatte->socket);
		ultimatte->connected = FALSE;
	} else closesocket (ultimatte->socket);

	g_idle_add ((GSourceFunc)g_source_ultimatte_is_disconnected, ultimatte);

	return NULL;
}

void disconnect_ultimatte (ultimatte_t *ultimatte)
{
	send (ultimatte->socket, "quit\n", 5, 0);
}

