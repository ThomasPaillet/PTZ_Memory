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

#include "ptz.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>


const char config_file_name[] = "PTZ-Memory.dat";

gboolean backup_needed = FALSE;

const char settings_txt[] = "_Paramètres";

GtkWidget *settings_window = NULL;

GtkWidget *settings_list_box;
GtkWidget *settings_configuration_button;
GtkWidget *settings_new_button;
GtkWidget *settings_delete_button;

GtkEntryBuffer *controller_ip_entry_buffer[4];

int focus_speed = 25;
int zoom_speed = 25;


gboolean delete_confirmation_window_key_press (GtkWidget *confirmation_window, GdkEventKey *event)
{
	if ((event->keyval == GDK_KEY_n) || (event->keyval == GDK_KEY_N) || (event->keyval == GDK_KEY_Escape)) {
		gtk_widget_destroy (confirmation_window);

		return GDK_EVENT_STOP;
	} else if ((event->keyval == GDK_KEY_o) || (event->keyval == GDK_KEY_O)) {
		delete_cameras_set ();
		gtk_widget_destroy (confirmation_window);

		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

void show_delete_confirmation_window (void)
{
	GtkWidget *window, *box1, *box2, *widget;
	GtkListBoxRow *list_box_row;
	GList *list;
	char message[128];
	char *text = "Etes-vous sûr de vouloir supprimer l'ensemble de caméras \"";

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (window), warning_txt);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), FALSE);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (window), "key-press-event", G_CALLBACK (delete_confirmation_window_key_press), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box1), MARGIN_VALUE);
		list_box_row = gtk_list_box_get_selected_row (GTK_LIST_BOX (settings_list_box));
		if (list_box_row != NULL) {
			list = gtk_container_get_children (GTK_CONTAINER (list_box_row));
			sprintf (message, "%s%s\"?", text, gtk_label_get_text (GTK_LABEL (list->data)));
			widget = gtk_label_new (message);
			gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);
		}

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_top (box2, MARGIN_VALUE);
		gtk_box_set_spacing (GTK_BOX (box2), MARGIN_VALUE);
		gtk_box_set_homogeneous (GTK_BOX (box2), TRUE);
			widget = gtk_button_new_with_label ("OUI");
			g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (delete_cameras_set), NULL);
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, TRUE, TRUE, 0);

			widget = gtk_button_new_with_label ("NON");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), window);
			gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_show_all (window);
}

void settings_list_box_row_selected (GtkListBox *list_box, GtkListBoxRow *row)
{
	if (row != NULL) {
		gtk_widget_set_sensitive (settings_configuration_button, TRUE);
		gtk_widget_set_sensitive (settings_delete_button, TRUE);
	} else {
		gtk_widget_set_sensitive (settings_configuration_button, FALSE);
		gtk_widget_set_sensitive (settings_delete_button, FALSE);
	}
}

void delete_settings_window (void)
{
	gtk_widget_destroy (settings_window);
	settings_window = NULL;
}

