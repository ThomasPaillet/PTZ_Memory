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

#include "free_d.h"

#include "cameras_set.h"
#include "logging.h"
#include "protocol.h"

#include <unistd.h>


char free_d_output_ip_address[16] = { '\0' };

gboolean free_d_output_ip_address_is_valid = FALSE;

struct sockaddr_in free_d_output_address;
SOCKET free_d_output_socket;

struct sockaddr_in free_d_input_address;
SOCKET free_d_input_socket;

gboolean outgoing_free_d_started = FALSE;

GThread *outgoing_free_d_thread = NULL;

GThread *incomming_free_d_thread = NULL;



void init_free_d (void)
{
	memset (&free_d_output_address, 0, sizeof (struct sockaddr_in));

	free_d_output_address.sin_family = AF_INET;
	free_d_output_address.sin_port = htons (FREE_D_UDP_PORT);

	memset (&free_d_input_address, 0, sizeof (struct sockaddr_in));

	free_d_input_address.sin_family = AF_INET;
	free_d_input_address.sin_port = htons (FREE_D_UDP_PORT);
	free_d_input_address.sin_addr.s_addr = inet_addr (my_ip_address);
}

gpointer send_free_d_message (gpointer data)
{
	char free_d_message[29];
	cameras_set_t *cameras_set_itr;
	int i, j;
	ptz_t *ptz;
	gint32 value;
	char checksum;

	do {
		g_mutex_lock (&cameras_sets_mutex);

		for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
			for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
				ptz = cameras_set_itr->cameras[i];

				if (ptz->is_on) {
					g_mutex_lock (&ptz->free_d_mutex);

					free_d_message[0] = 0xFF;

					free_d_message[1] = ptz->index;

					value = ptz->incomming_free_d_Pan + ptz->free_d_P + ptz->free_d_Pan;
					while (value > 5898240) value -= 5898240;	//180 * 32768;
					while (value < -5898240) value += 5898240;

					free_d_message[2] = ((char *)&value)[2];
					free_d_message[3] = ((char *)&value)[1];
					free_d_message[4] = ((char *)&value)[0];

					free_d_message[5] = ((char *)&ptz->free_d_Tilt)[2];
					free_d_message[6] = ((char *)&ptz->free_d_Tilt)[1];
					free_d_message[7] = ((char *)&ptz->free_d_Tilt)[0];

					free_d_message[8] = 0x00;
					free_d_message[9] = 0x00;
					free_d_message[10] = 0x00;

					value = ptz->incomming_free_d_X + ptz->free_d_X;
					if (value > 8388544) value = 8388544;	//131,071 * 64;
					else if (value < -8388544) value = -8388544;

					free_d_message[11] = ((char *)&value)[2];
					free_d_message[12] = ((char *)&value)[1];
					free_d_message[13] = ((char *)&value)[0];

					value = ptz->incomming_free_d_Y + ptz->free_d_Y;
					if (value > 8388544) value = 8388544;	//131,071 * 64;
					else if (value < -8388544) value = -8388544;

					free_d_message[14] = ((char *)&value)[2];
					free_d_message[15] = ((char *)&value)[1];
					free_d_message[16] = ((char *)&value)[0];

					value = ptz->incomming_free_d_Z + ptz->free_d_Z + ptz->free_d_optical_axis_height;
					if (value > 8388544) value = 8388544;	//131,071 * 64;
					else if (value < -8388544) value = -8388544;

					free_d_message[17] = ((char *)&value)[2];
					free_d_message[18] = ((char *)&value)[1];
					free_d_message[19] = ((char *)&value)[0];

					free_d_message[20] = ((char *)&ptz->zoom_position)[2];
					free_d_message[21] = ((char *)&ptz->zoom_position)[1];
					free_d_message[22] = ((char *)&ptz->zoom_position)[0];

					free_d_message[23] = ((char *)&ptz->focus_position)[2];
					free_d_message[24] = ((char *)&ptz->focus_position)[1];
					free_d_message[25] = ((char *)&ptz->focus_position)[0];

					free_d_message[26] = 0x08;
					free_d_message[27] = 0xFF;

					checksum = 0x40;
					for (j = 0; j < 28; j++) checksum -= free_d_message[j];
					free_d_message[28] = checksum;

					LOG_FREE_D_MESSAGE(inet_ntoa (free_d_output_address.sin_addr), free_d_message,sizeof (free_d_message),FALSE)

					sendto (free_d_output_socket, free_d_message, sizeof (free_d_message), 0, (struct sockaddr *)&free_d_output_address, sizeof (struct sockaddr_in));

					g_mutex_unlock (&ptz->free_d_mutex);
				}
			}
		}

		g_mutex_unlock (&cameras_sets_mutex);

		usleep (130000);
	} while (outgoing_free_d_started);

	shutdown (free_d_output_socket, SHUT_RD);
	closesocket (free_d_output_socket);

	return NULL;
}

