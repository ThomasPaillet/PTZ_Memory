/*
 * copyright (c) 2020 Thomas Paillet <thomas.paillet@net-c.fr

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
 * along with PTZ-Memory.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ptz.h"


gboolean controller_is_used = FALSE;

char controller_ip_adresse[16] = {'\0'};
gboolean controller_ip_adresse_is_valid = FALSE;

struct sockaddr_in controller_adresse;

extern char *http_cam_cmd, *http_cam_ptz_header;
extern char *http_header_1, *http_header_2, *http_header_3;

char controller_cmd[260];
int controller_cmd_size;


gpointer controller_switch_ptz (ptz_thread_t *ptz_thread)
{
	int port_number;
	SOCKET sock;

	port_number = ((ptz_t*)(ptz_thread->pointer))->index + 1;

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

	if (connect (sock, (struct sockaddr *) &controller_adresse, sizeof (struct sockaddr_in)) == 0) send (sock, controller_cmd, controller_cmd_size, 0);

	closesocket (sock);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

void init_controller (void)
{
	controller_cmd_size = sprintf (controller_cmd, "%sXPT:00%s%s%s%s%s%s", http_cam_cmd, http_cam_ptz_header, http_header_1, my_ip_adresse, http_header_2, my_ip_adresse, http_header_3);
}