gboolean settings_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) {
		gtk_widget_destroy (window);

		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

void controller_switch_activated (GtkSwitch *controller_switch, GParamSpec *pspec, GtkWidget *controller_ip_address_box)
{
	controller_is_used = gtk_switch_get_active (controller_switch);

	gtk_widget_set_sensitive (controller_ip_address_box, controller_is_used);

	backup_needed = TRUE;
}

void check_controller_ip_address (void)
{
	int i, j, k;

	for (i = 0, k = 0; i < 3; i++)
	{
		j = gtk_entry_buffer_get_bytes (controller_ip_entry_buffer[i]);
		memcpy (controller_ip_address + k, gtk_entry_buffer_get_text (controller_ip_entry_buffer[i]), j);
		k += j;
		controller_ip_address[k] = '.';
		k++;
	}
	j = gtk_entry_buffer_get_bytes (controller_ip_entry_buffer[i]);
	memcpy (controller_ip_address + k, gtk_entry_buffer_get_text (controller_ip_entry_buffer[3]), j);
	controller_ip_address[k + j] = '\0';

	controller_address.sin_addr.s_addr = inet_addr (controller_ip_address);

	if (controller_address.sin_addr.s_addr == INADDR_NONE) {
		controller_ip_address_is_valid = FALSE;
		controller_ip_address[0] = '\0';
		for (i = 0; i < 4; i++) gtk_entry_buffer_delete_text (controller_ip_entry_buffer[i], 0, -1);
	} else controller_ip_address_is_valid = TRUE;

	backup_needed = TRUE;
}

void focus_speed_changed (GtkRange *focus_speed_scale)
{
	focus_speed = (int)gtk_range_get_value (focus_speed_scale);

	focus_near_speed_cmd[2] = '0' + ((50 - focus_speed) / 10);
	focus_near_speed_cmd[3] = '0' + ((50 - focus_speed) % 10);

	focus_far_speed_cmd[2] = '0' + ((50 + focus_speed) / 10);
	focus_far_speed_cmd[3] = '0' + ((50 + focus_speed) % 10);

	backup_needed = TRUE;
}

void zoom_speed_changed (GtkRange *zoom_speed_scale)
{
	zoom_speed = (int)gtk_range_get_value (zoom_speed_scale);

	zoom_wide_speed_cmd[2] = '0' + ((50 - zoom_speed) / 10);
	zoom_wide_speed_cmd[3] = '0' + ((50 - zoom_speed) % 10);

	zoom_tele_speed_cmd[2] = '0' + ((50 + zoom_speed) / 10);
	zoom_tele_speed_cmd[3] = '0' + ((50 + zoom_speed) % 10);

	backup_needed = TRUE;
}

void send_ip_tally_check_button_toggled (GtkToggleButton *togglebutton)
{
	send_ip_tally = gtk_toggle_button_get_active (togglebutton);

	backup_needed = TRUE;
}

void update_notification_tcp_port_entry_activate (GtkEntry *entry, GtkEntryBuffer *entry_buffer)
{
	int port_sscanf;
	guint16 port;
	cameras_set_t *cameras_set_itr;
	int i;

	stop_update_notification ();

	sscanf (gtk_entry_buffer_get_text (entry_buffer), "%d", &port_sscanf);
	port = (guint16)port_sscanf;

	if ((port_sscanf < 1024) || (port_sscanf > 65535) || (port == ntohs (sw_p_08_address.sin_port))) {
		update_notification_address.sin_port = htons (UPDATE_NOTIFICATION_TCP_PORT);
		gtk_entry_buffer_set_text (entry_buffer, "31004", 5);
	} else update_notification_address.sin_port = htons (port);

	start_update_notification ();

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			if ((cameras_set_itr->ptz_ptr_array[i]->ip_address_is_valid) && (cameras_set_itr->ptz_ptr_array[i]->error_code != 0x30))
				send_update_start_cmd (cameras_set_itr->ptz_ptr_array[i]);
		}
	}

	backup_needed = TRUE;
}

void sw_p_08_tcp_port_entry_activate (GtkEntry *entry, GtkEntryBuffer *entry_buffer)
{
	int port_sscanf;
	guint16 port;

	stop_sw_p_08 ();

	sscanf (gtk_entry_buffer_get_text (entry_buffer), "%d", &port_sscanf);
	port = (guint16)port_sscanf;

	if ((port_sscanf < 1024) || (port_sscanf > 65535) || (port == ntohs (update_notification_address.sin_port))) {
		sw_p_08_address.sin_port = htons (SW_P_08_TCP_PORT);
		gtk_entry_buffer_set_text (entry_buffer, "8000", 5);
	} else sw_p_08_address.sin_port = htons (port);

	start_sw_p_08 ();

	backup_needed = TRUE;
}