void start_outgoing_freed_d (void)
{
	int i;
	ptz_t *ptz;

	free_d_output_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	outgoing_free_d_started = TRUE;

	if (current_cameras_set != NULL) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			ptz = current_cameras_set->cameras[i];

			if (ptz->is_on) {
				if (ptz->model == AW_HE130) {
					g_mutex_lock (&ptz->free_d_mutex);

					if (ptz->monitor_pan_tilt_thread == NULL) ptz->monitor_pan_tilt_thread = g_thread_new (NULL, (GThreadFunc)monitor_ptz_pan_tilt_position, ptz);

					g_mutex_unlock (&ptz->free_d_mutex);
				}
			}
		}
	}

	outgoing_free_d_thread = g_thread_new (NULL, send_free_d_message, NULL);
}

void stop_outgoing_freed_d (void)
{
	int i;
	ptz_t *ptz;

	outgoing_free_d_started = FALSE;

	if (current_cameras_set != NULL) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			ptz = current_cameras_set->cameras[i];

			if (ptz->monitor_pan_tilt) {
				g_mutex_lock (&ptz->free_d_mutex);

				ptz->monitor_pan_tilt = FALSE;

//				g_thread_join (ptz->monitor_pan_tilt_thread);
				ptz->monitor_pan_tilt_thread = NULL;

				g_mutex_unlock (&ptz->free_d_mutex);
			}
		}
	}

	if (outgoing_free_d_thread != NULL) {
		g_thread_join (outgoing_free_d_thread);
		outgoing_free_d_thread = NULL;
	}
}

gpointer receive_free_d_message (gpointer data)
{
	struct sockaddr_in src_addr;
	unsigned int addrlen;
	int msg_len, i;
	char free_d_message[64];
	char checksum;
	ptz_t *ptz;
	char *value;

	addrlen = sizeof (src_addr);

	while ((msg_len = recvfrom (free_d_input_socket, free_d_message, sizeof (free_d_message), 0, (struct sockaddr *)&src_addr, &addrlen)) > 1) {
		LOG_FREE_D_MESSAGE(inet_ntoa (src_addr.sin_addr),free_d_message,msg_len,TRUE)

		if (msg_len < 29) continue;

		checksum = 0x40;
		for (i = 0; i < 28; i++) checksum -= free_d_message[i];

		if (checksum != free_d_message[28]) continue;

		g_mutex_lock (&cameras_sets_mutex);

		if ((current_cameras_set != NULL) && (free_d_message[1] < current_cameras_set->number_of_cameras)) {
			ptz = current_cameras_set->cameras[(int)free_d_message[1]];

			if (ptz->active) {
				g_mutex_lock (&ptz->free_d_mutex);

				value = (char *)&ptz->incomming_free_d_Pan;

				if (free_d_message[2] & 0x80) value[3] = 0xFF;
				else value[3] = 0x00;
				value[2] = ((char *)free_d_message)[2];
				value[1] = ((char *)free_d_message)[3];
				value[0] = ((char *)free_d_message)[4];

				value = (char *)&ptz->incomming_free_d_X;

				if (free_d_message[11] & 0x80) value[3] = 0xFF;
				else value[3] = 0x00;
				value[2] = ((char *)free_d_message)[11];
				value[1] = ((char *)free_d_message)[12];
				value[0] = ((char *)free_d_message)[13];

				value = (char *)&ptz->incomming_free_d_Y;

				if (free_d_message[14] & 0x80) value[3] = 0xFF;
				else value[3] = 0x00;
				value[2] = ((char *)free_d_message)[14];
				value[1] = ((char *)free_d_message)[15];
				value[0] = ((char *)free_d_message)[16];

				value = (char *)&ptz->incomming_free_d_Z;

				if (free_d_message[17] & 0x80) value[3] = 0xFF;
				else value[3] = 0x00;
				value[2] = ((char *)free_d_message)[17];
				value[1] = ((char *)free_d_message)[18];
				value[0] = ((char *)free_d_message)[19];

				g_mutex_unlock (&ptz->free_d_mutex);
			}
		}

		g_mutex_unlock (&cameras_sets_mutex);

		addrlen = sizeof (src_addr);
	}

	return NULL;
}

void start_incomming_free_d (void)
{
	free_d_input_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind (free_d_input_socket, (struct sockaddr *)&free_d_input_address, sizeof (struct sockaddr_in));

	incomming_free_d_thread = g_thread_new (NULL, receive_free_d_message, NULL);
}

void stop_incomming_free_d (void)
{
	shutdown (free_d_input_socket, SHUT_RD);
	closesocket (free_d_input_socket);

	if (incomming_free_d_thread != NULL) {
		g_thread_join (incomming_free_d_thread);
		incomming_free_d_thread = NULL;
	}
}

