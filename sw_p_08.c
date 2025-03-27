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

#include "sw_p_08.h"

#include "cameras_set.h"
#include "main_window.h"
#include "protocol.h"
#include "settings.h"

#include <string.h>


#ifdef _WIN32
extern GdkPixbuf *pixbuf_grille_1;
extern GdkPixbuf *pixbuf_grille_2;
extern GdkPixbuf *pixbuf_grille_3;
extern GdkPixbuf *pixbuf_grille_4;
#endif


#define DLE 0x10
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15


typedef struct {
	SOCKET src_socket;
	struct sockaddr_in src_addr;
	GtkWidget *connected_label;
	char buffer[256];
	int recv_len;
	int index;
	GThread *thread;
} remote_device_t;


GMutex sw_p_08_mutex;

char OK[2] = { DLE, ACK };
char NOK[2] = { DLE, NAK };

char full_sw_p_08_buffer[14] = { DLE, ACK, DLE, STX, 0x04, 0x00, 0x00, 0x00, 0x00, 0x05, -0x09, DLE, ETX, ETX };
char *sw_p_08_buffer = &full_sw_p_08_buffer[2];
int sw_p_08_buffer_len = 11;

struct sockaddr_in sw_p_08_address;
SOCKET sw_p_08_socket;

GThread *sw_p_08_thread = NULL;

gboolean sw_p_08_server_started = FALSE;

remote_device_t remote_devices[2];

char tally_cameras_set = MAX_CAMERAS + 1;
char tally_ptz = MAX_CAMERAS + 1;


gboolean g_source_select_cameras_set_page (gpointer page_num)
{
	cameras_set_t *cameras_set_itr;

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		if (cameras_set_itr->page_num == GPOINTER_TO_INT (page_num)) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page_num);
			break;
		}
	}

	return G_SOURCE_REMOVE;
}

gboolean g_source_show_control_window (ptz_t *ptz)
{
	if (current_ptz != NULL) {
		current_ptz->control_window.is_on_screen = FALSE;
		gtk_widget_hide (current_ptz->control_window.window);
	}

	gtk_window_set_position (GTK_WINDOW (ptz->control_window.window), GTK_WIN_POS_CENTER);

	show_control_window (ptz);

	if (trackball != NULL) gdk_window_get_device_position_double (ptz->control_window.gdk_window, trackball, &ptz->control_window.x, &ptz->control_window.y, NULL);

	return G_SOURCE_REMOVE;
}

gboolean destroy_matrix_window (GtkWidget *window)
{
	remote_devices[0].connected_label = NULL;
	remote_devices[1].connected_label = NULL;

	gtk_widget_destroy (window);

	return GDK_EVENT_STOP;
}