void tsl_umd_v5_udp_port_entry_activate (GtkEntry *entry, GtkEntryBuffer *entry_buffer)
{
	int port_sscanf;
	guint16 port;

	stop_tally ();

	sscanf (gtk_entry_buffer_get_text (entry_buffer), "%d", &port_sscanf);
	port = (guint16)port_sscanf;

	if ((port_sscanf < 1024) || (port_sscanf > 65535)) {
		tsl_umd_v5_address.sin_port = htons (TSL_UMD_V5_UDP_PORT);
		gtk_entry_buffer_set_text (entry_buffer, "8900", 5);
	} else tsl_umd_v5_address.sin_port = htons (port);

	start_tally ();

	backup_needed = TRUE;
}

void create_settings_window (void)
{
	GtkWidget *box1, *frame, *box2, *box3, *box4, *widget, *controller_ip_address_box;
	cameras_set_t *cameras_set_itr;
	gint current_page;
	int i, l, k;
	GtkEntryBuffer *entry_buffer;
	char label[8];

	settings_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (settings_window), settings_txt + 1);
	gtk_window_set_type_hint (GTK_WINDOW (settings_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (settings_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (settings_window), TRUE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (settings_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (settings_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (settings_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (settings_window), "delete-event", G_CALLBACK (delete_settings_window), NULL);
	g_signal_connect (G_OBJECT (settings_window), "key-press-event", G_CALLBACK (settings_window_key_press), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Ensembles de caméras");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_margin_top (box2, MARGIN_VALUE);
		gtk_widget_set_margin_start (box2, MARGIN_VALUE);
		gtk_widget_set_margin_end (box2, MARGIN_VALUE);
		gtk_widget_set_margin_bottom (box2, MARGIN_VALUE);
			box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				settings_configuration_button = gtk_button_new_with_label ("_Configuration");
				gtk_button_set_use_underline (GTK_BUTTON (settings_configuration_button), TRUE);
				gtk_widget_set_margin_bottom (settings_configuration_button, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (settings_configuration_button), "clicked", G_CALLBACK (show_cameras_set_configuration_window), NULL);
				if (number_of_cameras_sets == 0) gtk_widget_set_sensitive (settings_configuration_button, FALSE);
			gtk_box_pack_start (GTK_BOX (box3), settings_configuration_button, FALSE, FALSE, 0);

				settings_new_button = gtk_button_new_with_label ("_Nouveau");
				gtk_button_set_use_underline (GTK_BUTTON (settings_new_button), TRUE);
				gtk_widget_set_margin_bottom (settings_new_button, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (settings_new_button), "clicked", G_CALLBACK (add_cameras_set), NULL);
				if (number_of_cameras_sets == MAX_CAMERAS_SET) gtk_widget_set_sensitive (settings_new_button, FALSE);
			gtk_box_pack_start (GTK_BOX (box3), settings_new_button, FALSE, FALSE, 0);

				settings_delete_button = gtk_button_new_with_label ("_Supprimer");
				gtk_button_set_use_underline (GTK_BUTTON (settings_delete_button), TRUE);
				g_signal_connect (G_OBJECT (settings_delete_button), "clicked", G_CALLBACK (show_delete_confirmation_window), NULL);
				if (number_of_cameras_sets == 0) gtk_widget_set_sensitive (settings_delete_button, FALSE);
			gtk_box_pack_start (GTK_BOX (box3), settings_delete_button, FALSE, FALSE, 0);
		gtk_box_pack_end (GTK_BOX (box2), box3, FALSE, FALSE, 0);

			settings_list_box = gtk_list_box_new ();

			for (i = 0; i < number_of_cameras_sets; i++)
			{
				cameras_set_itr = cameras_sets;
				while (cameras_set_itr->page_num != i) cameras_set_itr = cameras_set_itr->next;

				cameras_set_itr->list_box_row = gtk_list_box_row_new ();
				gtk_container_add (GTK_CONTAINER (cameras_set_itr->list_box_row), gtk_label_new (cameras_set_itr->name));
				gtk_container_add (GTK_CONTAINER (settings_list_box), cameras_set_itr->list_box_row);
			}

			g_signal_connect (G_OBJECT (settings_list_box), "row-selected", G_CALLBACK (settings_list_box_row_selected), NULL);

			current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_window_notebook));

			for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
				if (cameras_set_itr->page_num == current_page) {
					gtk_list_box_select_row (GTK_LIST_BOX (settings_list_box), GTK_LIST_BOX_ROW (cameras_set_itr->list_box_row));
					break;
				}
			}

		gtk_box_pack_start (GTK_BOX (box2), settings_list_box, TRUE, TRUE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box2);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		frame = gtk_frame_new ("Pupitre Panasonic");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_widget_set_margin_top (box3, MARGIN_VALUE);
			gtk_widget_set_margin_start (box3, MARGIN_VALUE);
			gtk_widget_set_margin_end (box3, MARGIN_VALUE);
			gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
				controller_ip_address_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					if (controller_ip_address_is_valid) {
						for (l = 1; controller_ip_address[l] != '.'; l++) {}
						controller_ip_entry_buffer[0] = gtk_entry_buffer_new (controller_ip_address, l);
						k = l + 1;
					} else controller_ip_entry_buffer[0] = gtk_entry_buffer_new (network_address[0], network_address_len[0]);

					widget = gtk_entry_new_with_buffer (controller_ip_entry_buffer[0]);
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 3);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 3);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					widget = gtk_label_new (".");
					gtk_widget_set_margin_start (widget, 2);
					gtk_widget_set_margin_end (widget, 2);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					if (controller_ip_address_is_valid) {
						for (l = 1; controller_ip_address[k + l] != '.'; l++) {}
						controller_ip_entry_buffer[1] = gtk_entry_buffer_new (controller_ip_address + k, l);
						k += l + 1;
					} else controller_ip_entry_buffer[1] = gtk_entry_buffer_new (network_address[1], network_address_len[1]);

					widget = gtk_entry_new_with_buffer (controller_ip_entry_buffer[1]);
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 3);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 3);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					widget = gtk_label_new (".");
					gtk_widget_set_margin_start (widget, 2);
					gtk_widget_set_margin_end (widget, 2);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					if (controller_ip_address_is_valid) {
						for (l = 1; controller_ip_address[k + l] != '.'; l++) {}
						controller_ip_entry_buffer[2] = gtk_entry_buffer_new (controller_ip_address + k, l);
						k += l + 1;
					} else controller_ip_entry_buffer[2] = gtk_entry_buffer_new (network_address[2], network_address_len[2]);

					widget = gtk_entry_new_with_buffer (controller_ip_entry_buffer[2]);
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 3);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 3);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					widget = gtk_label_new (".");
					gtk_widget_set_margin_start (widget, 2);
					gtk_widget_set_margin_end (widget, 2);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					if (controller_ip_address_is_valid) {
						for (l = 1; controller_ip_address[k + l] != '.'; l++) {}
						controller_ip_entry_buffer[3] = gtk_entry_buffer_new (controller_ip_address + k, l);
						k += l + 1;
					} else controller_ip_entry_buffer[3] = gtk_entry_buffer_new (NULL, -1);

					widget = gtk_entry_new_with_buffer (controller_ip_entry_buffer[3]);
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 3);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 3);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);

					widget = gtk_button_new_with_label ("Ok");
					gtk_widget_set_margin_start (widget, MARGIN_VALUE);
					g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (check_controller_ip_address), NULL);
				gtk_box_pack_start (GTK_BOX (controller_ip_address_box), widget, FALSE, FALSE, 0);
				gtk_widget_set_sensitive (controller_ip_address_box, controller_is_used);
			gtk_box_pack_end (GTK_BOX (box3), controller_ip_address_box, FALSE, FALSE, 0);

				widget = gtk_switch_new ();
				gtk_widget_set_margin_end (widget, MARGIN_VALUE);
				gtk_switch_set_active (GTK_SWITCH (widget), controller_is_used);
				g_signal_connect (widget, "notify::active", G_CALLBACK (controller_switch_activated), controller_ip_address_box);
			gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box3);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		frame = gtk_frame_new ("Réglages communs à toutes les caméras");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
			box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					widget = gtk_label_new ("Vitesse de réglage du focus");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 1, 49, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (widget), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (widget), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (widget), FALSE);
					gtk_range_set_value (GTK_RANGE (widget), focus_speed);
					g_signal_connect (widget, "value-changed", G_CALLBACK (focus_speed_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box3), widget, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget = gtk_label_new ("Vitesse de réglage du zoom");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 1, 49, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (widget), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (widget), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (widget), FALSE);
					gtk_range_set_value (GTK_RANGE (widget), zoom_speed);
					g_signal_connect (widget, "value-changed", G_CALLBACK (zoom_speed_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box3), widget, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Envoyer les tally aux caméras via IP :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					widget = gtk_check_button_new ();
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), send_ip_tally);
					g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (send_ip_tally_check_button_toggled), NULL);
					gtk_widget_set_margin_start (widget, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Update Notification Port TCP/IP :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					entry_buffer = gtk_entry_buffer_new (label, sprintf (label, "%hu", ntohs (update_notification_address.sin_port)));
					widget = gtk_entry_new_with_buffer (GTK_ENTRY_BUFFER (entry_buffer));
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 5);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 5);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					g_signal_connect (G_OBJECT (widget), "activate", G_CALLBACK (update_notification_tcp_port_entry_activate), entry_buffer);
					gtk_widget_set_margin_start (widget, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box2);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

		frame = gtk_frame_new ("Communication avec la régie");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
			box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("SW-P-08 Port TCP/IP :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					gtk_widget_set_margin_start (box4, MARGIN_VALUE);
					gtk_widget_set_margin_end (box4, MARGIN_VALUE);
						widget =  gtk_button_new_with_label ("Grille Snell SW-P-08");
						g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (show_matrix_window), NULL);
					gtk_box_set_center_widget (GTK_BOX (box4), widget);
				gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);

					entry_buffer = gtk_entry_buffer_new (label, sprintf (label, "%hu", ntohs (sw_p_08_address.sin_port)));
					widget = gtk_entry_new_with_buffer (GTK_ENTRY_BUFFER (entry_buffer));
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 5);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 5);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					g_signal_connect (G_OBJECT (widget), "activate", G_CALLBACK (sw_p_08_tcp_port_entry_activate), entry_buffer);
				gtk_box_pack_end (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("TSL UMD V5 Port UDP/IP :");
					gtk_widget_set_margin_end (widget, MARGIN_VALUE);
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					entry_buffer = gtk_entry_buffer_new (label, sprintf (label, "%hu", ntohs (tsl_umd_v5_address.sin_port)));
					widget = gtk_entry_new_with_buffer (GTK_ENTRY_BUFFER (entry_buffer));
					gtk_entry_set_input_purpose (GTK_ENTRY (widget), GTK_INPUT_PURPOSE_DIGITS);
					gtk_entry_set_max_length (GTK_ENTRY (widget), 5);
					gtk_entry_set_width_chars (GTK_ENTRY (widget), 5);
					gtk_entry_set_alignment (GTK_ENTRY (widget), 0.5);
					g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (digit_key_press), NULL);
					g_signal_connect (G_OBJECT (widget), "activate", G_CALLBACK (tsl_umd_v5_udp_port_entry_activate), entry_buffer);
					gtk_widget_set_margin_start (widget, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box2);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (settings_window), box1);

	gtk_window_set_resizable (GTK_WINDOW (settings_window), FALSE);
	gtk_widget_show_all (settings_window);
}

