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


const char cameras_label[] = "Caméras";

GMutex cameras_sets_mutex;

int number_of_cameras_sets = 0;

cameras_set_t *cameras_sets = NULL;

cameras_set_t *current_cameras_set = NULL;
cameras_set_t *new_cameras_set = NULL;
cameras_set_t *cameras_set_with_error = NULL;

GtkWidget *linked_memories_names_entries;
GtkWidget *linked_memories_names_labels;

gboolean show_linked_memories_names_entries;
gboolean show_linked_memories_names_labels;

//cameras_set_configuration_window
GtkWidget *cameras_set_configuration_window;
GtkEntryBuffer *cameras_set_configuration_name_entry_buffer;
int new_number_of_cameras;
int old_number_of_cameras;

char *nb[MAX_CAMERAS] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15" };

GtkWidget *cameras_set_configuration_window_grid;


typedef struct {
	GtkWidget *camera_switch;
	GtkEntryBuffer *name_entry_buffer;
	GtkWidget *name_entry;
	GtkEntryBuffer *ip_entry_buffer[4];
	GtkWidget *ip_entry[4];
	GtkWidget *dot[3];
} camera_configuration_widgets_t;

camera_configuration_widgets_t *cameras_configuration_widgets;


void camera_switch_activated (GtkSwitch *camera_switch, GParamSpec *pspec, camera_configuration_widgets_t *camera_configuration_widgets)
{
	gboolean active;

	active = gtk_switch_get_active (camera_switch);

	gtk_widget_set_sensitive (camera_configuration_widgets->ip_entry[0], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->dot[0], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->ip_entry[1], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->dot[1], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->ip_entry[2], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->dot[2], active);
	gtk_widget_set_sensitive (camera_configuration_widgets->ip_entry[3], active);
}

void number_of_cameras_has_changed (GtkComboBoxText *combo_box_text, cameras_set_t *cameras_set)
{
	int i;
	char *active_string;

	active_string = gtk_combo_box_text_get_active_text (combo_box_text);
	sscanf (active_string, "%d", &new_number_of_cameras);
	g_free (active_string);

	if (new_number_of_cameras < old_number_of_cameras) {
		for (i = old_number_of_cameras - 1; i >= new_number_of_cameras; i--) {
			gtk_widget_hide (cameras_configuration_widgets[i].ip_entry[3]);
			gtk_widget_hide (cameras_configuration_widgets[i].dot[2]);
			gtk_widget_hide (cameras_configuration_widgets[i].ip_entry[2]);
			gtk_widget_hide (cameras_configuration_widgets[i].dot[1]);
			gtk_widget_hide (cameras_configuration_widgets[i].ip_entry[1]);
			gtk_widget_hide (cameras_configuration_widgets[i].dot[0]);
			gtk_widget_hide (cameras_configuration_widgets[i].ip_entry[0]);
			gtk_widget_hide (cameras_configuration_widgets[i].name_entry);
			gtk_widget_hide (cameras_configuration_widgets[i].camera_switch);
		}
	} else if (new_number_of_cameras > old_number_of_cameras) {
		for (i = old_number_of_cameras; i < new_number_of_cameras; i++) {
			gtk_widget_show (cameras_configuration_widgets[i].camera_switch);
			gtk_widget_show (cameras_configuration_widgets[i].name_entry);
			gtk_widget_show (cameras_configuration_widgets[i].ip_entry[0]);
			gtk_widget_show (cameras_configuration_widgets[i].dot[0]);
			gtk_widget_show (cameras_configuration_widgets[i].ip_entry[1]);
			gtk_widget_show (cameras_configuration_widgets[i].dot[1]);
			gtk_widget_show (cameras_configuration_widgets[i].ip_entry[2]);
			gtk_widget_show (cameras_configuration_widgets[i].dot[2]);
			gtk_widget_show (cameras_configuration_widgets[i].ip_entry[3]);
		}
	}

	old_number_of_cameras = new_number_of_cameras;

	gtk_window_resize (GTK_WINDOW (cameras_set_configuration_window), 200, 200);
}

