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

#include <string.h>


#ifdef _WIN32
GdkPixbuf *pixbuf_grille_1;
GdkPixbuf *pixbuf_grille_2;
#endif


#define DLE 0x10
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15


GMutex sw_p_08_mutex;

char OK[2] = {DLE, ACK};
char NOK[2] = {DLE, NAK};

char full_sw_p_08_buffer[14] = {DLE, ACK, DLE, STX, 0x04, 0x00, 0x00, 0x00, 0x00, 0x05, -0x09, DLE, ETX, ETX};
char *sw_p_08_buffer = &full_sw_p_08_buffer[2];
int sw_p_08_buffer_len = 11;

struct sockaddr_in sw_p_08_adresse;
SOCKET sw_p_08_socket;

GThread *sw_p_08_thread = NULL;

gboolean sw_p_08_server_started = FALSE;

typedef struct {
	SOCKET src_socket;
	struct sockaddr_in src_addr;
	GtkWidget *connected_label;
	char buffer[256];
	int recv_len;
	int index;
	GThread *thread;
} remote_device_t;

remote_device_t remote_devices[2];

int number_of_matrix_source = 2;

char tally_opv = 0;

char *sw_p_08_grid_txt = "Grille Snell SW-P-08";


void ask_to_connect_pgm_to_ctrl_opv (void)
{
	g_mutex_lock (&sw_p_08_mutex);

	tally_opv = 1;

	sw_p_08_buffer[2] = 0x04;	//8.2.2. CROSSPOINT CONNECTED Message page 24
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = 0;	//CTRL OPV
	sw_p_08_buffer[6] = 1;	//Source PGM
	sw_p_08_buffer[7] = 5;
	sw_p_08_buffer[8] = -10;
	sw_p_08_buffer[9] = DLE;
	sw_p_08_buffer[10] = ETX;
	sw_p_08_buffer_len = 11;

	if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 11, 0);
	if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 11, 0);

	g_mutex_unlock (&sw_p_08_mutex);
}

void ask_to_connect_ptz_to_ctrl_opv (ptz_t *ptz)
{
	g_mutex_lock (&sw_p_08_mutex);

	tally_opv = ptz->matrix_source_number;

	sw_p_08_buffer[2] = 0x04;	//8.2.2. CROSSPOINT CONNECTED Message page 24
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = 0;	//CTRL OPV

	if (tally_opv == DLE) {
		sw_p_08_buffer[6] = DLE;
		sw_p_08_buffer[7] = DLE;
		sw_p_08_buffer[8] = 5;
		sw_p_08_buffer[9] = -25;
		sw_p_08_buffer[10] = DLE;
//		sw_p_08_buffer[11] = ETX;
		sw_p_08_buffer_len = 12;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 12, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 12, 0);
	} else {
		sw_p_08_buffer[6] = tally_opv;
		sw_p_08_buffer[7] = 5;
		sw_p_08_buffer[8] = -(tally_opv + 9);
		sw_p_08_buffer[9] = DLE;
		sw_p_08_buffer[10] = ETX;
		sw_p_08_buffer_len = 11;

		if (remote_devices[0].src_socket != INVALID_SOCKET) send (remote_devices[0].src_socket, sw_p_08_buffer, 11, 0);
		if (remote_devices[1].src_socket != INVALID_SOCKET) send (remote_devices[1].src_socket, sw_p_08_buffer, 11, 0);
	}

	g_mutex_unlock (&sw_p_08_mutex);
}

gboolean delete_matrix_window (GtkWidget *window)
{
	remote_devices[0].connected_label = NULL;
	remote_devices[1].connected_label = NULL;

	gtk_widget_destroy (window);

	gtk_window_set_transient_for (GTK_WINDOW (settings_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (settings_window), TRUE);

	return GDK_EVENT_STOP;
}

gboolean matrix_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) delete_matrix_window (window);

	return GDK_EVENT_PROPAGATE;
}

