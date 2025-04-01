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

#include "trackball.h"

#include "main_window.h"
#include "settings.h"


GList *pointing_devices = NULL;
GdkDevice *trackball = NULL;

GtkWidget *pointing_devices_combo_box = NULL;
gulong pointing_devices_combo_box_handler_id = 0;
GtkWidget *trackball_buttons;

char *trackball_name = NULL;
size_t trackball_name_len = 0;

int pan_tilt_stop_sensibility = 5;

gint trackball_button_action[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

gboolean button_pressed[10] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };


void device_added_to_seat (GdkSeat *seat, GdkDevice *device)
{
	gboolean trackball_is_back;

	pointing_devices = g_list_prepend (pointing_devices, device);

	if ((trackball_name_len > 0) && (memcmp (gdk_device_get_name (device), trackball_name, trackball_name_len) == 0)) {
		trackball = device;
		trackball_is_back = TRUE;
	} else trackball_is_back = FALSE;

	if (settings_window != NULL) {
		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box), 1, gdk_device_get_name (device));

		if (trackball_is_back) {
			g_signal_handler_block (pointing_devices_combo_box, pointing_devices_combo_box_handler_id);
			gtk_combo_box_set_active (GTK_COMBO_BOX (pointing_devices_combo_box), 1);
			g_signal_handler_unblock (pointing_devices_combo_box, pointing_devices_combo_box_handler_id);

			gtk_widget_set_sensitive (trackball_buttons, TRUE);
		}
	}
}

void device_removed_from_seat (GdkSeat *seat, GdkDevice *device)
{
	GList *glist;
	int i;

	pointing_devices = g_list_remove (pointing_devices, device);

	if (device == trackball) trackball = NULL;

	if (settings_window != NULL) {
		gtk_widget_set_sensitive (trackball_buttons, FALSE);

		g_signal_handler_block (pointing_devices_combo_box, pointing_devices_combo_box_handler_id);
		gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box));

		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box), 0, "");

		for (glist = pointing_devices, i = 1; glist != NULL; glist = glist->next, i++) {
			gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box), i, gdk_device_get_name (glist->data));

			if ((trackball_name_len > 0) && (memcmp (gdk_device_get_name (glist->data), trackball_name, trackball_name_len) == 0)) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (pointing_devices_combo_box), i);

				gtk_widget_set_sensitive (trackball_buttons, TRUE);
			}
		}
		g_signal_handler_unblock (pointing_devices_combo_box, pointing_devices_combo_box_handler_id);
	}
}

gboolean trackball_settings_button_press (GtkWidget *window, GdkEventButton *event)
{
	if ((gdk_event_get_source_device ((GdkEvent *)event) == trackball) && (event->button > 0 ) && (event->button <= 10)) {
		button_pressed[event->button - 1] = TRUE;

		gtk_widget_queue_draw (trackball_buttons);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean trackball_settings_button_release (GtkWidget *window, GdkEventButton *event)
{
	if ((gdk_event_get_source_device ((GdkEvent *)event) == trackball) && (event->button > 0 ) && (event->button <= 10)) {
		button_pressed[event->button - 1] = FALSE;

		gtk_widget_queue_draw (trackball_buttons);
	}

	return GDK_EVENT_PROPAGATE;
}

void pointing_devices_changed (void)
{
	GList *glist;

	g_free (trackball_name);
	trackball_name = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box));
	trackball_name_len = strlen (trackball_name);

	if (trackball_name_len > 0) {
		for (glist = pointing_devices; glist != NULL; glist = glist->next) {
			if (memcmp (gdk_device_get_name (glist->data), trackball_name, trackball_name_len) == 0) {
				trackball = glist->data;
				break;
			}
		}

		gtk_widget_set_sensitive (trackball_buttons, TRUE);
	} else {
		trackball = NULL;
		gtk_widget_set_sensitive (trackball_buttons, FALSE);
	}

	backup_needed = TRUE;
}

void pan_tilt_stop_sensibility_value_changed (GtkRange *range)
{
	pan_tilt_stop_sensibility = gtk_range_get_value (range);

	backup_needed = TRUE;
}

void trackball_button_action_changed (GtkComboBox *combo_box, gpointer index)
{
	trackball_button_action[GPOINTER_TO_INT (index)] = gtk_combo_box_get_active (combo_box);

	backup_needed = TRUE;
}

gboolean combo_box_outline_draw (GtkWidget *widget, cairo_t *cr, gpointer index)
{
	if (button_pressed[GPOINTER_TO_INT (index)]) cairo_set_source_rgba (cr, 0.8, 0.545, 0.0, 1.0);
	else cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);

	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