void cameras_set_configuration_window_ok (GtkWidget *button, cameras_set_t *cameras_set)
{
	int i, j, k, ip[4];
	const gchar *entry_buffer_text;
	ptz_t *ptz;
	GtkWidget *widget, *settings_list_box_row;
	cameras_set_t *cameras_set_itr;
	gboolean camera_is_active;
	GSList *ip_addresss_list, *gslist_itr;
	ptz_thread_t *ptz_thread;

	entry_buffer_text = gtk_entry_buffer_get_text (cameras_set_configuration_name_entry_buffer);
	if (strcmp (cameras_set->name, entry_buffer_text) != 0) {
		strcpy (cameras_set->name, entry_buffer_text);
		if (new_cameras_set == NULL) {
			gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (main_window_notebook), cameras_set->page, entry_buffer_text);
			gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (gtk_list_box_get_selected_row (GTK_LIST_BOX (settings_list_box))))), entry_buffer_text);
		}
	}

	g_mutex_lock (&cameras_sets_mutex);

	if (new_cameras_set != NULL) {
		for (i = 0; ((i < cameras_set->number_of_cameras) && (i < new_number_of_cameras)); i++) {
			ptz = cameras_set->ptz_ptr_array[i];

			ptz->active = gtk_switch_get_active (GTK_SWITCH (cameras_configuration_widgets[i].camera_switch));

			entry_buffer_text = gtk_entry_buffer_get_text (cameras_configuration_widgets[i].name_entry_buffer);
			strcpy (ptz->name, entry_buffer_text);

			if (ptz->active) {
				create_ptz_widgets_horizontal (ptz);
				//else create_ptz_widgets_vertical (ptz);
			} else {
				cameras_set->number_of_ghost_cameras++;
				create_ghost_ptz_widgets_horizontal (ptz);
				//else create_ghost_ptz_widgets_vertical (ptz);
			}
		}

		settings_list_box_row = gtk_list_box_row_new ();
		widget = gtk_label_new (cameras_set->name);
		gtk_container_add (GTK_CONTAINER (settings_list_box_row), widget);
		gtk_container_add (GTK_CONTAINER (settings_list_box), settings_list_box_row);
		gtk_widget_show_all (settings_list_box_row);
		gtk_list_box_select_row (GTK_LIST_BOX (settings_list_box), GTK_LIST_BOX_ROW (settings_list_box_row));

		cameras_set->next = cameras_sets;
		cameras_sets = cameras_set;

		if (number_of_cameras_sets == 0) {
			gtk_widget_set_sensitive (thumbnail_size_scale, TRUE);
			gtk_widget_set_sensitive (switch_cameras_on_button, TRUE);
			gtk_widget_set_sensitive (switch_cameras_off_button, TRUE);
		}
		if (number_of_cameras_sets == 1) gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_window_notebook), TRUE);

		number_of_cameras_sets++;
	} else {
		for (i = new_number_of_cameras; i < cameras_set->number_of_cameras; i++) {
			ptz = cameras_set->ptz_ptr_array[i];

			if ((ptz->ip_address_is_valid) && (ptz->error_code != 0x30)) {
				for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
					if (cameras_set == cameras_set_itr) continue;

					for (j = 0; j < cameras_set_itr->number_of_cameras; j++) {
						if (strcmp (cameras_set_itr->ptz_ptr_array[j]->ip_address, ptz->ip_address) == 0) break;
					}
					if (j < cameras_set_itr->number_of_cameras) break;
				}
				if (cameras_set_itr == NULL) {
					send_ptz_control_command (ptz, "#LPC0", TRUE);
					send_update_stop_cmd (ptz);
				}
			}

			if (!ptz->active) cameras_set->number_of_ghost_cameras--;

			if (ptz->name_grid != NULL) {
				gtk_widget_destroy (ptz->name_separator);
				gtk_widget_destroy (ptz->name_grid);
				gtk_widget_destroy (ptz->memories_separator);
				gtk_widget_destroy (ptz->memories_grid);
				if (ptz->active) {
					gtk_widget_destroy (ptz->control_window.window);

					for (j = 0; j < MAX_MEMORIES; j++) {
						if (!ptz->memories[j].empty) {
							g_object_unref (G_OBJECT (ptz->memories[j].full_pixbuf));
							if (thumbnail_width != 320) g_object_unref (G_OBJECT (ptz->memories[j].scaled_pixbuf));
						}
					}
				}
			}

			g_free (ptz);
		}
	}

	if (new_number_of_cameras > cameras_set->number_of_cameras) {
		cameras_set->ptz_ptr_array = g_realloc (cameras_set->ptz_ptr_array, new_number_of_cameras * sizeof (ptz_t*));

		for (i = cameras_set->number_of_cameras; i < new_number_of_cameras; i++) {
			ptz = g_malloc (sizeof (ptz_t));
			cameras_set->ptz_ptr_array[i] = ptz;

			entry_buffer_text = gtk_entry_buffer_get_text (cameras_configuration_widgets[i].name_entry_buffer);
			strcpy (ptz->name, entry_buffer_text);
			ptz->index = i;

			init_ptz (ptz);

			ptz->active = gtk_switch_get_active (GTK_SWITCH (cameras_configuration_widgets[i].camera_switch));

			if (ptz->active) {
				create_ptz_widgets_horizontal (ptz);
				//else 
			} else {
				cameras_set->number_of_ghost_cameras++;
				create_ghost_ptz_widgets_horizontal (ptz);
				//else 
			}

			gtk_widget_show (ptz->name_separator);
			gtk_widget_show_all (ptz->name_grid);
			gtk_widget_show (ptz->memories_separator);
			gtk_widget_show_all (ptz->memories_grid);

			if (new_cameras_set == NULL) {
				gtk_box_pack_start (GTK_BOX (cameras_set->name_grid_box), ptz->name_separator, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (cameras_set->name_grid_box), ptz->name_grid, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (cameras_set->memories_grid_box), ptz->memories_separator, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (cameras_set->memories_grid_box), ptz->memories_grid, FALSE, FALSE, 0);
			}
		}
	}

	cameras_set->number_of_cameras = new_number_of_cameras;

	g_mutex_unlock (&cameras_sets_mutex);

	if (new_cameras_set != NULL) add_cameras_set_to_main_window_notebook (cameras_set);

	ip_addresss_list = NULL;

	for (i = 0; i < new_number_of_cameras; i++) {
		ptz = cameras_set->ptz_ptr_array[i];

		camera_is_active = gtk_switch_get_active (GTK_SWITCH (cameras_configuration_widgets[i].camera_switch));

		if (camera_is_active != ptz->active) {
			if (camera_is_active) {
				ptz->active = TRUE;
				cameras_set->number_of_ghost_cameras--;

				if (ptz->name_grid != NULL) {
					gtk_widget_destroy (ptz->name_grid);
					gtk_widget_destroy (ptz->memories_grid);
				}

				create_ptz_widgets_horizontal (ptz);
				//else 
			} else {
				ptz->active = FALSE;
				cameras_set->number_of_ghost_cameras++;

				if ((ptz->ip_address_is_valid) && (ptz->error_code != 0x30)) {
					for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
						if (cameras_set == cameras_set_itr) continue;

						for (j = 0; j < cameras_set_itr->number_of_cameras; j++) {
							if (strcmp (cameras_set_itr->ptz_ptr_array[j]->ip_address, ptz->ip_address) == 0) break;
						}
						if (j < cameras_set_itr->number_of_cameras) break;
					}
					if (cameras_set_itr == NULL) {
						send_ptz_control_command (ptz, "#LPC0", TRUE);
						send_update_stop_cmd (ptz);
					}
				}

				ptz->ip_address[0] = '\0';
				ptz->ip_address_is_valid = FALSE;

				if (ptz->name_grid != NULL) {
					gtk_widget_destroy (ptz->name_grid);
					gtk_widget_destroy (ptz->memories_grid);
					gtk_widget_destroy (ptz->control_window.window);

					for (j = 0; j < MAX_MEMORIES; j++) {
						if (!ptz->memories[j].empty) {
							g_object_unref (G_OBJECT (ptz->memories[j].full_pixbuf));
							if (thumbnail_width != 320) g_object_unref (G_OBJECT (ptz->memories[j].scaled_pixbuf));
						}
					}
				}

				create_ghost_ptz_widgets_horizontal (ptz);
				//else 
			}

			gtk_widget_show_all (ptz->name_grid);
			gtk_box_pack_start (GTK_BOX (cameras_set->name_grid_box), ptz->name_grid, FALSE, FALSE, 0);
			gtk_box_reorder_child (GTK_BOX (cameras_set->name_grid_box), ptz->name_grid, (ptz->index * 2) + 1);
			gtk_widget_show_all (ptz->memories_grid);
			gtk_box_pack_start (GTK_BOX (cameras_set->memories_grid_box), ptz->memories_grid, FALSE, FALSE, 0);
			gtk_box_reorder_child (GTK_BOX (cameras_set->memories_grid_box), ptz->memories_grid, (ptz->index * 2) + 1);
		}

		entry_buffer_text = gtk_entry_buffer_get_text (cameras_configuration_widgets[i].name_entry_buffer);
		if (strcmp (ptz->name, entry_buffer_text) != 0) {
			strcpy (ptz->name, entry_buffer_text);
			gtk_widget_queue_draw (ptz->name_drawing_area);
		}

		if (!camera_is_active) continue;

		for (j = 0; j < 4; j++)
		{
			if (sscanf (gtk_entry_buffer_get_text (cameras_configuration_widgets[i].ip_entry_buffer[j]), "%d", &ip[j]) != 1) break;
			else if (ip[j] < 0) break;
			else if (ip[j] > 254) break;
		}

		if (j == 4) {
			sprintf (ptz->new_ip_address, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

			for (gslist_itr = ip_addresss_list; gslist_itr != NULL; gslist_itr = gslist_itr->next)
				if (strcmp ((char*)(gslist_itr->data), ptz->new_ip_address) == 0) break;

			if (gslist_itr != NULL) {
				ptz->ip_address_is_valid = FALSE;
				ptz->ip_address[0] = '\0';
				ptz_is_off (ptz);
			} else if (strcmp (ptz->new_ip_address, ptz->ip_address) != 0) {
				if (new_cameras_set == NULL) {
					if ((ptz->ip_address_is_valid) && (ptz->error_code != 0x30)) {
						for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
							if (cameras_set == cameras_set_itr) continue;

							for (k = 0; k < cameras_set_itr->number_of_cameras; k++) {
								if (strcmp (cameras_set_itr->ptz_ptr_array[k]->ip_address, ptz->ip_address) == 0) break;
							}
							if (k < cameras_set_itr->number_of_cameras) break;
						}
						if (cameras_set_itr == NULL) {
							send_ptz_control_command (ptz, "#LPC0", TRUE);
							send_update_stop_cmd (ptz);
						}
					}
				}

				ip_addresss_list = g_slist_prepend (ip_addresss_list, ptz->new_ip_address);

				ptz->ip_address_is_valid = TRUE;
				ptz->address.sin_addr.s_addr = inet_addr (ptz->new_ip_address);
				strcpy (ptz->ip_address, ptz->new_ip_address);

				g_idle_add ((GSourceFunc)ptz_is_off, ptz);

				ptz_thread = g_malloc (sizeof (ptz_thread_t));
				ptz_thread->ptz_ptr = ptz;
				ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)start_ptz, ptz_thread);
			}
		} else {
			ptz->ip_address_is_valid = FALSE;
			ptz->ip_address[0] = '\0';
			ptz_is_off (ptz);
		}
	}

	g_slist_free (ip_addresss_list);

	gtk_widget_destroy (cameras_set_configuration_window);

	gtk_window_set_transient_for (GTK_WINDOW (settings_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (settings_window), TRUE);

	g_free (cameras_configuration_widgets);

	new_cameras_set = NULL;

	backup_needed = TRUE;
}

