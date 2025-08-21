/*
 * copyright (c) 2020 2021 2025 Thomas Paillet <thomas.paillet@net-c.fr>

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

#include "controller.h"

#include "logging.h"
#include "protocol.h"


gboolean controller_is_used = FALSE;

char controller_ip_address[16] = { '\0' };

gboolean controller_ip_address_is_valid = FALSE;

struct sockaddr_in controller_address;


extern char *http_cam_cmd, *http_cam_ptz_header;
extern char *http_header_1, *http_header_2, *http_header_3;

char controller_cmd[260];
int controller_cmd_size;


gpointer controller_switch_ptz (ptz_thread_t *ptz_thread)
{
	int port_number, size;
	SOCKET sock;
	char buffer[264];

	port_number = ptz_thread->ptz->index + 1;

	if (port_number == 10) {
		controller_cmd[28] = '1';
		controller_cmd[29] = '0';
	} else if (port_number <= 9) {
		controller_cmd[28] = '0';
		controller_cmd[29] = '0' + port_number;
	} else {
		g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

		return NULL;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect (sock, (struct sockaddr *) &controller_address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, controller_cmd, controller_cmd_size, 0);

		if (logging && log_panasonic) {
			g_mutex_lock (&logging_mutex);
			log_controller_command (__FILE__, controller_ip_address, controller_cmd, controller_cmd_size);
			g_mutex_unlock (&logging_mutex);

			size = recv (sock, buffer, sizeof (buffer), 0);

			g_mutex_lock (&logging_mutex);
			log_controller_response (__FILE__, controller_ip_address, buffer, size);
			g_mutex_unlock (&logging_mutex);
		}
	}

	closesocket (sock);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

void init_controller (void)
{
	memset (&controller_address, 0, sizeof (struct sockaddr_in));
	controller_address.sin_family = AF_INET;
	controller_address.sin_port = htons (80);

	controller_cmd_size = sprintf (controller_cmd, "%sXPT:00%s%s%s%s%s%s", http_cam_cmd, http_cam_ptz_header, http_header_1, my_ip_address, http_header_2, my_ip_address, http_header_3);
}

