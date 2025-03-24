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

#include "interface.h"

#include "cameras_set.h"
#include "main_window.h"
#include "settings.h"


const char interface_settings_txt[] = "_Interface";
const char pixel_txt[] = "pixel";
const char pixels_txt[] = "pixels";

GtkWidget *interface_settings_window;

GtkWidget *orientation_check_button;
GtkWidget *show_linked_memories_names_entries_check_button;
GtkWidget *show_linked_memories_names_labels_check_button;
GtkWidget *thumbnail_size_scale;
GtkWidget *memories_button_vertical_margins_scale, *vertical_margins_label;
GtkWidget *memories_button_horizontal_margins_scale, *horizontal_margins_label;

gulong orientation_check_button_handler_id;
gulong show_linked_memories_names_entries_check_button_handler_id;
gulong show_linked_memories_names_labels_check_button_handler_id;
gulong thumbnail_size_scale_handler_id;
gulong memories_button_vertical_margins_scale_handler_id;
gulong memories_button_horizontal_margins_scale_handler_id;

//GtkWidget *memories_name_red_scale, *horizontal_margins_label;


void orientation_check_button_toggled (GtkToggleButton *togglebutton)
{
	int i;
	char memories_name[MAX_MEMORIES][MEMORIES_NAME_LENGTH + 1];

	current_cameras_set->orientation = gtk_toggle_button_get_active (togglebutton);

	for (i = 0; i < MAX_MEMORIES; i++) {
		memcpy (memories_name[i], gtk_label_get_text (GTK_LABEL (current_cameras_set->memories_labels[i])), MEMORIES_NAME_LENGTH);
		memories_name[i][MEMORIES_NAME_LENGTH] = '\0';
	}

	gtk_widget_destroy (current_cameras_set->page_box);

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		if (current_cameras_set->cameras[i]->active) {
			if (current_cameras_set->orientation) create_ptz_widgets_horizontal (current_cameras_set->cameras[i], current_cameras_set->thumbnail_width, current_cameras_set->thumbnail_height);
			else create_ptz_widgets_vertical (current_cameras_set->cameras[i], current_cameras_set->thumbnail_width, current_cameras_set->thumbnail_height);

			if (!current_cameras_set->cameras[i]->is_on) {
				gtk_widget_set_sensitive (current_cameras_set->cameras[i]->name_grid, FALSE);
				gtk_widget_set_sensitive (current_cameras_set->cameras[i]->memories_grid, FALSE);
			}
		} else {
			if (current_cameras_set->orientation) create_ghost_ptz_widgets_horizontal (current_cameras_set->cameras[i], current_cameras_set->thumbnail_width, current_cameras_set->thumbnail_height);
			else create_ghost_ptz_widgets_vertical (current_cameras_set->cameras[i], current_cameras_set->thumbnail_width, current_cameras_set->thumbnail_height);
		}
	}

	fill_cameras_set_page (current_cameras_set);

	for (i = 0; i < MAX_MEMORIES; i++) {
		gtk_entry_set_text (GTK_ENTRY (current_cameras_set->entry_widgets[i]), memories_name[i]);
		gtk_label_set_text (GTK_LABEL (current_cameras_set->memories_labels[i]), memories_name[i]);
	}

	gtk_widget_show_all (current_cameras_set->page);

	if (!current_cameras_set->show_linked_memories_names_entries) gtk_widget_hide (current_cameras_set->linked_memories_names_entries);
	if (!current_cameras_set->show_linked_memories_names_labels) gtk_widget_hide (current_cameras_set->linked_memories_names_labels);


	backup_needed = TRUE;
}

void show_linked_memories_names_entries_check_button_toggled (GtkToggleButton *togglebutton)
{
	current_cameras_set->show_linked_memories_names_entries = gtk_toggle_button_get_active (togglebutton);

	if (current_cameras_set->show_linked_memories_names_entries) gtk_widget_show (current_cameras_set->linked_memories_names_entries);
    else gtk_widget_hide (current_cameras_set->linked_memories_names_entries);

	backup_needed = TRUE;
}