void load_config_file (void)
{
	FILE *config_file;
	int i, j, k, index;
	cameras_set_t *cameras_set_tmp;
	ptz_t *ptz;
	char memories_name[MAX_MEMORIES][MEMORIES_NAME_LENGTH + 1];
	int pixbuf_rowstride;
	gsize pixbuf_byte_length;
	guint8 *pixbuf_data;

	config_file = fopen (config_file_name, "rb");
	if (config_file == NULL) return;

	fread (&thumbnail_size, sizeof (gdouble), 1, config_file);
	if (thumbnail_size < 0.5) thumbnail_size = 0.5;
	else if (thumbnail_size > 1.0) thumbnail_size = 1.0;

	thumbnail_width = 320 * thumbnail_size;
	thumbnail_height = 180 * thumbnail_size;

	fread (&number_of_cameras_sets, sizeof (int), 1, config_file);

	if (number_of_cameras_sets < 0) number_of_cameras_sets = 0;
	if (number_of_cameras_sets > MAX_CAMERAS_SET) number_of_cameras_sets = 0;

	for (i = 0; i < number_of_cameras_sets; i++)
	{
		cameras_set_tmp = g_malloc (sizeof (cameras_set_t));

		cameras_set_tmp->next = cameras_sets;
		cameras_sets = cameras_set_tmp;

		cameras_set_tmp->number_of_ghost_cameras = 0;

		cameras_set_tmp->thumbnail_width = thumbnail_width;

		fread (cameras_set_tmp->name, sizeof (char), CAMERAS_SET_NAME_LENGTH, config_file);
		cameras_set_tmp->name[CAMERAS_SET_NAME_LENGTH] = '\0';

		fread (&cameras_set_tmp->number_of_cameras, sizeof (int), 1, config_file);
		if ((cameras_set_tmp->number_of_cameras < 1) || (cameras_set_tmp->number_of_cameras > MAX_CAMERAS)) cameras_set_tmp->number_of_cameras = 5;

		cameras_set_tmp->ptz_ptr_array = g_malloc (cameras_set_tmp->number_of_cameras * sizeof (ptz_t*));

		for (j = 0; j < cameras_set_tmp->number_of_cameras; j++) {
			ptz = g_malloc (sizeof (ptz_t));
			cameras_set_tmp->ptz_ptr_array[j] = ptz;

			fread (ptz->name, sizeof (char), 2, config_file);
			ptz->name[2] = '\0';
			ptz->index = j;

			init_ptz (ptz);

			fread (&ptz->active, sizeof (gboolean), 1, config_file);

			if (ptz->active) {
				create_ptz_widgets (ptz);

				fread (ptz->ip_address, sizeof (char), 16, config_file);
				ptz->ip_address[15] = '\0';

				ptz->address.sin_addr.s_addr = inet_addr (ptz->ip_address);

				if (ptz->address.sin_addr.s_addr == INADDR_NONE) {
					ptz->ip_address_is_valid = FALSE;
					ptz->ip_address[0] = '\0';
					cameras_set_with_error = cameras_set_tmp;
				} else ptz->ip_address_is_valid = TRUE;

				fread (&ptz->number_of_memories, sizeof (int), 1, config_file);
				if ((ptz->number_of_memories < 0) || (ptz->number_of_memories > MAX_MEMORIES)) ptz->number_of_memories = 0;

				for (k = 0; k < ptz->number_of_memories; k++) {
					fread (&index, sizeof (int), 1, config_file);

					ptz->memories[index].empty = FALSE;

					fread (ptz->memories[index].pan_tilt_position_cmd + 4, sizeof (char), 8, config_file);
					fread (&ptz->memories[index].zoom_position, sizeof (int), 1, config_file);
					fread (ptz->memories[index].zoom_position_hexa, sizeof (char), 3, config_file);
					fread (&ptz->memories[index].focus_position, sizeof (int), 1, config_file);
					fread (ptz->memories[index].focus_position_hexa, sizeof (char), 3, config_file);
					fread (&pixbuf_rowstride, sizeof (int), 1, config_file);
					fread (&pixbuf_byte_length, sizeof (gsize), 1, config_file);

					pixbuf_data = g_malloc (pixbuf_byte_length);
					fread (pixbuf_data, sizeof (guint8), pixbuf_byte_length, config_file);

					ptz->memories[index].pixbuf = gdk_pixbuf_new_from_data (pixbuf_data, GDK_COLORSPACE_RGB, FALSE, 8, 320, 180, pixbuf_rowstride, (GdkPixbufDestroyNotify)g_free, NULL);
					if (thumbnail_width == 320) ptz->memories[index].image = gtk_image_new_from_pixbuf (ptz->memories[index].pixbuf);
					else {
						ptz->memories[index].scaled_pixbuf = gdk_pixbuf_scale_simple (ptz->memories[index].pixbuf, thumbnail_width, thumbnail_height, GDK_INTERP_BILINEAR);
						ptz->memories[index].image = gtk_image_new_from_pixbuf (ptz->memories[index].scaled_pixbuf);
					}
					gtk_button_set_image (GTK_BUTTON (ptz->memories[index].button), ptz->memories[index].image);

					fread (ptz->memories[index].name, sizeof (char), MEMORIES_NAME_LENGTH, config_file);
					ptz->memories[index].name[MEMORIES_NAME_LENGTH] = '\0';
				}
			} else {
				cameras_set_tmp->number_of_ghost_cameras++;
				create_ghost_ptz_widgets (ptz);
			}
		}

		for (j = 0; j < MAX_MEMORIES; j++) {
			fread (memories_name[j], sizeof (char), MEMORIES_NAME_LENGTH, config_file);
			memories_name[j][MEMORIES_NAME_LENGTH] = '\0';
		}

		add_cameras_set_to_main_window_notebook (cameras_set_tmp);

		for (j = 0; j < MAX_MEMORIES; j++) {
			gtk_entry_set_text (GTK_ENTRY (cameras_set_tmp->entry_widgets[j]), memories_name[j]);
			gtk_label_set_text (GTK_LABEL (cameras_set_tmp->memories_labels[j]), memories_name[j]);
		}
	}

	fread (&controller_is_used, sizeof (gboolean), 1, config_file);

	fread (controller_ip_address, sizeof (char), 16, config_file);
	controller_ip_address[15] = '\0';

	controller_address.sin_addr.s_addr = inet_addr (controller_ip_address);

	if (controller_address.sin_addr.s_addr == INADDR_NONE) {
		controller_ip_address_is_valid = FALSE;
		controller_ip_address[0] = '\0';
	} else controller_ip_address_is_valid = TRUE;

	fread (&focus_speed, sizeof (int), 1, config_file);
	if ((focus_speed < 1) || (focus_speed > 49)) focus_speed = 25;
	focus_near_speed_cmd[2] = '0' + ((50 - focus_speed) / 10);
	focus_near_speed_cmd[3] = '0' + ((50 - focus_speed) % 10);
	focus_far_speed_cmd[2] = '0' + ((50 + focus_speed) / 10);
	focus_far_speed_cmd[3] = '0' + ((50 + focus_speed) % 10);

	fread (&zoom_speed, sizeof (int), 1, config_file);
	if ((zoom_speed < 1) || (zoom_speed > 49)) zoom_speed = 25;
	zoom_wide_speed_cmd[2] = '0' + ((50 - zoom_speed) / 10);
	zoom_wide_speed_cmd[3] = '0' + ((50 - zoom_speed) % 10);
	zoom_tele_speed_cmd[2] = '0' + ((50 + zoom_speed) / 10);
	zoom_tele_speed_cmd[3] = '0' + ((50 + zoom_speed) % 10);

	fread (&send_ip_tally, sizeof (gboolean), 1, config_file);

	fread (&update_notification_address.sin_port, sizeof (guint16), 1, config_file);
	if (ntohs (update_notification_address.sin_port) < 1024) update_notification_address.sin_port = htons (UPDATE_NOTIFICATION_TCP_PORT);

	fread (&sw_p_08_address.sin_port, sizeof (guint16), 1, config_file);
	if ((ntohs (sw_p_08_address.sin_port) < 1024) || (sw_p_08_address.sin_port == update_notification_address.sin_port))
		sw_p_08_address.sin_port = htons (SW_P_08_TCP_PORT);

	fread (&tsl_umd_v5_address.sin_port, sizeof (guint16), 1, config_file);
	if (ntohs (tsl_umd_v5_address.sin_port) < 1024) tsl_umd_v5_address.sin_port = htons (TSL_UMD_V5_UDP_PORT);

	fclose (config_file);
}