gboolean matrix_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) {
		destroy_matrix_window (window);

		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

void show_matrix_window (void)
{
	GtkWidget *window, *box, *grid, *widget;
	char label[64];
	int i;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Grille Snell SW-P-08");
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (settings_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (destroy_matrix_window), NULL);
	g_signal_connect (G_OBJECT (window), "key-press-event", G_CALLBACK (matrix_window_key_press), NULL);

		box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
			sprintf (label, "Liaison TCP/IP : %s:%d", my_ip_address, ntohs (sw_p_08_address.sin_port));
			widget = gtk_label_new (label);
			gtk_widget_set_margin_top (widget, MARGIN_VALUE);
			gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

			if (sw_p_08_server_started) {
				if (remote_devices[0].src_socket != INVALID_SOCKET) {
					sprintf (label, "%s est connecté", inet_ntoa (remote_devices[0].src_addr.sin_addr));
					remote_devices[0].connected_label = gtk_label_new (label);
				} else remote_devices[0].connected_label = gtk_label_new (NULL);
				gtk_widget_set_margin_bottom (remote_devices[0].connected_label, MARGIN_VALUE);
				gtk_box_pack_start (GTK_BOX (box), remote_devices[0].connected_label, FALSE, FALSE, 0);

				if (remote_devices[1].src_socket != INVALID_SOCKET) {
					sprintf (label, "%s est connecté", inet_ntoa (remote_devices[1].src_addr.sin_addr));
					remote_devices[1].connected_label = gtk_label_new (label);

				} else remote_devices[1].connected_label = gtk_label_new (NULL);
				gtk_widget_set_margin_bottom (remote_devices[1].connected_label, MARGIN_VALUE);
				gtk_box_pack_start (GTK_BOX (box), remote_devices[1].connected_label, FALSE, FALSE, 0);
			} else {
				widget = gtk_label_new ("Problème réseau !");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
			}

			grid = gtk_grid_new ();
			gtk_widget_set_margin_top (grid, MARGIN_VALUE);
			gtk_widget_set_margin_start (grid, 15);
			gtk_widget_set_margin_end (grid, 15);
				widget = gtk_label_new ("1");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_1);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_1.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_3);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_3.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);

			for (i = 1; i < MAX_CAMERAS; i++) {
				sprintf (label, "%d", i + 1);
				widget = gtk_label_new (label);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (grid), widget, i, 0, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_2);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_2.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, i, 1, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_4);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_4.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, i, 2, 1, 1);
			}

				widget = gtk_label_new ("Echap");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS, 0, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_2);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_2.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS, 1, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_4);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_4.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS, 2, 1, 1);

				widget = gtk_label_new ("Rien");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS + 1, 0, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_2);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_2.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS + 1, 1, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_4);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_4.png");
			#endif
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS + 1, 2, 1, 1);

				widget = gtk_label_new (" 1: Ensemble de caméras");
				gtk_widget_set_halign (widget, GTK_ALIGN_START);
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS + 2, 1, 1, 1);

				widget = gtk_label_new (" 2: PTZ");
				gtk_widget_set_halign (widget, GTK_ALIGN_START);
			gtk_grid_attach (GTK_GRID (grid), widget, MAX_CAMERAS + 2, 2, 1, 1);
		gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);

	gtk_widget_show_all (window);

	if (sw_p_08_server_started) {
		if (remote_devices[0].src_socket == INVALID_SOCKET) gtk_widget_hide (remote_devices[0].connected_label);
		if (remote_devices[1].src_socket == INVALID_SOCKET) gtk_widget_hide (remote_devices[1].connected_label);
	}
}

gboolean remote_device_connect (remote_device_t *remote_device)
{
	char label[32];

	if ((remote_device->connected_label != NULL) && (remote_device->src_socket != INVALID_SOCKET)) {
		sprintf (label, "%s est connecté", inet_ntoa (remote_device->src_addr.sin_addr));
		gtk_label_set_text (GTK_LABEL (remote_device->connected_label), label);
		gtk_widget_show (remote_device->connected_label);
	}

	return G_SOURCE_REMOVE;
}

gboolean remote_device_disconnect (remote_device_t *remote_device)
{
	if (remote_device->connected_label != NULL) gtk_widget_hide (remote_device->connected_label);

	return G_SOURCE_REMOVE;
}

void tell_cameras_set_is_selected (gint page_num)
{
	g_mutex_lock (&sw_p_08_mutex);

	tally_cameras_set = page_num;

	sw_p_08_buffer[2] = 0x04;				//CROSSPOINT CONNECTED Message
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = 0;					//Dest	"1: Ensemble de caméras"
	sw_p_08_buffer[6] = tally_cameras_set;	//Src   "camera_set->page_num"

	if (tally_cameras_set == DLE) {
		sw_p_08_buffer[7] = DLE;
		sw_p_08_buffer[8] = 5;
		sw_p_08_buffer[9] = -25;
		sw_p_08_buffer[10] = DLE;
//		sw_p_08_buffer[11] = ETX;
		sw_p_08_buffer_len = 12;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 12, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 12, 0);
	} else {
		sw_p_08_buffer[7] = 5;
		sw_p_08_buffer[8] = -9 - tally_cameras_set;
		sw_p_08_buffer[9] = DLE;
		sw_p_08_buffer[10] = ETX;
		sw_p_08_buffer_len = 11;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 11, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 11, 0);
	}

	g_mutex_unlock (&sw_p_08_mutex);
}

void ask_to_connect_pgm_to_ctrl_opv (void)
{
	g_mutex_lock (&sw_p_08_mutex);

	tally_ptz = MAX_CAMERAS;

	sw_p_08_buffer[2] = 0x04;			//CROSSPOINT CONNECTED Message
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = 1;				//Dest	"2: PTZ"
	sw_p_08_buffer[6] = MAX_CAMERAS;	//Src   "Echap"
#if MAX_CAMERAS == DLE
	sw_p_08_buffer[7] = DLE;
	sw_p_08_buffer[8] = 5;
	sw_p_08_buffer[9] = -26;
	sw_p_08_buffer[10] = DLE;
//	sw_p_08_buffer[11] = ETX;
	sw_p_08_buffer_len = 12;

	if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 12, 0);
	if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 12, 0);