void show_linked_memories_names_labels_check_button_toggled (GtkToggleButton *togglebutton)
{
	current_cameras_set->show_linked_memories_names_labels = gtk_toggle_button_get_active (togglebutton);

	if (current_cameras_set->show_linked_memories_names_labels) gtk_widget_show (current_cameras_set->linked_memories_names_labels);
    else gtk_widget_hide (current_cameras_set->linked_memories_names_labels);

	backup_needed = TRUE;
}

void thumbnail_size_value_changed (GtkRange *range)
{
	int old_thumbnail_width;
	int i, j;
	ptz_t *ptz;

    old_thumbnail_width = current_cameras_set->thumbnail_width;

	current_cameras_set->thumbnail_size = gtk_range_get_value (range);
	current_cameras_set->thumbnail_width = 320 * current_cameras_set->thumbnail_size;
	current_cameras_set->thumbnail_height = 180 * current_cameras_set->thumbnail_size;

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		ptz = current_cameras_set->cameras[i];

		if (ptz->active) {
			if (current_cameras_set->orientation) {
				gtk_widget_set_size_request (ptz->name_drawing_area, current_cameras_set->thumbnail_height, current_cameras_set->thumbnail_height + 10);
				gtk_widget_set_size_request (ptz->error_drawing_area, 8, current_cameras_set->thumbnail_height + 10);
			} else {
				gtk_widget_set_size_request (ptz->name_drawing_area, current_cameras_set->thumbnail_width + 10, current_cameras_set->thumbnail_height);
				gtk_widget_set_size_request (ptz->error_drawing_area, current_cameras_set->thumbnail_width + 10, 8);
			}

			for (j = 0; j < MAX_MEMORIES; j++) {
				if (!ptz->memories[j].empty) {
					gtk_widget_destroy (ptz->memories[j].image);

					gtk_widget_set_size_request (ptz->memories[j].button, current_cameras_set->thumbnail_width + 10, current_cameras_set->thumbnail_height + 10);

					if (old_thumbnail_width != 320) g_object_unref (G_OBJECT (ptz->memories[j].scaled_pixbuf));

					if (current_cameras_set->thumbnail_width == 320) ptz->memories[j].image = gtk_image_new_from_pixbuf (ptz->memories[j].full_pixbuf);
					else {
						ptz->memories[j].scaled_pixbuf = gdk_pixbuf_scale_simple (ptz->memories[j].full_pixbuf, current_cameras_set->thumbnail_width, current_cameras_set->thumbnail_height, GDK_INTERP_BILINEAR);
						ptz->memories[j].image = gtk_image_new_from_pixbuf (ptz->memories[j].scaled_pixbuf);
					}
					gtk_button_set_image (GTK_BUTTON (ptz->memories[j].button), ptz->memories[j].image);
				} else gtk_widget_set_size_request (ptz->memories[j].button, current_cameras_set->thumbnail_width + 10, current_cameras_set->thumbnail_height + 10);
			}
		} else {
			if (current_cameras_set->orientation) {
				gtk_widget_set_size_request (ptz->name_drawing_area, current_cameras_set->thumbnail_height + 8, current_cameras_set->thumbnail_height / 2);
				gtk_widget_set_size_request (ptz->ghost_body, ((current_cameras_set->thumbnail_width + 10) + (2 * current_cameras_set->memories_button_vertical_margins)) * MAX_MEMORIES, current_cameras_set->thumbnail_height / 2);
			} else {
				gtk_widget_set_size_request (ptz->name_drawing_area, current_cameras_set->thumbnail_height / 2, current_cameras_set->thumbnail_height + 8);
				gtk_widget_set_size_request (ptz->ghost_body, current_cameras_set->thumbnail_height / 2, ((current_cameras_set->thumbnail_height + 10) + (2 * current_cameras_set->memories_button_horizontal_margins)) * MAX_MEMORIES);
			}
		}

		if (current_cameras_set->orientation) {
			gtk_widget_set_size_request (current_cameras_set->entry_widgets_padding, current_cameras_set->thumbnail_height + 12, 34);
			gtk_widget_set_size_request (current_cameras_set->memories_labels_padding, current_cameras_set->thumbnail_height + 12, 20);
			gtk_widget_set_size_request (current_cameras_set->memories_scrollbar_padding, current_cameras_set->thumbnail_height + 12, 10);
		} else {
			gtk_widget_set_size_request (current_cameras_set->entry_widgets_padding, 34, current_cameras_set->thumbnail_height + 12);
			gtk_widget_set_size_request (current_cameras_set->memories_labels_padding, 20, current_cameras_set->thumbnail_height + 12);
			gtk_widget_set_size_request (current_cameras_set->memories_scrollbar_padding, 10, current_cameras_set->thumbnail_height + 12);
		}

		for (j = 0; j < MAX_MEMORIES; j++) {
			if (current_cameras_set->orientation) {
				gtk_widget_set_size_request (current_cameras_set->entry_widgets[j], current_cameras_set->thumbnail_width + 6, 34);
				gtk_widget_set_size_request (current_cameras_set->memories_labels[j], current_cameras_set->thumbnail_width + 6, 20);
			} else {
				gtk_widget_set_size_request (current_cameras_set->entry_widgets[j], 34, current_cameras_set->thumbnail_width + 10);
				gtk_widget_set_size_request (current_cameras_set->memories_labels[j], 20, current_cameras_set->thumbnail_height + 10);
			}
		}
	}

	backup_needed = TRUE;
}