void cameras_set_configuration_window_cancel (void)
{
	int i;

	gtk_widget_destroy (cameras_set_configuration_window);

	gtk_window_set_transient_for (GTK_WINDOW (settings_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (settings_window), TRUE);

	g_free (cameras_configuration_widgets);

	if (new_cameras_set != NULL) {
		for (i = 0; i < 5; i++) g_free (new_cameras_set->ptz_ptr_array[i]);

		g_free (new_cameras_set->ptz_ptr_array);
		g_free (new_cameras_set);
		new_cameras_set = NULL;
	}
}

gboolean cameras_set_confirmation_window_key_press (GtkWidget *confirmation_window, GdkEventKey *event, cameras_set_t *cameras_set)
{
	if (event->keyval == GDK_KEY_Escape) cameras_set_configuration_window_cancel ();

	return GDK_EVENT_PROPAGATE;
}

void show_cameras_set_configuration_window (void)
{
	gint index;
	cameras_set_t *cameras_set;
	GtkWidget *box1, *box2, *widget, *frame2, *button;
	int i, j, k, l;
	ptz_t *ptz;
	char camera_name[3];

	if (cameras_set_with_error != NULL) {
		cameras_set = cameras_set_with_error;
		cameras_set_with_error = NULL;
	} else if (new_cameras_set == NULL) {
		index = gtk_list_box_row_get_index (gtk_list_box_get_selected_row (GTK_LIST_BOX (settings_list_box)));

		for (cameras_set = cameras_sets; cameras_set != NULL; cameras_set = cameras_set->next) {
			if (cameras_set->page_num == index) break;
		}
	} else cameras_set = new_cameras_set;

	gtk_window_set_transient_for (GTK_WINDOW (settings_window), NULL);
	gtk_window_set_modal (GTK_WINDOW (settings_window), FALSE);

	cameras_set_configuration_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (cameras_set_configuration_window), "Configuration");
	gtk_window_set_type_hint (GTK_WINDOW (cameras_set_configuration_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size (GTK_WINDOW (cameras_set_configuration_window), 50, 50);
	gtk_window_set_modal (GTK_WINDOW (cameras_set_configuration_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (cameras_set_configuration_window), GTK_WINDOW (settings_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (cameras_set_configuration_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (cameras_set_configuration_window), FALSE);
	g_signal_connect (G_OBJECT (cameras_set_configuration_window), "delete-event", G_CALLBACK (cameras_set_configuration_window_cancel), NULL);
	g_signal_connect (G_OBJECT (cameras_set_configuration_window), "key-press-event", G_CALLBACK (cameras_set_confirmation_window_key_press), cameras_set);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box1), MARGIN_VALUE);
		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_bottom (box2, MARGIN_VALUE);
			widget = gtk_label_new ("Nom :");
			gtk_widget_set_margin_end (widget, MARGIN_VALUE);
			gtk_container_add (GTK_CONTAINER (box2), widget);

			cameras_set_configuration_name_entry_buffer = gtk_entry_buffer_new (cameras_set->name, strlen (cameras_set->name));
			widget = gtk_entry_new_with_buffer (cameras_set_configuration_name_entry_buffer);
			gtk_entry_set_max_length (GTK_ENTRY (widget), CAMERAS_SET_NAME_LENGTH);
			gtk_entry_set_width_chars (GTK_ENTRY (widget), CAMERAS_SET_NAME_LENGTH);
			gtk_container_add (GTK_CONTAINER (box2), widget);
		gtk_container_add (GTK_CONTAINER (box1), box2);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_bottom (box2, MARGIN_VALUE);
			widget = gtk_label_new ("Nombre de caméras :");
			gtk_widget_set_margin_end (widget, MARGIN_VALUE);
			gtk_container_add (GTK_CONTAINER (box2), widget);

			widget = gtk_combo_box_text_new ();
			for (i = 0; i < MAX_CAMERAS; i++) {
				gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), nb[i]);
			}
			gtk_combo_box_set_active (GTK_COMBO_BOX (widget), cameras_set->number_of_cameras - 1);
			g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (number_of_cameras_has_changed), cameras_set);
			gtk_container_add (GTK_CONTAINER (box2), widget);
		gtk_container_add (GTK_CONTAINER (box1), box2);

		frame2 = gtk_frame_new (cameras_label);
		gtk_frame_set_label_align (GTK_FRAME (frame2), 0.5, 0.5);
		gtk_widget_set_margin_bottom (frame2, MARGIN_VALUE);

		cameras_set_configuration_window_grid = gtk_grid_new ();
		gtk_widget_set_margin_start (cameras_set_configuration_window_grid, MARGIN_VALUE);
		gtk_widget_set_margin_end (cameras_set_configuration_window_grid, MARGIN_VALUE);

		widget = gtk_label_new ("Nom :");
		gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), widget, 1, 0, 1, 1);

		widget = gtk_label_new ("address IP :");
		gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), widget, 2, 0, 7, 1);

		cameras_configuration_widgets = g_malloc (MAX_CAMERAS * sizeof (camera_configuration_widgets_t));
		new_number_of_cameras = cameras_set->number_of_cameras;
		old_number_of_cameras = cameras_set->number_of_cameras;

		for (i = 0, j = 1; i < cameras_set->number_of_cameras; i++, j++) {
			ptz = cameras_set->ptz_ptr_array[i];

			widget = gtk_switch_new ();
			gtk_switch_set_active (GTK_SWITCH (widget), ptz->active);
			g_signal_connect (widget, "notify::active", G_CALLBACK (camera_switch_activated), &cameras_configuration_widgets[i]);
			gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), widget, 0, j, 1, 1);
			cameras_configuration_widgets[i].camera_switch = widget;

			cameras_configuration_widgets[i].name_entry_buffer = gtk_entry_buffer_new (ptz->name, strlen (ptz->name));
			cameras_configuration_widgets[i].name_entry = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].name_entry_buffer);
			gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 2);
			gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 4);
			gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 0.5);
			gtk_widget_set_margin_start (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
			gtk_widget_set_margin_end (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].name_entry, 1, j, 1, 1);

			if (ptz->ip_address_is_valid) {
				for (l = 1; ptz->ip_address[l] != '.'; l++) {}
				cameras_configuration_widgets[i].ip_entry_buffer[0] = gtk_entry_buffer_new (ptz->ip_address, l);
				k = l + 1;
			} else cameras_configuration_widgets[i].ip_entry_buffer[0] = gtk_entry_buffer_new (network_address[0], network_address_len[0]);

			cameras_configuration_widgets[i].ip_entry[0] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[0]);
			gtk_entry_set_input_purpose (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), GTK_INPUT_PURPOSE_DIGITS);
			g_signal_connect (G_OBJECT (cameras_configuration_widgets[i].ip_entry[0]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
			gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 3);
			gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 3);
			gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 0.5);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[0], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[0], 2, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].ip_entry[0], FALSE);

			cameras_configuration_widgets[i].dot[0] = gtk_label_new (".");
			gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[0], 2);
			gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[0], 2);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[0], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[0], 3, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].dot[0], FALSE);

			if (ptz->ip_address_is_valid) {
				for (l = 1; ptz->ip_address[k+l] != '.'; l++) {}
				cameras_configuration_widgets[i].ip_entry_buffer[1] = gtk_entry_buffer_new (ptz->ip_address + k, l);
				k = k + l + 1;
			} else cameras_configuration_widgets[i].ip_entry_buffer[1] = gtk_entry_buffer_new (network_address[1], network_address_len[1]);

			cameras_configuration_widgets[i].ip_entry[1] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[1]);
			gtk_entry_set_input_purpose (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), GTK_INPUT_PURPOSE_DIGITS);
			g_signal_connect (G_OBJECT (cameras_configuration_widgets[i].ip_entry[1]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
			gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 3);
			gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 3);
			gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 0.5);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[1], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[1], 4, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].ip_entry[1], FALSE);

			cameras_configuration_widgets[i].dot[1] = gtk_label_new (".");
			gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[1], 2);
			gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[1], 2);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[1], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[1], 5, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].dot[1], FALSE);

			if (ptz->ip_address_is_valid) {
				for (l = 1; ptz->ip_address[k+l] != '.'; l++) {}
				cameras_configuration_widgets[i].ip_entry_buffer[2] = gtk_entry_buffer_new (ptz->ip_address + k, l);
				k = k + l + 1;
			} else cameras_configuration_widgets[i].ip_entry_buffer[2] = gtk_entry_buffer_new (network_address[2], network_address_len[2]);

			cameras_configuration_widgets[i].ip_entry[2] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[2]);
			gtk_entry_set_input_purpose (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), GTK_INPUT_PURPOSE_DIGITS);
			g_signal_connect (G_OBJECT (cameras_configuration_widgets[i].ip_entry[2]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
			gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 3);
			gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 3);
			gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 0.5);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[2], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[2], 6, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].ip_entry[2], FALSE);

			cameras_configuration_widgets[i].dot[2] = gtk_label_new (".");
			gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[2], 2);
			gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[2], 2);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[2], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[2], 7, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].dot[2], FALSE);

			if (ptz->ip_address_is_valid) {
				for (l = 1; ptz->ip_address[k+l] != '\0'; l++) {}
				cameras_configuration_widgets[i].ip_entry_buffer[3] = gtk_entry_buffer_new (ptz->ip_address + k, l);
			} else cameras_configuration_widgets[i].ip_entry_buffer[3] = gtk_entry_buffer_new (NULL, -1);

			cameras_configuration_widgets[i].ip_entry[3] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[3]);
			gtk_entry_set_input_purpose (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), GTK_INPUT_PURPOSE_DIGITS);
			g_signal_connect (G_OBJECT (cameras_configuration_widgets[i].ip_entry[3]), "key-press-event", G_CALLBACK (digit_key_press), NULL);
			gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 3);
			gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 3);
			gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 0.5);
			gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[3], MARGIN_VALUE);
			gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[3], 8, j, 1, 1);
			if (!ptz->active) gtk_widget_set_sensitive (cameras_configuration_widgets[i].ip_entry[3], FALSE);
		}
		gtk_container_add (GTK_CONTAINER (frame2), cameras_set_configuration_window_grid);
		gtk_container_add (GTK_CONTAINER (box1), frame2);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box2, GTK_ALIGN_CENTER);
			button = gtk_button_new_with_label ("OK");
			gtk_widget_set_margin_end (button, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cameras_set_configuration_window_ok), cameras_set);
			gtk_box_pack_start (GTK_BOX (box2), button, FALSE, FALSE, 0);

			button = gtk_button_new_with_label ("Annuler");
			g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cameras_set_configuration_window_cancel), NULL);
			gtk_box_pack_start (GTK_BOX (box2), button, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (box1), box2);
	gtk_container_add (GTK_CONTAINER (cameras_set_configuration_window), box1);

	gtk_window_set_resizable (GTK_WINDOW (cameras_set_configuration_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (cameras_set_configuration_window), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_widget_show_all (cameras_set_configuration_window);

	while (i < MAX_CAMERAS) {
		cameras_configuration_widgets[i].camera_switch = gtk_switch_new ();
		gtk_switch_set_active (GTK_SWITCH (cameras_configuration_widgets[i].camera_switch), TRUE);
		g_signal_connect (cameras_configuration_widgets[i].camera_switch, "notify::active", G_CALLBACK (camera_switch_activated), &cameras_configuration_widgets[i]);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].camera_switch, MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].camera_switch, 0, j, 1, 1);

		sprintf (camera_name, "%d", j);
		cameras_configuration_widgets[i].name_entry_buffer = gtk_entry_buffer_new (camera_name, strlen (camera_name));
		cameras_configuration_widgets[i].name_entry = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].name_entry_buffer);
		gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 2);
		gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 4);
		gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].name_entry), 0.5);
		gtk_widget_set_margin_start (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
		gtk_widget_set_margin_end (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].name_entry, MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].name_entry, 1, j, 1, 1);

		cameras_configuration_widgets[i].ip_entry_buffer[0] = gtk_entry_buffer_new (network_address[0], network_address_len[0]);
		cameras_configuration_widgets[i].ip_entry[0] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[0]);
		gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 3);
		gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 3);
		gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[0]), 0.5);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[0], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[0], 2, j, 1, 1);

		cameras_configuration_widgets[i].dot[0] = gtk_label_new (".");
		gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[0], 2);
		gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[0], 2);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[0], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[0], 3, j, 1, 1);

		cameras_configuration_widgets[i].ip_entry_buffer[1] = gtk_entry_buffer_new (network_address[1], network_address_len[1]);
		cameras_configuration_widgets[i].ip_entry[1] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[1]);
		gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 3);
		gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 3);
		gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[1]), 0.5);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[1], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[1], 4, j, 1, 1);

		cameras_configuration_widgets[i].dot[1] = gtk_label_new (".");
		gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[1], 2);
		gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[1], 2);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[1], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[1], 5, j, 1, 1);

		cameras_configuration_widgets[i].ip_entry_buffer[2] = gtk_entry_buffer_new (network_address[2], network_address_len[2]);
		cameras_configuration_widgets[i].ip_entry[2] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[2]);
		gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 3);
		gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 3);
		gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[2]), 0.5);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[2], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[2], 6, j, 1, 1);

		cameras_configuration_widgets[i].dot[2] = gtk_label_new (".");
		gtk_widget_set_margin_start (cameras_configuration_widgets[i].dot[2], 2);
		gtk_widget_set_margin_end (cameras_configuration_widgets[i].dot[2], 2);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].dot[2], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].dot[2], 7, j, 1, 1);

		cameras_configuration_widgets[i].ip_entry_buffer[3] = gtk_entry_buffer_new (NULL, -1);
		cameras_configuration_widgets[i].ip_entry[3] = gtk_entry_new_with_buffer (cameras_configuration_widgets[i].ip_entry_buffer[3]);
		gtk_entry_set_max_length (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 3);
		gtk_entry_set_width_chars (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 3);
		gtk_entry_set_alignment (GTK_ENTRY (cameras_configuration_widgets[i].ip_entry[3]), 0.5);
		gtk_widget_set_margin_bottom (cameras_configuration_widgets[i].ip_entry[3], MARGIN_VALUE);
		gtk_grid_attach (GTK_GRID (cameras_set_configuration_window_grid), cameras_configuration_widgets[i].ip_entry[3], 8, j, 1, 1);

		i++;
		j++;
	}
}

