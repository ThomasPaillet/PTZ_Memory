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

#include <winsock2.h>


void WSAInit (void)
{
	WORD requested_version;
	WSADATA wsa_data;
	BOOL return_value = TRUE;

	requested_version = MAKEWORD (2, 0);

	if (WSAStartup (requested_version, &wsa_data)) return_value = FALSE;
	else if (LOBYTE (wsa_data.wVersion) != 2) {
			WSACleanup ();
			return_value = FALSE;
	}
	if (!return_value) MessageBox (NULL, "WSAInit error !", NULL, MB_OK | MB_ICONERROR);
}

void timersub (const struct timeval* tvp, const struct timeval* uvp, struct timeval* vvp)
{
	vvp->tv_sec = tvp->tv_sec - uvp->tv_sec;
	vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;

	if (vvp->tv_usec < 0) {
		--vvp->tv_sec;
		vvp->tv_usec += 1000000;
	}
}