void memories_button_vertical_margins_value_changed (GtkRange *range)
{
	int new_memories_button_vertical_margins = gtk_range_get_value (range);

	if ((new_memories_button_vertical_margins <= 1) && (current_cameras_set->memories_button_vertical_margins > 1))  gtk_label_set_text (GTK_LABEL (vertical_margins_label), pixel_txt);
	else if ((new_memories_button_vertical_margins > 1) && (current_cameras_set->memories_button_vertical_margins <= 1)) gtk_label_set_text (GTK_LABEL (vertical_margins_label), pixels_txt);

	current_cameras_set->memories_button_vertical_margins = new_memories_button_vertical_margins;

	update_current_cameras_set_vertical_margins ();

	backup_needed = TRUE;
}

void memories_button_horizontal_margins_value_changed (GtkRange *range)
{
	int new_memories_button_horizontal_margins = gtk_range_get_value (range);

	if ((new_memories_button_horizontal_margins <= 1) && (current_cameras_set->memories_button_horizontal_margins > 1)) gtk_label_set_text (GTK_LABEL (horizontal_margins_label), pixel_txt);
	else if ((new_memories_button_horizontal_margins > 1) && (current_cameras_set->memories_button_horizontal_margins <= 1)) gtk_label_set_text (GTK_LABEL (horizontal_margins_label), pixels_txt);

	current_cameras_set->memories_button_horizontal_margins = new_memories_button_horizontal_margins;

	update_current_cameras_set_horizontal_margins ();

	backup_needed = TRUE;
}
















/*


gboolean hide_popup_window (GtkWidget *window)
{
	gtk_widget_hide (window);
	popup_window = NULL;

	gtk_event_box_set_above_child (GTK_EVENT_BOX (rcp_vision->event_box), FALSE);
	gtk_event_box_set_above_child (GTK_EVENT_BOX (((cameras_set_t*)(rcp_vision->current_cameras_set))->master_rcp.root_widget), FALSE);

	return GDK_EVENT_STOP;
}

gboolean popup_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) hide_popup_window (window);

	return GDK_EVENT_STOP;
}

				box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					widget = gtk_label_new ("-");
				gtk_box_pack_start (GTK_BOX (box4), widget, FALSE, FALSE, 0);

					thumbnail_size_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.5, 1.0, 0.1);
					gtk_scale_set_draw_value (GTK_SCALE (thumbnail_size_scale), FALSE);
					gtk_scale_set_has_origin (GTK_SCALE (thumbnail_size_scale), FALSE);
//					gtk_widget_set_size_request (thumbnail_size_scale, 200, 10);
					gtk_widget_set_margin_start (thumbnail_size_scale, 10);
					gtk_widget_set_margin_end (thumbnail_size_scale, 10);
				gtk_box_pack_start (GTK_BOX (box4), thumbnail_size_scale, FALSE, FALSE, 0);

					widget = gtk_label_new ("+");
				gtk_box_pack_start (GTK_BOX (box4), widget, FALSE, FALSE, 0);

					widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 20.0, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (widget), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (widget), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (widget), FALSE);
					gtk_range_set_value (GTK_RANGE (widget), memories_button_vertical_margins);
					g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (memories_button_vertical_margins_value_changed), label);
				gtk_box_pack_start (GTK_BOX (box3), widget, TRUE, TRUE, 0);
*/