void add_cameras_set (void)
{
	int i;
	ptz_t *ptz;

	new_cameras_set = g_malloc (sizeof (cameras_set_t));
	new_cameras_set->name[0] = '\0';
	new_cameras_set->number_of_cameras = 5;
	new_cameras_set->ptz_ptr_array = g_malloc (5 * sizeof (ptz_t*));
	new_cameras_set->number_of_ghost_cameras = 0;
	new_cameras_set->thumbnail_width = thumbnail_width;

	for (i = 0; i < 5; i++) {
		ptz = g_malloc (sizeof (ptz_t));
		new_cameras_set->ptz_ptr_array[i] = ptz;
		sprintf (ptz->name, "%d", i + 1);
		ptz->index = i;
		init_ptz (ptz);
	}

	show_cameras_set_configuration_window ();
}

void delete_cameras_set (void)
{
	GtkListBoxRow *list_box_row;
	const gchar *name;
	cameras_set_t *cameras_set_itr, *cameras_set_prev,  *other_cameras_set;
	int i, j;
	ptz_t *ptz;

	list_box_row = gtk_list_box_get_selected_row (GTK_LIST_BOX (settings_list_box));

	if (list_box_row != NULL) {
		name = gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (list_box_row))));

		g_mutex_lock (&cameras_sets_mutex);

		if (strcmp (cameras_sets->name, name) == 0) {
			cameras_set_itr = cameras_sets;
			cameras_sets = cameras_sets->next;
		} else {
			cameras_set_prev = cameras_sets;
			cameras_set_itr = cameras_sets->next;

			while (cameras_set_itr != NULL) {
				if (strcmp (cameras_set_itr->name, name) == 0) {
					cameras_set_prev->next = cameras_set_itr->next;
					break;
				}
				cameras_set_prev = cameras_set_itr;
				cameras_set_itr = cameras_set_itr->next;
			}
		}

		g_mutex_unlock (&cameras_sets_mutex);

		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			ptz = cameras_set_itr->ptz_ptr_array[i];

			if ((ptz->ip_address_is_valid) && (ptz->error_code != 0x30)) {
				for (other_cameras_set = cameras_sets; other_cameras_set != NULL; other_cameras_set = other_cameras_set->next) {
					if (other_cameras_set == cameras_set_itr) continue;

					for (j = 0; j < other_cameras_set->number_of_cameras; j++) {
						if (strcmp (other_cameras_set->ptz_ptr_array[j]->ip_address, ptz->ip_address) == 0) break;
					}
					if (j < other_cameras_set->number_of_cameras) break;
				}

				if (other_cameras_set == NULL) {
					send_ptz_control_command (ptz, "#LPC0", TRUE);
					send_update_stop_cmd (ptz);
				}
			}

			if (ptz->active) gtk_widget_destroy (ptz->control_window.window);

			for (j = 0; j < MAX_MEMORIES; j++) {
				if (!ptz->memories[j].empty) {
					g_object_unref (G_OBJECT (ptz->memories[j].full_pixbuf));
					if (thumbnail_width != 320) g_object_unref (G_OBJECT (ptz->memories[j].scaled_pixbuf));
				}
			}

			g_free (ptz);
		}
		g_free (cameras_set_itr->ptz_ptr_array);

		gtk_notebook_remove_page (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page_num);

		gtk_list_box_select_row (GTK_LIST_BOX (settings_list_box), NULL);
		gtk_widget_destroy (GTK_WIDGET (list_box_row));

		g_free (cameras_set_itr);

		if (number_of_cameras_sets == MAX_CAMERAS_SET) gtk_widget_set_sensitive (settings_new_button, TRUE);
		number_of_cameras_sets--;

		if (number_of_cameras_sets == 1) gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_window_notebook), FALSE);

		if (number_of_cameras_sets == 0) {
			current_cameras_set = NULL;
			gtk_widget_set_sensitive (thumbnail_size_scale, FALSE);
			gtk_widget_set_sensitive (switch_cameras_on_button, FALSE);
			gtk_widget_set_sensitive (switch_cameras_off_button, FALSE);
		}

		for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next)
			cameras_set_itr->page_num = gtk_notebook_page_num (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page);
	}

	backup_needed = TRUE;
}