GtkWidget* create_trackball_settings_frame (void)
{
	GtkWidget *frame, *box1, *box2, *widget, *grid1, *grid2;
	GList *glist;
	int i;

	frame = gtk_frame_new ("Trackball/Joystick *expérimental*");
	gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (frame), MARGIN_VALUE);
		box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_widget_set_margin_top (box2, MARGIN_VALUE);
			gtk_widget_set_margin_start (box2, MARGIN_VALUE);
			gtk_widget_set_margin_end (box2, MARGIN_VALUE);
			gtk_widget_set_margin_bottom (box2, MARGIN_VALUE);
				pointing_devices_combo_box =  gtk_combo_box_text_new ();
				gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box), 0, "");
				gtk_combo_box_set_active (GTK_COMBO_BOX (pointing_devices_combo_box), 0);
				for (glist = pointing_devices, i = 1; glist != NULL; glist = glist->next, i++) {
					gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (pointing_devices_combo_box), i, gdk_device_get_name (glist->data));
					if ((trackball_name_len > 0) && (memcmp (gdk_device_get_name (glist->data), trackball_name, trackball_name_len) == 0)) gtk_combo_box_set_active (GTK_COMBO_BOX (pointing_devices_combo_box), i);
				}
				pointing_devices_combo_box_handler_id = g_signal_connect (G_OBJECT (pointing_devices_combo_box), "changed", G_CALLBACK (pointing_devices_changed), NULL);
			gtk_box_pack_start (GTK_BOX (box2), pointing_devices_combo_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

			trackball_buttons = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
				box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_widget_set_margin_top (box2, MARGIN_VALUE);
				gtk_widget_set_margin_start (box2, MARGIN_VALUE);
				gtk_widget_set_margin_end (box2, MARGIN_VALUE);
				gtk_widget_set_margin_bottom (box2, MARGIN_VALUE);
					widget =  gtk_label_new ("Sensibilité à l'arrêt");
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

					widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 20.0, 1.0);
					gtk_scale_set_value_pos (GTK_SCALE (widget), GTK_POS_RIGHT);
					gtk_scale_set_draw_value (GTK_SCALE (widget), TRUE);
					gtk_scale_set_has_origin (GTK_SCALE (widget), FALSE);
					gtk_range_set_value (GTK_RANGE (widget), pan_tilt_stop_sensibility);
					g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (pan_tilt_stop_sensibility_value_changed), NULL);
				gtk_box_pack_start (GTK_BOX (box2), widget, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (trackball_buttons), box2, FALSE, FALSE, 0);

				grid1 = gtk_grid_new ();
				for (i = 0; i < 5; i++) {
					grid2 = gtk_grid_new ();
						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 0, 3, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 1, 1, 1);

						widget =  gtk_combo_box_text_new ();
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 0, "");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 1, "Zoom +");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 2, "Zoom -");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 3, "OTAF");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 4, "Focus +");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 5, "Focus -");
						gtk_combo_box_set_active (GTK_COMBO_BOX (widget), trackball_button_action[i]);
						g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (trackball_button_action_changed), GINT_TO_POINTER (i));
					gtk_grid_attach (GTK_GRID (grid2), widget, 1, 1, 1, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i));
					gtk_grid_attach (GTK_GRID (grid2), widget, 2, 1, 1, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 2, 3, 1);
				gtk_grid_attach (GTK_GRID (grid1), grid2, i, 0, 1, 1);
				}

				for (i = 0; i < 5; i++) {
					grid2 = gtk_grid_new ();
						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i + 5));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 0, 3, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i + 5));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 1, 1, 1);

						widget =  gtk_combo_box_text_new ();
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 0, "");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 1, "Zoom +");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 2, "Zoom -");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 3, "OTAF");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 4, "Focus +");
						gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), 5, "Focus -");
						gtk_combo_box_set_active (GTK_COMBO_BOX (widget), trackball_button_action[i + 5]);
						g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (trackball_button_action_changed), GINT_TO_POINTER (i + 5));
					gtk_grid_attach (GTK_GRID (grid2), widget, 1, 1, 1, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i + 5));
					gtk_grid_attach (GTK_GRID (grid2), widget, 2, 1, 1, 1);

						widget = gtk_drawing_area_new ();
						gtk_widget_set_size_request (widget, MARGIN_VALUE, MARGIN_VALUE);
						g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (combo_box_outline_draw), GINT_TO_POINTER (i + 5));
					gtk_grid_attach (GTK_GRID (grid2), widget, 0, 2, 3, 1);
				gtk_grid_attach (GTK_GRID (grid1), grid2, i, 1, 1, 1);
				}
			gtk_box_pack_start (GTK_BOX (trackball_buttons), grid1, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box1), trackball_buttons, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), box1);

	if ((trackball_name_len < 1) || (gtk_combo_box_get_active (GTK_COMBO_BOX (pointing_devices_combo_box))) == 0) gtk_widget_set_sensitive (trackball_buttons, FALSE);

	return frame;
}