#else
	sw_p_08_buffer[7] = 5;
	sw_p_08_buffer[8] = -10 - MAX_CAMERAS;
	sw_p_08_buffer[9] = DLE;
	sw_p_08_buffer[10] = ETX;
	sw_p_08_buffer_len = 11;

	if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 11, 0);
	if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 11, 0);
#endif
	g_mutex_unlock (&sw_p_08_mutex);
}

void ask_to_connect_ptz_to_ctrl_opv (ptz_t *ptz)
{
	g_mutex_lock (&sw_p_08_mutex);

	tally_ptz = ptz->index;

	sw_p_08_buffer[2] = 0x04;		//CROSSPOINT CONNECTED Message
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = 1;			//Dest	"2: PTZ"
	sw_p_08_buffer[6] = tally_ptz;	//Src   "ptz->index"

	if (tally_ptz == DLE) {
		sw_p_08_buffer[7] = DLE;
		sw_p_08_buffer[8] = 5;
		sw_p_08_buffer[9] = -26;
		sw_p_08_buffer[10] = DLE;
//		sw_p_08_buffer[11] = ETX;
		sw_p_08_buffer_len = 12;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 12, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 12, 0);
	} else {
		sw_p_08_buffer[7] = 5;
		sw_p_08_buffer[8] = -10 -tally_ptz;
		sw_p_08_buffer[9] = DLE;
		sw_p_08_buffer[10] = ETX;
		sw_p_08_buffer_len = 11;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 11, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 11, 0);
	}

	g_mutex_unlock (&sw_p_08_mutex);
}

void send_ok_plus_crosspoint_tally_message (SOCKET sock, char dest)
{
	char src;

	if (dest == 0) src = tally_cameras_set;
	else if (dest == 1) src = tally_ptz;
	else src = MAX_CAMERAS + 1;

	full_sw_p_08_buffer[4] = 0x03;	//CROSSPOINT TALLY Message
	full_sw_p_08_buffer[5] = 0x00;
	full_sw_p_08_buffer[6] = 0x00;
	full_sw_p_08_buffer[7] = dest;

	if (src == DLE) {
		full_sw_p_08_buffer[8] = DLE;
		full_sw_p_08_buffer[9] = DLE;
		full_sw_p_08_buffer[10] = 5;
		full_sw_p_08_buffer[11] = -24 - dest;
		full_sw_p_08_buffer[12] = DLE;
//		full_sw_p_08_buffer[13] = ETX;
		sw_p_08_buffer_len = 12;

		send (sock, full_sw_p_08_buffer, 14, 0);
	} else {
		full_sw_p_08_buffer[8] = src;
		full_sw_p_08_buffer[9] = 5;
		full_sw_p_08_buffer[10] = -8 - dest - src;
		full_sw_p_08_buffer[11] = DLE;
		full_sw_p_08_buffer[12] = ETX;
		sw_p_08_buffer_len = 11;

		send (sock, full_sw_p_08_buffer, 13, 0);
	}
}

void send_crosspoint_connected_message (SOCKET sock, char dest, char src)
{
	sw_p_08_buffer[2] = 0x04;	//CROSSPOINT CONNECTED Message
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = dest;

	if (src == DLE) {
		sw_p_08_buffer[6] = DLE;
		sw_p_08_buffer[7] = DLE;
		sw_p_08_buffer[8] = 5;
		sw_p_08_buffer[9] = -25 - dest;
		sw_p_08_buffer[10] = DLE;
//		sw_p_08_buffer[11] = ETX;
		sw_p_08_buffer_len = 12;

		send (sock, sw_p_08_buffer, 12, 0);
	} else {
		sw_p_08_buffer[6] = src;
		sw_p_08_buffer[7] = 5;
		sw_p_08_buffer[8] = -9 - dest - src;
		sw_p_08_buffer[9] = DLE;
		sw_p_08_buffer[10] = ETX;
		sw_p_08_buffer_len = 11;

		send (sock, sw_p_08_buffer, 11, 0);
	}
}