void entry_activate (GtkEntry *entry, GtkLabel *label)
{
	gtk_label_set_text (label, gtk_entry_get_text (entry));

	backup_needed = TRUE;
}

void entry_scrolled_window_hadjustment_value_changed (GtkAdjustment *hadjustment, cameras_set_t *cameras_set)
{
	gdouble value = gtk_adjustment_get_value (hadjustment);

	gtk_adjustment_set_value (cameras_set->memories_scrolled_window_hadjustment, value);
	gtk_adjustment_set_value (cameras_set->label_scrolled_window_hadjustment, value);
}

void memories_scrolled_window_hadjustment_value_changed (GtkAdjustment *hadjustment, cameras_set_t *cameras_set)
{
	gdouble value = gtk_adjustment_get_value (hadjustment);

	gtk_adjustment_set_value (cameras_set->entry_scrolled_window_hadjustment, value);
	gtk_adjustment_set_value (cameras_set->label_scrolled_window_hadjustment, value);
}

void label_scrolled_window_hadjustment_value_changed (GtkAdjustment *hadjustment, cameras_set_t *cameras_set)
{
	gdouble value = gtk_adjustment_get_value (hadjustment);

	gtk_adjustment_set_value (cameras_set->memories_scrolled_window_hadjustment, value);
	gtk_adjustment_set_value (cameras_set->entry_scrolled_window_hadjustment, value);
}