void save_config_file (void)
{
	FILE *config_file;
	int i, j, k;
	cameras_set_t *cameras_set_itr;
	ptz_t *ptz;
	int pixbuf_rowstride;
	gsize pixbuf_byte_length;

	config_file = fopen (config_file_name, "wb");

	fwrite (&thumbnail_size, sizeof (gdouble), 1, config_file);

	fwrite (&number_of_cameras_sets, sizeof (int), 1, config_file);

	for (i = 0; i < number_of_cameras_sets; i++)
	{
		cameras_set_itr = cameras_sets;
		while (cameras_set_itr->page_num != i) cameras_set_itr = cameras_set_itr->next;

		fwrite (cameras_set_itr->name, sizeof (char), CAMERAS_SET_NAME_LENGTH, config_file);
		fwrite (&cameras_set_itr->number_of_cameras, sizeof (int), 1, config_file);

		for (j = 0; j < cameras_set_itr->number_of_cameras; j++) {
			ptz = cameras_set_itr->ptz_ptr_array[j];

			fwrite (ptz->name, sizeof (char), 2, config_file);
			fwrite (&ptz->active, sizeof (gboolean), 1, config_file);

			if (ptz->active) {
				fwrite (ptz->ip_address, sizeof (char), 16, config_file);
				fwrite (&ptz->number_of_memories, sizeof (int), 1, config_file);

				for (k = 0; k < MAX_MEMORIES; k++) {
					if (!ptz->memories[k].empty) {
						fwrite (&k, sizeof (int), 1, config_file);
						fwrite (ptz->memories[k].pan_tilt_position_cmd + 4, sizeof (char), 8, config_file);
						fwrite (&ptz->memories[k].zoom_position, sizeof (int), 1, config_file);
						fwrite (ptz->memories[k].zoom_position_hexa, sizeof (char), 3, config_file);
						fwrite (&ptz->memories[k].focus_position, sizeof (int), 1, config_file);
						fwrite (ptz->memories[k].focus_position_hexa, sizeof (char), 3, config_file);
						pixbuf_rowstride = gdk_pixbuf_get_rowstride (ptz->memories[k].pixbuf);
						fwrite (&pixbuf_rowstride, sizeof (int), 1, config_file);
						pixbuf_byte_length = gdk_pixbuf_get_byte_length (ptz->memories[k].pixbuf);
						fwrite (&pixbuf_byte_length, sizeof (gsize), 1, config_file);
						fwrite (gdk_pixbuf_read_pixels (ptz->memories[k].pixbuf), sizeof (guint8), pixbuf_byte_length, config_file);
						fwrite (ptz->memories[k].name, sizeof (char), MEMORIES_NAME_LENGTH, config_file);
					}
				}
			}
		}

		for (j = 0; j < MAX_MEMORIES; j++) {
			fwrite (gtk_label_get_text (GTK_LABEL (cameras_set_itr->memories_labels[j])), sizeof (char), MEMORIES_NAME_LENGTH, config_file);
		}
	}

	fwrite (&controller_is_used, sizeof (gboolean), 1, config_file);

	fwrite (controller_ip_address, sizeof (char), 16, config_file);

	fwrite (&focus_speed, sizeof (int), 1, config_file);

	fwrite (&zoom_speed, sizeof (int), 1, config_file);

	fwrite (&send_ip_tally, sizeof (gboolean), 1, config_file);

	fwrite (&update_notification_address.sin_port, sizeof (guint16), 1, config_file);

	fwrite (&sw_p_08_address.sin_port, sizeof (guint16), 1, config_file);

	fwrite (&tsl_umd_v5_address.sin_port, sizeof (guint16), 1, config_file);

	fclose (config_file);
}