void show_matrix_window (void)
{
	GtkWidget *window, *scrolled_window, *box, *grid, *widget;
	int i;
	cameras_set_t *cameras_set_itr;
	ptz_t *ptz;
	char label[64];
	gint main_window_width, main_window_height, window_width, window_height, root_x, root_y;

	gtk_window_set_transient_for (GTK_WINDOW (settings_window), NULL);
	gtk_window_set_modal (GTK_WINDOW (settings_window), FALSE);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), sw_p_08_grid_txt);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (settings_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_matrix_window), NULL);
	g_signal_connect (G_OBJECT (window), "key-press-event", G_CALLBACK (matrix_window_key_press), NULL);

		box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
			sprintf (label, "Liaison TCP/IP : %s:%d", my_ip_adresse, ntohs (sw_p_08_adresse.sin_port));
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
				widget = gtk_label_new (cameras_set_label);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

				widget = gtk_label_new (cameras_label);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

				widget = gtk_label_new ("Index");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);

				widget = gtk_label_new ("1");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, 1, 2, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_1);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_1.png");
			#endif
				gtk_grid_attach (GTK_GRID (grid), widget, 1, 3, 1, 1);

				widget = gtk_label_new ("PGM");
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, 2, 1, 1, 1);

			for (i = 2; i <= number_of_matrix_source; i++) {
				sprintf (label, "%d", i);
				widget = gtk_label_new (label);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, i, 2, 1, 1);

			#ifdef _WIN32
				widget = gtk_image_new_from_pixbuf (pixbuf_grille_2);
			#elif defined (__linux)
				widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/grille_2.png");
			#endif
				gtk_grid_attach (GTK_GRID (grid), widget, i, 3, 1, 1);
			}

		for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
			for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
				ptz = cameras_set_itr->ptz_ptr_array[i];

				widget = gtk_label_new (cameras_set_itr->name);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_label_set_angle (GTK_LABEL (widget), 90);
				gtk_grid_attach (GTK_GRID (grid), widget, ptz->matrix_source_number + 1, 0, 1, 1);

				widget = gtk_label_new (ptz->name);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				gtk_grid_attach (GTK_GRID (grid), widget, ptz->matrix_source_number + 1, 1, 1, 1);
			}
		}

				widget = gtk_label_new (" 1: CTRL OPV");
				gtk_widget_set_halign (widget, GTK_ALIGN_START);
				gtk_widget_set_margin_start (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (grid), widget, number_of_matrix_source + 1, 3, 1, 1);
		gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);

	gtk_window_get_size (GTK_WINDOW (main_window), &main_window_width, &main_window_height);
	gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

	if (window_width > main_window_width) {
		gtk_widget_hide (window);
		gtk_window_get_position (GTK_WINDOW (window), &root_x, &root_y);
		g_object_ref (grid);
		gtk_container_remove (GTK_CONTAINER (box), grid);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scrolled_window), main_window_width - 10);
		gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), window_height - 10);
		gtk_container_add (GTK_CONTAINER (scrolled_window), grid);
		g_object_unref (grid);

		gtk_box_pack_start (GTK_BOX (box), scrolled_window, FALSE, FALSE, 0);
//		gtk_window_move (GTK_WINDOW (window), root_x + (window_width - main_window_width) / 2 + 5, root_y);
		gtk_widget_show_all (window);
	}

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

void send_ok_plus_crosspoint_tally_message_to_socket (SOCKET sock, char dest)
{
	char src;

	if (dest == 0) src = tally_opv;
	else src = 0;

	full_sw_p_08_buffer[4] = 0x03;
	full_sw_p_08_buffer[5] = 0x00;
	full_sw_p_08_buffer[6] = 0x00;
	full_sw_p_08_buffer[7] = dest;

	if (src == DLE) {
		full_sw_p_08_buffer[8] = DLE;
		full_sw_p_08_buffer[9] = DLE;
		full_sw_p_08_buffer[10] = 5;
		full_sw_p_08_buffer[11] = -(dest + 24);
		full_sw_p_08_buffer[12] = DLE;
//		full_sw_p_08_buffer[13] = ETX;
		sw_p_08_buffer_len = 12;

		send (sock, full_sw_p_08_buffer, 14, 0);
	} else {
		full_sw_p_08_buffer[8] = src;
		full_sw_p_08_buffer[9] = 5;
		full_sw_p_08_buffer[10] = -(dest + src + 8);
		full_sw_p_08_buffer[11] = DLE;
		full_sw_p_08_buffer[12] = ETX;
		sw_p_08_buffer_len = 11;

		send (sock, full_sw_p_08_buffer, 13, 0);
	}
}