void create_horizontal_linked_memories_names_entries (cameras_set_t *cameras_set)
{
	int i;
	GtkWidget *box, *scrolled_window, *widget;

	linked_memories_names_entries = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		cameras_set->entry_widgets_padding = gtk_drawing_area_new ();
		gtk_widget_set_size_request (cameras_set->entry_widgets_padding, thumbnail_height + 13, 34);
	gtk_box_pack_start (GTK_BOX (linked_memories_names_entries), cameras_set->entry_widgets_padding, FALSE, FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 30);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
			box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			for (i = 0; i < MAX_MEMORIES; i++) {
				cameras_set->entry_widgets[i] = gtk_entry_new ();
				gtk_widget_set_size_request (cameras_set->entry_widgets[i], thumbnail_width + 6, 34);
				gtk_widget_set_margin_start (cameras_set->entry_widgets[i], 2);
				gtk_widget_set_margin_end (cameras_set->entry_widgets[i], 2);
				gtk_entry_set_max_length (GTK_ENTRY (cameras_set->entry_widgets[i]), MEMORIES_NAME_LENGTH);
				gtk_entry_set_width_chars (GTK_ENTRY (cameras_set->entry_widgets[i]), MEMORIES_NAME_LENGTH);
				gtk_entry_set_alignment (GTK_ENTRY (cameras_set->entry_widgets[i]), 0.5);
				gtk_box_pack_start (GTK_BOX (box), cameras_set->entry_widgets[i], FALSE, FALSE, 0);
			}
				widget = gtk_drawing_area_new ();
				gtk_widget_set_size_request (widget, 4, 34);
			gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (scrolled_window), box);
		cameras_set->entry_scrolled_window_hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
		g_signal_connect (G_OBJECT (cameras_set->entry_scrolled_window_hadjustment), "value-changed", G_CALLBACK (entry_scrolled_window_hadjustment_value_changed), cameras_set);
	gtk_box_pack_start (GTK_BOX (linked_memories_names_entries), scrolled_window, TRUE, TRUE, 0);
}

