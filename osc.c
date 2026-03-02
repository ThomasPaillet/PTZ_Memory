/*
 * copyright (c) 2026 Thomas Paillet <thomas.paillet@net-c.fr>

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

#include "osc.h"

#include "cameras_set.h"
#include "logging.h"
#include "main_window.h"
#include "protocol.h"
#include "sw_p_08.h"


struct sockaddr_in osc_address;
SOCKET osc_socket;

GThread *osc_thread = NULL;


void parse_osc_memory (memory_t *memory, char *buffer)
{
	ptz_thread_t *ptz_thread;
	cameras_set_t *cameras_set;
	int i;
	ptz_t *ptz;
	memory_t *other_memory;

	LOG_OSC_STRING_INT("parse_osc_memory ",memory->index)
	LOG_OSC_2_STRINGS("parse_osc_memory ",buffer)

	if (strcmp (buffer, "Store") == 0) {
		if (((ptz_t *)memory->ptz)->is_on) {
			g_signal_handler_block (memory->button, memory->button_handler_id);

			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->pointer = memory;
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)save_memory, ptz_thread);
		}
	} else if (!memory->empty) {
		if (strcmp (buffer, "Recall") == 0) {
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (link_toggle_button))) {
				cameras_set = ((ptz_t *)memory->ptz)->cameras_set;

				for (i = 0; i < cameras_set->number_of_cameras; i++) {
					ptz = cameras_set->cameras[i];

					if (ptz->is_on) {
						other_memory = ptz->memories + memory->index;

						if (!other_memory->empty) {
							g_signal_handler_block (other_memory->button, other_memory->button_handler_id);

							ptz_thread = g_malloc (sizeof (ptz_thread_t));
							ptz_thread->pointer = other_memory;
							ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)load_other_memory, ptz_thread);
						}
					}
				}
			} else {
				if (((ptz_t *)memory->ptz)->is_on) {
					g_signal_handler_block (memory->button, memory->button_handler_id);

					ptz_thread = g_malloc (sizeof (ptz_thread_t));
					ptz_thread->pointer = memory;
					ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)load_other_memory, ptz_thread);
				}
			}

			tell_memory_is_selected (memory->index);
		} else if ((strcmp (buffer, "Delete") == 0) && (((ptz_t *)memory->ptz)->is_on)) delete_memory (memory);
	}
}

void parse_osc_ptz (ptz_t *ptz, char *buffer)
{
	int i;
	ptz_thread_t *ptz_thread;

	LOG_OSC_2_STRINGS("parse_osc_ptz ",ptz->name)
	LOG_OSC_2_STRINGS("parse_osc_ptz ",buffer)

	if (ptz->ip_address_is_valid) {
		i = 0;
		while ((buffer[i] != '/') && (buffer[i] != '\0')) i++;

		if (buffer[i] == '/') {
			if ((i == 1) && ('0' < buffer[0]) && (buffer[0] <= '9')) parse_osc_memory (ptz->memories + buffer[0] - '1', buffer + 2);
			else if (i == 2) {
				if ((buffer[0] == '0') && ('0' < buffer[1]) && (buffer[1] <= '9')) parse_osc_memory (ptz->memories + buffer[1] - '1', buffer + 3);
				else if ((buffer[0] == '1') && ('0' <= buffer[1]) && (buffer[1] <= '9')) parse_osc_memory (ptz->memories + 10 + buffer[1] - '1', buffer + 3);
			}
		} else {
			if (strcmp (buffer, "Power_On") == 0) {
				ptz_thread = g_malloc (sizeof (ptz_thread_t));
				ptz_thread->pointer = ptz;
				ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_on, ptz_thread);
			} else if (strcmp (buffer, "Standby") == 0) {
				ptz_thread = g_malloc (sizeof (ptz_thread_t));
				ptz_thread->pointer = ptz;
				ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_off, ptz_thread);
			}
		}
	}
}

gboolean select_cameras_set (gpointer page_num)
{
	gtk_notebook_set_current_page (GTK_NOTEBOOK (main_window_notebook), GPOINTER_TO_INT (page_num));

	return G_SOURCE_REMOVE;
}

void parse_osc_cameras_set (cameras_set_t *cameras_set, char *buffer)
{
	int i, j;
	ptz_t *ptz;

	LOG_OSC_2_STRINGS("parse_osc_cameras_set ",cameras_set->name)
	LOG_OSC_2_STRINGS("parse_osc_cameras_set ",buffer)

	i = 0;
	while ((buffer[i] != '/') && (buffer[i] != '\0')) i++;

	if (buffer[i] == '/') {
		buffer[i] = '\0';

		for (j = 0; j < cameras_set->number_of_cameras; j++) {
			ptz = cameras_set->cameras[j];

			if (strcmp (buffer, ptz->name) == 0) {
				parse_osc_ptz (ptz, buffer + i + 1);

				break;
			}
		}

		if (j == cameras_set->number_of_cameras) {
			if ((i == 1) && ('0' < buffer[0]) && (buffer[0] <= '9')) parse_osc_ptz (cameras_set->cameras[buffer[0] - '1'], buffer + 2);
			else if (i == 2) {
				if ((buffer[0] == '0') && ('0' < buffer[1]) && (buffer[1] <= '9')) parse_osc_ptz (cameras_set->cameras[buffer[1] - '1'], buffer + 3);
				else if ((buffer[0] == '1') && ('0' <= buffer[1]) && (buffer[1] <= '9')) parse_osc_ptz (cameras_set->cameras[10 + buffer[1] - '1'], buffer + 3);
			}
		}
	} else {
		if (strcmp (buffer, "Group_On") == 0) switch_cameras_set_on (cameras_set);
		else if (strcmp (buffer, "Group_Off") == 0) switch_cameras_set_off (cameras_set);
		else if (strcmp (buffer, "Select") == 0) g_idle_add ((GSourceFunc)select_cameras_set, GINT_TO_POINTER (cameras_set->page_num));
	}
}

void parse_osc_message (char *buffer)
{
	int i, cameras_set_num;
	cameras_set_t *cameras_set_itr;
	GSList *slist_itr;
	ptz_thread_t *ptz_thread;

	LOG_OSC_2_STRINGS("parse_osc_message ",buffer)

	i = 0;
	while ((buffer[i] != '/') && (buffer[i] != '\0')) i++;

	g_mutex_lock (&cameras_sets_mutex);

	if (buffer[i] == '/') {
		buffer[i] = '\0';

		for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
			if (strcmp (buffer, cameras_set_itr->name) == 0) {
				parse_osc_cameras_set (cameras_set_itr, buffer + i + 1);

				break;
			}
		}

		if (cameras_set_itr == NULL) {
			if ((i == 1) && ('0' < buffer[0]) && (buffer[0] <= '9')) {
				cameras_set_num = buffer[0] - '1';

				for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
					if (cameras_set_itr->page_num == cameras_set_num) {
						parse_osc_cameras_set (cameras_set_itr, buffer + 2);

						break;
					}
				}
			} else if (i == 2) {
				if ((buffer[0] == '0') && ('0' < buffer[1]) && (buffer[1] <= '9')) cameras_set_num = buffer[1] - '1';
				else if ((buffer[0] == '1') && ('0' <= buffer[1]) && (buffer[1] <= '9')) cameras_set_num = 10 + buffer[1] - '1';
				else {
					g_mutex_unlock (&cameras_sets_mutex);

					return;
				}

				for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
					if (cameras_set_itr->page_num == cameras_set_num) {
						parse_osc_cameras_set (cameras_set_itr, buffer + 3);

						break;
					}
				}
			}
		}
	} else {
		if (strcmp (buffer, "All_On") == 0) {
			for (slist_itr = ptz_slist; slist_itr != NULL; slist_itr = slist_itr->next) {
				ptz_thread = g_malloc (sizeof (ptz_thread_t));
				ptz_thread->pointer = slist_itr->data;
				ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_on, ptz_thread);
			}
		} else if (strcmp (buffer, "All_Off") == 0) {
			for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) switch_cameras_set_off (cameras_set_itr);
		} else if (strcmp (buffer, "Group_On") == 0) {
			if (current_cameras_set != NULL) switch_cameras_set_on (current_cameras_set);
		} else if (strcmp (buffer, "Group_Off") == 0) {
			if (current_cameras_set != NULL) switch_cameras_set_off (current_cameras_set);
		} else if (strcmp (buffer, "Bind_On") == 0) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (link_toggle_button), TRUE);
		else if (strcmp (buffer, "Bind_Off") == 0) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (link_toggle_button), FALSE);
	}

	g_mutex_unlock (&cameras_sets_mutex);
}

void parse_osc_packet (char *buffer, int size)
{
	char osc_bundle[8] = "#bundle";
	int element_size, bundle_itr = 16;

	if ((size > 1) && (buffer[0] == '/')) parse_osc_message (buffer + 1);
	else if (size > 20) {
		if (*((guint64 *)buffer) == *((guint64 *)osc_bundle)) {
			do {
				((char *)&element_size)[3] = buffer[bundle_itr++];
				((char *)&element_size)[2] = buffer[bundle_itr++];
				((char *)&element_size)[1] = buffer[bundle_itr++];
				((char *)&element_size)[0] = buffer[bundle_itr++];

				parse_osc_packet (buffer + bundle_itr, element_size);

				bundle_itr += element_size;
			} while (bundle_itr < size - 4);
		}
	}
}

gpointer receive_osc_packet (void)
{
	struct sockaddr_in src_addr;
	socklen_t addrlen;
	char packet[2048];
	int size;

	LOG_OSC_STRING("receive_osc_packet ()")

	addrlen = sizeof (src_addr);

	while ((size = recvfrom (osc_socket, packet, sizeof (packet) - 1, 0, (struct sockaddr *)&src_addr, &addrlen)) > 1) {
		packet[size] = '\0';
		LOG_OSC_PACKET(src_addr.sin_addr,packet,size)

		parse_osc_packet (packet, size);

		addrlen = sizeof (src_addr);
	}

	LOG_OSC_STRING("receive_osc_packet () return")

	return NULL;
}

void init_osc (void)
{
	memset (&osc_address, 0, sizeof (struct sockaddr_in));

	osc_address.sin_family = AF_INET;
	osc_address.sin_port = htons (OSC_UDP_PORT);
	osc_address.sin_addr.s_addr = inet_addr (my_ip_address);
}

void start_osc (void)
{
	LOG_OSC_STRING("start_osc ()")

	osc_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind (osc_socket, (struct sockaddr *)&osc_address, sizeof (struct sockaddr_in));

	osc_thread = g_thread_new (NULL, (GThreadFunc)receive_osc_packet, NULL);
}

void stop_osc (void)
{
	LOG_OSC_STRING("stop_osc ()")

	shutdown (osc_socket, SHUT_RD);
	closesocket (osc_socket);

	if (osc_thread != NULL) {
		g_thread_join (osc_thread);
		osc_thread = NULL;
	}
}