void show_interface_settings_window (void)
{
	g_signal_handler_block (orientation_check_button, orientation_check_button_handler_id);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (orientation_check_button), current_cameras_set->orientation);
	g_signal_handler_unblock (orientation_check_button, orientation_check_button_handler_id);

	g_signal_handler_block (show_linked_memories_names_entries_check_button, show_linked_memories_names_entries_check_button_handler_id);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_linked_memories_names_entries_check_button), current_cameras_set->show_linked_memories_names_entries);
	g_signal_handler_unblock (show_linked_memories_names_entries_check_button, show_linked_memories_names_entries_check_button_handler_id);

	g_signal_handler_block (show_linked_memories_names_labels_check_button, show_linked_memories_names_labels_check_button_handler_id);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_linked_memories_names_labels_check_button), current_cameras_set->show_linked_memories_names_labels);
	g_signal_handler_unblock (show_linked_memories_names_labels_check_button, show_linked_memories_names_labels_check_button_handler_id);

	g_signal_handler_block (thumbnail_size_scale, thumbnail_size_scale_handler_id);
	gtk_range_set_value (GTK_RANGE (thumbnail_size_scale), current_cameras_set->thumbnail_size);
	g_signal_handler_unblock (thumbnail_size_scale, thumbnail_size_scale_handler_id);

	g_signal_handler_block (memories_button_vertical_margins_scale, memories_button_vertical_margins_scale_handler_id);
	gtk_range_set_value (GTK_RANGE (memories_button_vertical_margins_scale), current_cameras_set->memories_button_vertical_margins);
	g_signal_handler_unblock (memories_button_vertical_margins_scale, memories_button_vertical_margins_scale_handler_id);

	if (current_cameras_set->memories_button_vertical_margins <= 1) gtk_label_set_text (GTK_LABEL (vertical_margins_label), pixel_txt);
	else gtk_label_set_text (GTK_LABEL (vertical_margins_label), pixels_txt);

	g_signal_handler_block (memories_button_horizontal_margins_scale, memories_button_horizontal_margins_scale_handler_id);
	gtk_range_set_value (GTK_RANGE (memories_button_horizontal_margins_scale), current_cameras_set->memories_button_horizontal_margins);
	g_signal_handler_unblock (memories_button_horizontal_margins_scale, memories_button_horizontal_margins_scale_handler_id);

	if (current_cameras_set->memories_button_horizontal_margins <= 1) gtk_label_set_text (GTK_LABEL (horizontal_margins_label), pixel_txt);
	else gtk_label_set_text (GTK_LABEL (horizontal_margins_label), pixels_txt);

	gtk_window_set_transient_for (GTK_WINDOW (interface_settings_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (interface_settings_window), TRUE);

	gtk_widget_show_all (interface_settings_window);
}

gboolean hide_interface_settings_window (void)
{
	gtk_window_set_transient_for (GTK_WINDOW (interface_settings_window), NULL);
	gtk_window_set_modal (GTK_WINDOW (interface_settings_window), FALSE);

	gtk_widget_hide (interface_settings_window);

	return GDK_EVENT_STOP;
}