void create_horizontal_linked_memories_names_labels (cameras_set_t *cameras_set)
{
	int i;
	GtkWidget *box, *scrolled_window, *widget;

	linked_memories_names_labels = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		cameras_set->memories_labels_padding = gtk_drawing_area_new ();
		gtk_widget_set_size_request (cameras_set->memories_labels_padding, thumbnail_height + 13, 10);
	gtk_box_pack_start (GTK_BOX (linked_memories_names_labels), cameras_set->memories_labels_padding, FALSE, FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolled_window), GTK_CORNER_BOTTOM_LEFT);
		gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 50);
			box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			for (i = 0; i < MAX_MEMORIES; i++) {
				cameras_set->memories_labels[i] = gtk_label_new (NULL);
				gtk_widget_set_size_request (cameras_set->memories_labels[i], thumbnail_width + 6, 10);
				gtk_widget_set_margin_start (cameras_set->memories_labels[i], 2);
				gtk_widget_set_margin_end (cameras_set->memories_labels[i], 2);
				gtk_label_set_xalign (GTK_LABEL (cameras_set->memories_labels[i]), 0.5);
				gtk_box_pack_start (GTK_BOX (box), cameras_set->memories_labels[i], FALSE, FALSE, 0);
				g_signal_connect (G_OBJECT (cameras_set->entry_widgets[i]), "activate", G_CALLBACK (entry_activate), cameras_set->memories_labels[i]);
			}
				widget = gtk_drawing_area_new ();
				gtk_widget_set_size_request (widget, 4, 10);
			gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (scrolled_window), box);
		cameras_set->label_scrolled_window_hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
		g_signal_connect (G_OBJECT (cameras_set->label_scrolled_window_hadjustment), "value-changed", G_CALLBACK (label_scrolled_window_hadjustment_value_changed), cameras_set);
	gtk_box_pack_start (GTK_BOX (linked_memories_names_labels), scrolled_window, TRUE, TRUE, 0);
}