void send_crosspoint_message_to_socket (SOCKET sock, char dest, char src)
{
	sw_p_08_buffer[2] = 0x04;
	sw_p_08_buffer[3] = 0;
	sw_p_08_buffer[4] = 0;
	sw_p_08_buffer[5] = dest;

	if (src == DLE) {
		sw_p_08_buffer[6] = DLE;
		sw_p_08_buffer[7] = DLE;
		sw_p_08_buffer[8] = 5;
		sw_p_08_buffer[9] = -(dest + 25);
		sw_p_08_buffer[10] = DLE;
//		sw_p_08_buffer[11] = ETX;
		sw_p_08_buffer_len = 12;

		send (sock, sw_p_08_buffer, 12, 0);
	} else {
		sw_p_08_buffer[6] = src;
		sw_p_08_buffer[7] = 5;
		sw_p_08_buffer[8] = -(dest + src + 9);
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
	cameras_set_t *cameras_set_itr;
	ptz_t *ptz;
	int i;

	while (recv_from_remote_device_socket (remote_device, buffer, 2)) {
		g_mutex_lock (&sw_p_08_mutex);

		if (buffer[0] == DLE) {
/*			if (buffer[1] == ACK) {
			} else if (buffer[1] == NAK) {
			} else*/ if (buffer[1] == STX) {
				if (!recv_from_remote_device_socket (remote_device, buffer, 1)) { g_mutex_unlock (&sw_p_08_mutex); break; }

				if (buffer[0] == 0x01) {		//8.1.1. Crosspoint Interrogate Message page 20
					if (!recv_from_remote_device_socket (remote_device, buffer, 7)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					if ((0x01 + buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4]) == 0) {
						send_ok_plus_crosspoint_tally_message_to_socket (remote_device->src_socket, buffer[2]);
					} else send (remote_device->src_socket, NOK, 2, 0);
				} else if (buffer[0] == 0x02) {	//8.1.2. Crosspoint Connect Message page 21
					if (!recv_from_remote_device_socket (remote_device, buffer, 4)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					if (buffer[3] == DLE) {
						if (!recv_from_remote_device_socket (remote_device, buffer + 3, 5)) { g_mutex_unlock (&sw_p_08_mutex); break; }
					} else if (!recv_from_remote_device_socket (remote_device, buffer + 4, 4)) { g_mutex_unlock (&sw_p_08_mutex); break; }

					if ((0x02 + buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5]) == 0) {
						send (remote_device->src_socket, OK, 2, 0);

						if ((buffer[2] == 0) && (buffer[3] > 1)) {
							ptz = NULL;

							for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
								for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
									if (cameras_set_itr->ptz_ptr_array[i]->matrix_source_number == buffer[3]) {
										ptz = cameras_set_itr->ptz_ptr_array[i];
										break;
									}
								}
								if (ptz != NULL) break;
							}

							if (cameras_set_itr != NULL) {
								if (cameras_set_itr != current_camera_set) gtk_notebook_set_current_page (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page_num);

								tally_opv = ptz->matrix_source_number;
								gtk_window_set_position (GTK_WINDOW (ptz->control_window.window), GTK_WIN_POS_CENTER);
								show_control_window (ptz);
								gdk_window_get_device_position_double (ptz->control_window.gdk_window, trackball, &ptz->control_window.x, &ptz->control_window.y, NULL);
							}
						}

						send_crosspoint_message_to_socket (remote_device->src_socket, buffer[2], buffer[3]);
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

	memset (&sw_p_08_adresse, 0, sizeof (struct sockaddr_in));
	sw_p_08_adresse.sin_family = AF_INET;
	sw_p_08_adresse.sin_port = htons (SW_P_08_TCP_PORT);
	sw_p_08_adresse.sin_addr.s_addr = inet_addr (my_ip_adresse);

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

	if (bind (sw_p_08_socket, (struct sockaddr *)&sw_p_08_adresse, sizeof (struct sockaddr_in)) == 0) {
		if (listen (sw_p_08_socket, 2) == 0) sw_p_08_thread = g_thread_new (NULL, (GThreadFunc)sw_p_08_server, NULL);
	}
}

void stop_sw_p_08 (void)
{
	sw_p_08_server_started = FALSE;

	if (remote_devices[0].src_socket != INVALID_SOCKET) {
		closesocket (remote_devices[0].src_socket);
		remote_devices[0].src_socket = INVALID_SOCKET;
	}

	if (remote_devices[1].src_socket != INVALID_SOCKET) {
		closesocket (remote_devices[1].src_socket);
		remote_devices[1].src_socket = INVALID_SOCKET;
	}

	closesocket (sw_p_08_socket);

	tally_opv = 0;

/*	if (remote_devices[0].thread != NULL) {
		g_thread_join (remote_devices[0].thread);
		remote_devices[0].thread = NULL;
	}

	if (remote_devices[1].thread != NULL) {
		g_thread_join (remote_devices[1].thread);
		remote_devices[1].thread = NULL;
	}*/

//	if (sw_p_08_thread != NULL) g_thread_join (sw_p_08_thread);
	sw_p_08_thread = NULL;
}