gboolean interface_settings_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) {
		hide_interface_settings_window ();

		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

void create_interface_settings_window (void)
{
	GtkWidget *box1, *frame, *box2, *box3, *widget;

	interface_settings_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (interface_settings_window), interface_settings_txt + 1);
	gtk_window_set_decorated (GTK_WINDOW (interface_settings_window), FALSE);
	gtk_window_set_type_hint (GTK_WINDOW (interface_settings_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (interface_settings_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (interface_settings_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (interface_settings_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (interface_settings_window), "delete-event", G_CALLBACK (hide_interface_settings_window), NULL);
	g_signal_connect (G_OBJECT (interface_settings_window), "key-press-event", G_CALLBACK (interface_settings_window_key_press), NULL);

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Mémoire");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
			box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Orientation :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					orientation_check_button = gtk_check_button_new ();
					orientation_check_button_handler_id = g_signal_connect (G_OBJECT (orientation_check_button), "toggled", G_CALLBACK (orientation_check_button_toggled), NULL);
					gtk_widget_set_margin_start (orientation_check_button, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), orientation_check_button, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Afficher la zone de saisie des noms de mémoires liées :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					show_linked_memories_names_entries_check_button = gtk_check_button_new ();
					show_linked_memories_names_entries_check_button_handler_id = g_signal_connect (G_OBJECT (show_linked_memories_names_entries_check_button), "toggled", G_CALLBACK (show_linked_memories_names_entries_check_button_toggled), NULL);
					gtk_widget_set_margin_start (show_linked_memories_names_entries_check_button, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), show_linked_memories_names_entries_check_button, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Afficher la zone de rappel des noms de mémoires liées :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);

					show_linked_memories_names_labels_check_button = gtk_check_button_new ();
					show_linked_memories_names_labels_check_button_handler_id = g_signal_connect (G_OBJECT (show_linked_memories_names_labels_check_button), "toggled", G_CALLBACK (show_linked_memories_names_labels_check_button_toggled), NULL);
					gtk_widget_set_margin_start (show_linked_memories_names_labels_check_button, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), show_linked_memories_names_labels_check_button, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

    			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					thumbnail_size_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.5, 1.0, 0.1);
					gtk_scale_set_value_pos (GTK_SCALE (thumbnail_size_scale), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (thumbnail_size_scale), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (thumbnail_size_scale), FALSE);
					thumbnail_size_scale_handler_id = g_signal_connect (G_OBJECT (thumbnail_size_scale), "value-changed", G_CALLBACK (thumbnail_size_value_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box3), thumbnail_size_scale, TRUE, TRUE, 0);

					widget = gtk_label_new ("%");
					gtk_widget_set_margin_start (widget, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Taille des marges gauche/droite des boutons de mémoires :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					memories_button_vertical_margins_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 20.0, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (memories_button_vertical_margins_scale), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (memories_button_vertical_margins_scale), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (memories_button_vertical_margins_scale), FALSE);
					memories_button_vertical_margins_scale_handler_id = g_signal_connect (G_OBJECT (memories_button_vertical_margins_scale), "value-changed", G_CALLBACK (memories_button_vertical_margins_value_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box3), memories_button_vertical_margins_scale, TRUE, TRUE, 0);

					vertical_margins_label = gtk_label_new (NULL);
					gtk_widget_set_margin_start (vertical_margins_label, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), vertical_margins_label, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					widget =  gtk_label_new ("Taille des marges haut/bas des boutons de mémoires :");
				gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

				box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box3, MARGIN_VALUE);
				gtk_widget_set_margin_start (box3, MARGIN_VALUE);
				gtk_widget_set_margin_end (box3, MARGIN_VALUE);
					memories_button_horizontal_margins_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 20.0, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (memories_button_horizontal_margins_scale), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (memories_button_horizontal_margins_scale), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (memories_button_horizontal_margins_scale), FALSE);
					memories_button_horizontal_margins_scale_handler_id = g_signal_connect (G_OBJECT (memories_button_horizontal_margins_scale), "value-changed", G_CALLBACK (memories_button_horizontal_margins_value_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box3), memories_button_horizontal_margins_scale, TRUE, TRUE, 0);

					horizontal_margins_label = gtk_label_new (NULL);
					gtk_widget_set_margin_start (horizontal_margins_label, MARGIN_VALUE);
				gtk_box_pack_end (GTK_BOX (box3), horizontal_margins_label, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), box2);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);


/*	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		frame = gtk_frame_new ("Mémoire");
		gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
		gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
			box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);*/


//		gtk_container_add (GTK_CONTAINER (frame), box2);
//	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (interface_settings_window), box1);

	gtk_window_set_resizable (GTK_WINDOW (interface_settings_window), FALSE);
}