void add_cameras_set_to_main_window_notebook (cameras_set_t *cameras_set)
{
	int i;
	GtkWidget *box1, *scrolled_window, *widget, *memories_scrolled_window;

	cameras_set->page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		create_horizontal_linked_memories_names_entries (cameras_set);
		//else create_vertical_linked_memories_names_entries (cameras_set);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), linked_memories_names_entries, FALSE, FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				cameras_set->name_grid_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

				memories_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (memories_scrolled_window), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
					cameras_set->memories_grid_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
					for (i = 0; i < cameras_set->number_of_cameras; i++) {
						gtk_box_pack_start (GTK_BOX (cameras_set->name_grid_box), cameras_set->ptz_ptr_array[i]->name_separator, FALSE, FALSE, 0);
						gtk_box_pack_start (GTK_BOX (cameras_set->name_grid_box), cameras_set->ptz_ptr_array[i]->name_grid, FALSE, FALSE, 0);
						gtk_box_pack_start (GTK_BOX (cameras_set->memories_grid_box), cameras_set->ptz_ptr_array[i]->memories_separator, FALSE, FALSE, 0);
						gtk_box_pack_start (GTK_BOX (cameras_set->memories_grid_box), cameras_set->ptz_ptr_array[i]->memories_grid, FALSE, FALSE, 0);
					}
				gtk_container_add (GTK_CONTAINER (memories_scrolled_window), cameras_set->memories_grid_box);
				cameras_set->memories_scrolled_window_hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (memories_scrolled_window));
				g_signal_connect (G_OBJECT (cameras_set->memories_scrolled_window_hadjustment), "value-changed", G_CALLBACK (memories_scrolled_window_hadjustment_value_changed), cameras_set);
			gtk_box_pack_start (GTK_BOX (box1), cameras_set->name_grid_box, FALSE, FALSE, 0);

				widget = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
			gtk_box_pack_start (GTK_BOX (box1), widget, FALSE, FALSE, 0);

			gtk_box_pack_start (GTK_BOX (box1), memories_scrolled_window, TRUE, TRUE, 0);
		cameras_set->scrolled_window_vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
		gtk_container_add (GTK_CONTAINER (scrolled_window), box1);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), scrolled_window, TRUE, TRUE, 0);

		widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), widget, FALSE, FALSE, 0);

		create_horizontal_linked_memories_names_labels (cameras_set);
		//else create_vertical_linked_memories_names_labels (cameras_set);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), linked_memories_names_labels, FALSE, FALSE, 0);

	gtk_widget_show_all (cameras_set->page);

	if (!show_linked_memories_names_entries) gtk_widget_hide (linked_memories_names_entries);
	if (!show_linked_memories_names_labels) gtk_widget_hide (linked_memories_names_labels);

	widget = gtk_label_new (cameras_set->name);
	cameras_set->page_num = gtk_notebook_append_page (GTK_NOTEBOOK (main_window_notebook), cameras_set->page, widget);
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (main_window_notebook), cameras_set->page, TRUE);
}