gboolean recv_from_remote_device_socket (remote_device_t *remote_device, char* buffer, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (remote_device->index < remote_device->recv_len) {
			buffer[i] = remote_device->buffer[remote_device->index++];
		} else {
			if ((remote_device->recv_len = recv (remote_device->src_socket, remote_device->buffer, 256, 0)) <= 0) return FALSE;
			buffer[i] = remote_device->buffer[0];
			remote_device->index = 1;
		}
	}
	return TRUE;
}

gpointer receive_message_from_remote_device (remote_device_t *remote_device)
{
	char buffer[8];
	ptz_t *ptz;

	while (recv_from_remote_device_socket (remote_device, buffer, 2)) {
		g_mutex_lock (&sw_p_08_mutex);

		if (buffer[0] == DLE) {
/*			if (buffer[1] == ACK) {
			} else if (buffer[1] == NAK) {
			} else*/ if (buffer[1] == STX) {
				if (!recv_from_remote_device_socket (remote_device, buffer, 1)) { g_mutex_unlock (&sw_p_08_mutex); break; }

				if (buffer[0] == 0x01) {		//Crosspoint Interrogate Message
					if (!recv_from_remote_device_socket (remote_device, buffer, 7)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					if ((0x01 + buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4]) == 0) {
						send_ok_plus_crosspoint_tally_message (remote_device->src_socket, buffer[2]);
					} else send (remote_device->src_socket, NOK, 2, 0);
				} else if (buffer[0] == 0x02) {	//Crosspoint Connect Message
					if (!recv_from_remote_device_socket (remote_device, buffer, 4)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					if (buffer[3] == DLE) {
						if (!recv_from_remote_device_socket (remote_device, buffer + 3, 5)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					} else if (!recv_from_remote_device_socket (remote_device, buffer + 4, 4)) { g_mutex_unlock (&sw_p_08_mutex); break; }

					if ((0x02 + buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5]) == 0) {
						send (remote_device->src_socket, OK, 2, 0);

						if (buffer[2] == 0) {
							if (buffer[3] < number_of_cameras_sets) {
								g_idle_add ((GSourceFunc)g_source_select_cameras_set_page, GINT_TO_POINTER ((int)buffer[3]));

								tally_cameras_set = buffer[3];
							} else tally_cameras_set = MAX_CAMERAS + 1;
						} else if (buffer[2] == 1) {
							g_mutex_lock (&cameras_sets_mutex);

							if ((current_cameras_set != NULL) && (buffer[3] < current_cameras_set->number_of_cameras)) {
								ptz = current_cameras_set->cameras[(int)buffer[3]];

								g_mutex_unlock (&cameras_sets_mutex);

								if (ptz->active && gtk_widget_get_sensitive (ptz->name_grid)) {
									g_idle_add ((GSourceFunc)g_source_show_control_window, ptz);

									tally_ptz = ptz->index;
								} else tally_ptz = MAX_CAMERAS + 1;
							} else {
								g_mutex_unlock (&cameras_sets_mutex);

								tally_ptz = MAX_CAMERAS + 1;
							}
						}

						send_crosspoint_connected_message (remote_device->src_socket, buffer[2], buffer[3]);
					} else send (remote_device->src_socket, NOK, 2, 0);
				} else {
					send (remote_device->src_socket, OK, 2, 0);

					if ((remote_device->recv_len = recv (remote_device->src_socket, remote_device->buffer, 256, 0)) <= 0) { g_mutex_unlock (&sw_p_08_mutex); break; }
					remote_device->index = 0;
				}
			}
		}

		g_mutex_unlock (&sw_p_08_mutex);
	}

	closesocket (remote_device->src_socket);
	remote_device->src_socket = INVALID_SOCKET;
	g_idle_add ((GSourceFunc)remote_device_disconnect, remote_device);

	return NULL;
}

gpointer sw_p_08_server (void)
{
	SOCKET src_socket;
	struct sockaddr_in src_addr;
#ifdef _WIN32
	int addrlen;
#elif defined (__linux)
	socklen_t addrlen;
#endif

	sw_p_08_server_started = TRUE;

	while (sw_p_08_server_started) {
		addrlen = sizeof (struct sockaddr_in);
		src_socket = accept (sw_p_08_socket, (struct sockaddr *)&src_addr, &addrlen);

		if (src_socket == INVALID_SOCKET) continue;

		if (remote_devices[0].src_socket == INVALID_SOCKET) {
			if (remote_devices[0].thread != NULL) {
				g_thread_join (remote_devices[0].thread);
				remote_devices[0].thread = NULL;
			}

			if ((remote_devices[0].recv_len = recv (src_socket, remote_devices[0].buffer, 256, 0)) <= 0) {
				closesocket (src_socket);
				continue;
			}

			remote_devices[0].src_socket = src_socket;
			memcpy (&remote_devices[0].src_addr, &src_addr, sizeof (struct sockaddr_in));
			remote_devices[0].index = 0;

			g_idle_add ((GSourceFunc)remote_device_connect, remote_devices);

			remote_devices[0].thread = g_thread_new (NULL, (GThreadFunc)receive_message_from_remote_device, remote_devices);
		} else if (remote_devices[1].src_socket == INVALID_SOCKET) {
			if (remote_devices[1].thread != NULL) {
				g_thread_join (remote_devices[1].thread);
				remote_devices[1].thread = NULL;
			}

			if ((remote_devices[1].recv_len = recv (src_socket, remote_devices[1].buffer, 256, 0)) <= 0) {
				closesocket (src_socket);
				continue;
			}

			remote_devices[1].src_socket = src_socket;
			memcpy (&remote_devices[1].src_addr, &src_addr, sizeof (struct sockaddr_in));
			remote_devices[1].index = 0;

			g_idle_add ((GSourceFunc)remote_device_connect, remote_devices + 1);

			remote_devices[1].thread = g_thread_new (NULL, (GThreadFunc)receive_message_from_remote_device, remote_devices + 1);
		} else closesocket (src_socket);
	}

	return NULL;
}

void init_sw_p_08 (void)
{
	g_mutex_init (&sw_p_08_mutex);

	memset (&sw_p_08_address, 0, sizeof (struct sockaddr_in));
	sw_p_08_address.sin_family = AF_INET;
	sw_p_08_address.sin_port = htons (SW_P_08_TCP_PORT);
	sw_p_08_address.sin_addr.s_addr = inet_addr (my_ip_address);

	remote_devices[0].src_socket = INVALID_SOCKET;
	remote_devices[0].connected_label = NULL;
	remote_devices[0].thread = NULL;

	remote_devices[1].src_socket = INVALID_SOCKET;
	remote_devices[1].connected_label = NULL;
	remote_devices[1].thread = NULL;
}

void start_sw_p_08 (void)
{
	sw_p_08_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (bind (sw_p_08_socket, (struct sockaddr *)&sw_p_08_address, sizeof (struct sockaddr_in)) == 0) {
		if (listen (sw_p_08_socket, 2) == 0) sw_p_08_thread = g_thread_new (NULL, (GThreadFunc)sw_p_08_server, NULL);
	}
}

void stop_sw_p_08 (void)
{
	sw_p_08_server_started = FALSE;

	if (remote_devices[0].src_socket != INVALID_SOCKET) {
		shutdown (remote_devices[0].src_socket, SHUT_RD);
		closesocket (remote_devices[0].src_socket);
		remote_devices[0].src_socket = INVALID_SOCKET;
	}

	if (remote_devices[1].src_socket != INVALID_SOCKET) {
		shutdown (remote_devices[1].src_socket, SHUT_RD);
		closesocket (remote_devices[1].src_socket);
		remote_devices[1].src_socket = INVALID_SOCKET;
	}

	shutdown (sw_p_08_socket, SHUT_RD);
	closesocket (sw_p_08_socket);

	tally_cameras_set = MAX_CAMERAS + 1;
	tally_ptz = MAX_CAMERAS + 1;

	if (remote_devices[0].thread != NULL) {
		g_thread_join (remote_devices[0].thread);
		remote_devices[0].thread = NULL;
	}

	if (remote_devices[1].thread != NULL) {
		g_thread_join (remote_devices[1].thread);
		remote_devices[1].thread = NULL;
	}

	if (sw_p_08_thread != NULL) {
		g_thread_join (sw_p_08_thread);
		sw_p_08_thread = NULL;
	}
}

