/*
 * copyright (c) 2020 2021 Thomas Paillet <thomas.paillet@net-c.fr>

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


void init_ptz (ptz_t *ptz)
{
	int i;

	ptz->active = TRUE;
	ptz->ip_address[0] = '\0';
	ptz->ip_address_is_valid = FALSE;

	init_ptz_cmd (ptz);

	ptz->model = AW_UE150;

	for (i = 0; i < MAX_MEMORIES; i++) {
		ptz->memories[i].ptz = ptz;
		ptz->memories[i].index = i;
		ptz->memories[i].empty = TRUE;
		ptz->memories[i].image = NULL;
		ptz->memories[i].pan_tilt_position_cmd[0] = '#';
		ptz->memories[i].pan_tilt_position_cmd[1] = 'A';
		ptz->memories[i].pan_tilt_position_cmd[2] = 'P';
		ptz->memories[i].pan_tilt_position_cmd[3] = 'C';
		ptz->memories[i].pan_tilt_position_cmd[12] = '\0';
	}
	ptz->number_of_memories = 0;

	ptz->zoom_position = 0xAAA;

	ptz->zoom_position_cmd[0] = '#';
	ptz->zoom_position_cmd[1] = 'A';
	ptz->zoom_position_cmd[2] = 'X';
	ptz->zoom_position_cmd[3] = 'Z';
	ptz->zoom_position_cmd[4] = 'A';
	ptz->zoom_position_cmd[5] = 'A';
	ptz->zoom_position_cmd[6] = 'A';
	ptz->zoom_position_cmd[7] = '\0';

	ptz->focus_position = 0xAAA;

	ptz->focus_position_cmd[0] = '#';
	ptz->focus_position_cmd[1] = 'A';
	ptz->focus_position_cmd[2] = 'X';
	ptz->focus_position_cmd[3] = 'F';
	ptz->focus_position_cmd[4] = 'A';
	ptz->focus_position_cmd[5] = 'A';
	ptz->focus_position_cmd[6] = 'A';
	ptz->focus_position_cmd[7] = '\0';

	ptz->control_window.is_on_screen = FALSE;

	ptz->control_window.focus_timeout_id = 0;
	ptz->control_window.pan_tilt_timeout_id = 0;
	ptz->control_window.zoom_timeout_id = 0;

	ptz->control_window.key_pressed = FALSE;

	ptz->name_grid = NULL;

	ptz->enter_notify_name_drawing_area = FALSE;

	ptz->tally_data = 0x00;
	ptz->tally_1_is_on = FALSE;

	ptz->error_code = 0x00;
}

gboolean ptz_is_on (ptz_t *ptz)
{
	gtk_widget_set_sensitive (ptz->name_grid, TRUE);
	gtk_widget_set_sensitive (ptz->memories_grid, TRUE);

	return G_SOURCE_REMOVE;
}

gboolean ptz_is_off (ptz_t *ptz)
{
	if (ptz->control_window.is_on_screen) {
		ptz->control_window.is_on_screen = FALSE;
		gtk_widget_hide (ptz->control_window.window);
	}

	if ((ptz->error_code != 0x30) && (ptz->error_code != 0x00)) {
		ptz->error_code = 0x00;
		gtk_widget_queue_draw (ptz->error_drawing_area);
		gtk_widget_set_tooltip_text (ptz->error_drawing_area, NULL);
	}

	gtk_widget_set_sensitive (ptz->name_grid, FALSE);
	gtk_widget_set_sensitive (ptz->memories_grid, FALSE);

	return G_SOURCE_REMOVE;
}

gboolean free_ptz_thread (ptz_thread_t *ptz_thread)
{
	g_thread_join (ptz_thread->thread);
	g_free (ptz_thread);

	return G_SOURCE_REMOVE;
}

gpointer start_ptz (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = (ptz_t*)ptz_thread->pointer;
	int response = 0;
	char buffer[16];

	send_update_start_cmd (ptz);

	if (ptz->error_code != 0x30) send_ptz_request_command (ptz, "#O", &response);

	if (response == 1) {
		send_cam_request_command_string (ptz, "QID", buffer);
		if (memcmp (buffer, "AW-HE130", 8) == 0) ptz->model = AW_HE130;

		send_cam_request_command (ptz, "QAF", &response);
		if (response == 1) ptz->auto_focus = TRUE;
		else ptz->auto_focus = FALSE;

		send_ptz_request_command_string (ptz, "#LPI", buffer);
		ptz->zoom_position_cmd[4] = buffer[3];
		ptz->zoom_position_cmd[5] = buffer[4];
		ptz->zoom_position_cmd[6] = buffer[5];

		ptz->focus_position_cmd[4] = buffer[6];
		ptz->focus_position_cmd[5] = buffer[7];
		ptz->focus_position_cmd[6] = buffer[8];

		if (buffer[3] <= '9') ptz->zoom_position = (buffer[3] - '0') * 256;
		else ptz->zoom_position = (buffer[3] - '7') * 256;
		if (buffer[4] <= '9') ptz->zoom_position += (buffer[4] - '0') * 16;
		else ptz->zoom_position += (buffer[4] - '7') * 16;
		if (buffer[5] <= '9') ptz->zoom_position += buffer[5] - '0';
		else ptz->zoom_position += buffer[5] - '7';

		if (buffer[6] <= '9') ptz->focus_position = (buffer[6] - '0') * 256;
		else ptz->focus_position = (buffer[6] - '7') * 256;
		if (buffer[7] <= '9') ptz->focus_position += (buffer[7] - '0') * 16;
		else ptz->focus_position += (buffer[7] - '7') * 16;
		if (buffer[8] <= '9') ptz->focus_position += buffer[8] - '0';
		else ptz->focus_position += buffer[8] - '7';

		send_ptz_control_command (ptz, "#LPC1", TRUE);

		g_idle_add ((GSourceFunc)ptz_is_on, ptz);
	} else g_idle_add ((GSourceFunc)ptz_is_off, ptz);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gpointer switch_ptz_on (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = ptz_thread->pointer;

	if (ptz->error_code == 0x30) send_update_start_cmd (ptz);

	if (ptz->error_code != 0x30) send_ptz_control_command (ptz, "#O1", TRUE);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gpointer switch_ptz_off (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = ptz_thread->pointer;

	if (ptz->error_code == 0x30) send_update_start_cmd (ptz);

	if (ptz->error_code != 0x30) send_ptz_control_command (ptz, "#O0", TRUE);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gboolean update_button (memory_t *memory)
{
	if (memory->image == NULL) {
		if (thumbnail_width == 320) memory->image = gtk_image_new_from_pixbuf (memory->pixbuf);
		else memory->image = gtk_image_new_from_pixbuf (memory->scaled_pixbuf);
		gtk_button_set_image (GTK_BUTTON (memory->button), memory->image);
	} else {
		if (thumbnail_width == 320) gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->pixbuf);
		else gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->scaled_pixbuf);
	}

	g_signal_handler_unblock (memory->button, memory->button_handler_id);

	return G_SOURCE_REMOVE;
}

gpointer save_memory (ptz_thread_t *ptz_thread)
{
	memory_t *memory = ptz_thread->pointer;
	ptz_t *ptz = memory->ptz;
	char response[16];

	if (ptz->model == AW_HE130) send_thumbnail_320_request_cmd (memory);
	else send_thumbnail_640_request_cmd (memory);

	if (thumbnail_width != 320) {
		if (!memory->empty) g_object_unref (G_OBJECT (memory->scaled_pixbuf));
		memory->scaled_pixbuf = gdk_pixbuf_scale_simple (memory->pixbuf, thumbnail_width, thumbnail_height, GDK_INTERP_BILINEAR);
	}

	memory->zoom_position_hexa[0] = ptz->zoom_position_cmd[4];
	memory->zoom_position_hexa[1] = ptz->zoom_position_cmd[5];
	memory->zoom_position_hexa[2] = ptz->zoom_position_cmd[6];
	memory->zoom_position = ptz->zoom_position;

	memory->focus_position_hexa[0] = ptz->focus_position_cmd[4];
	memory->focus_position_hexa[1] = ptz->focus_position_cmd[5];
	memory->focus_position_hexa[2] = ptz->focus_position_cmd[6];
	memory->focus_position = ptz->focus_position;

	if (memory->empty) {
		memory->empty = FALSE;
		ptz->number_of_memories++;
	}

	send_ptz_request_command_string (ptz, "#APC", response);
	memory->pan_tilt_position_cmd[4] = response[3];
	memory->pan_tilt_position_cmd[5] = response[4];
	memory->pan_tilt_position_cmd[6] = response[5];
	memory->pan_tilt_position_cmd[7] = response[6];
	memory->pan_tilt_position_cmd[8] = response[7];
	memory->pan_tilt_position_cmd[9] = response[8];
	memory->pan_tilt_position_cmd[10] = response[9];
	memory->pan_tilt_position_cmd[11] = response[10];

	g_idle_add ((GSourceFunc)update_button, memory);
	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	backup_needed = TRUE;

	return NULL;
}

gpointer load_memory (ptz_thread_t *ptz_thread)
{
	memory_t *memory = ptz_thread->pointer;
	ptz_t *ptz = memory->ptz;
	ptz_thread_t *controller_thread;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	if (controller_is_used && controller_ip_address_is_valid) {
		controller_thread = g_malloc (sizeof (ptz_thread_t));
		controller_thread->pointer = ptz;
		controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
	}

	if (ptz->zoom_position != memory->zoom_position) {
		ptz->zoom_position_cmd[4] = memory->zoom_position_hexa[0];
		ptz->zoom_position_cmd[5] = memory->zoom_position_hexa[1];
		ptz->zoom_position_cmd[6] = memory->zoom_position_hexa[2];
		send_ptz_control_command (ptz, ptz->zoom_position_cmd, TRUE);
		ptz->zoom_position = memory->zoom_position;
	}

	if (!ptz->auto_focus && (ptz->focus_position != memory->focus_position)) {
		ptz->focus_position_cmd[4] = memory->focus_position_hexa[0];
		ptz->focus_position_cmd[5] = memory->focus_position_hexa[1];
		ptz->focus_position_cmd[6] = memory->focus_position_hexa[2];
		send_ptz_control_command (ptz, ptz->focus_position_cmd, TRUE);
		ptz->focus_position = memory->focus_position;
	}

	g_signal_handler_unblock (memory->button, memory->button_handler_id);
	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gboolean release_memory_button (memory_t *memory)
{
//	gtk_widget_unset_state_flags (memory->button, GTK_STATE_FLAG_ACTIVE);
	gtk_widget_unset_state_flags (memory->button, GTK_STATE_FLAG_PRELIGHT);
	g_signal_handler_unblock (memory->button, memory->button_handler_id);

	return G_SOURCE_REMOVE;
}

gpointer load_other_memory (ptz_thread_t *ptz_thread)
{
	memory_t *memory = ptz_thread->pointer;
	ptz_t *ptz = memory->ptz;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	if (ptz->zoom_position != memory->zoom_position) {
		ptz->zoom_position_cmd[4] = memory->zoom_position_hexa[0];
		ptz->zoom_position_cmd[5] = memory->zoom_position_hexa[1];
		ptz->zoom_position_cmd[6] = memory->zoom_position_hexa[2];
		send_ptz_control_command (ptz, ptz->zoom_position_cmd, TRUE);
		ptz->zoom_position = memory->zoom_position;
	}

	if (!ptz->auto_focus && (ptz->focus_position != memory->focus_position)) {
		ptz->focus_position_cmd[4] = memory->focus_position_hexa[0];
		ptz->focus_position_cmd[5] = memory->focus_position_hexa[1];
		ptz->focus_position_cmd[6] = memory->focus_position_hexa[2];
		send_ptz_control_command (ptz, ptz->focus_position_cmd, TRUE);
		ptz->focus_position = memory->focus_position;
	}

	g_idle_add ((GSourceFunc)release_memory_button, memory);
	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gboolean memory_button_button_press_event (GtkButton *button, GdkEventButton *event, memory_t *memory)
{
	ptz_thread_t *ptz_thread;
	int i;
	ptz_t *ptz;

	if (event->button != GDK_BUTTON_PRIMARY) return GDK_EVENT_PROPAGATE;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_toggle_button))) {
		if (!memory->empty) {
			memory->empty = TRUE;
			((ptz_t*)memory->ptz)->number_of_memories--;
			gtk_button_set_image (GTK_BUTTON (memory->button), NULL);
			g_object_unref (G_OBJECT (memory->pixbuf));
			if (thumbnail_width != 320) g_object_unref (G_OBJECT (memory->scaled_pixbuf));
			memory->image = NULL;
			backup_needed = TRUE;
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delete_toggle_button), FALSE);
	} else if (memory->empty || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (store_toggle_button))) {
		g_signal_handler_block (memory->button, memory->button_handler_id);

		ptz_thread = g_malloc (sizeof (ptz_thread_t));
		ptz_thread->pointer = memory;
		ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)save_memory, ptz_thread);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (store_toggle_button), FALSE);
	} else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (link_toggle_button))) {
			for (i = 0; i < current_camera_set->number_of_cameras; i++) {
				ptz = current_camera_set->ptz_ptr_array[i];

				if (ptz->ip_address_is_valid && (ptz->error_code != 0x30) && !ptz->memories[memory->index].empty) {
					if (ptz->memories + memory->index == memory) {
						g_signal_handler_block (memory->button, memory->button_handler_id);

						ptz_thread = g_malloc (sizeof (ptz_thread_t));
						ptz_thread->pointer = memory;
						ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, ptz_thread);
					} else {
//						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_ACTIVE, FALSE);
						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_PRELIGHT, FALSE);
						g_signal_handler_block (ptz->memories[memory->index].button, ptz->memories[memory->index].button_handler_id);

						ptz_thread = g_malloc (sizeof (ptz_thread_t));
						ptz_thread->pointer = ptz->memories + memory->index;
						ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)load_other_memory, ptz_thread);
					}
				}
			}
		} else {
			g_signal_handler_block (memory->button, memory->button_handler_id);

			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->pointer = memory;
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, ptz_thread);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean name_drawing_area_button_press_event (GtkButton *widget, GdkEventButton *event, ptz_t *ptz)
{
	ptz_thread_t *controller_thread;

	if (event->button == GDK_BUTTON_PRIMARY) {
		gtk_window_set_position (GTK_WINDOW (ptz->control_window.window), GTK_WIN_POS_MOUSE);
		show_control_window (ptz);

		if (trackball != NULL) gdk_device_get_position_double (mouse, NULL, &ptz->control_window.x, &ptz->control_window.y);

		ask_to_connect_ptz_to_ctrl_opv (ptz);

		if (controller_is_used && controller_ip_address_is_valid) {
			controller_thread = g_malloc (sizeof (ptz_thread_t));
			controller_thread->pointer = ptz;
			controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
		}
	}

	return GDK_EVENT_STOP;
}

gboolean name_drawing_area_enter_notify_event (GtkWidget *widget, GdkEvent *event, ptz_t *ptz)
{
	ptz->enter_notify_name_drawing_area = TRUE;
	gtk_widget_queue_draw (widget);
	gtk_widget_queue_draw (ptz->error_drawing_area);

	return GDK_EVENT_PROPAGATE;
}

gboolean name_drawing_area_leave_notify_event (GtkWidget *widget, GdkEvent *event, ptz_t *ptz)
{
	ptz->enter_notify_name_drawing_area = FALSE;
	gtk_widget_queue_draw (widget);
	gtk_widget_queue_draw (ptz->error_drawing_area);

	return GDK_EVENT_PROPAGATE;
}

gboolean ghost_name_drawing_area_button_press_event (GtkButton *widget, GdkEventButton *event, ptz_t *ptz)
{
	if (event->button == GDK_BUTTON_PRIMARY) ask_to_connect_ptz_to_ctrl_opv (ptz);

	return GDK_EVENT_STOP;
}

void create_ptz_widgets (ptz_t *ptz)
{
	int i;

	create_control_window (ptz);

	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 0, 0, 3, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 1, 1, 1);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, thumbnail_height, thumbnail_height + 10);
		gtk_widget_set_events (ptz->name_drawing_area, gtk_widget_get_events (ptz->name_drawing_area) | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (name_drawing_area_button_press_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "enter-notify-event", G_CALLBACK (name_drawing_area_enter_notify_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "leave-notify-event", G_CALLBACK (name_drawing_area_leave_notify_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->error_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->error_drawing_area, 8, thumbnail_height + 10);
		g_signal_connect (G_OBJECT (ptz->error_drawing_area), "draw", G_CALLBACK (error_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->error_drawing_area, 2, 1, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 0, 2, 3, 1);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, MAX_MEMORIES, 1);

		for (i = 0; i < MAX_MEMORIES; i++) {
			ptz->memories[i].button = gtk_button_new ();
			gtk_widget_set_size_request (ptz->memories[i].button, thumbnail_width + 10, thumbnail_height + 10);
			ptz->memories[i].button_handler_id = g_signal_connect (G_OBJECT (ptz->memories[i].button), "button-press-event", G_CALLBACK (memory_button_button_press_event), ptz->memories + i);
			gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->memories[i].button, i, 1, 1, 1);
		}

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 0, 2, MAX_MEMORIES, 1);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], MAX_MEMORIES, 0, 1, 3);
}

gboolean ghost_body_draw (GtkWidget *widget, cairo_t *cr)
{
	cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);
	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

void create_ghost_ptz_widgets (ptz_t *ptz)
{
	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 0, 0, 2, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 1, 1, 1);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, thumbnail_height + 8, thumbnail_height / 2);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (ghost_name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (ghost_name_drawing_area_button_press_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 0, 2, 2, 1);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, MAX_MEMORIES, 1);

		ptz->ghost_body = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->ghost_body, (thumbnail_width + 10) * MAX_MEMORIES, thumbnail_height / 2);
		g_signal_connect (G_OBJECT (ptz->ghost_body), "draw", G_CALLBACK (ghost_body_draw), NULL);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->ghost_body, 0, 1, MAX_MEMORIES, 1);

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 0, 2, MAX_MEMORIES, 1);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (ghost_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], MAX_MEMORIES, 0, 1, 3);
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

void add_cameras_set_to_main_window_notebook (cameras_set_t *cameras_set)
{
	int i;
	GtkWidget *box1, *box2, *scrolled_window, *widget, *memories_scrolled_window;

	cameras_set->page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			cameras_set->entry_widgets_padding = gtk_drawing_area_new ();
			gtk_widget_set_size_request (cameras_set->entry_widgets_padding, thumbnail_height + 13, 34);
		gtk_box_pack_start (GTK_BOX (box1), cameras_set->entry_widgets_padding, FALSE, FALSE, 0);

			scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 30);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
				box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				for (i = 0; i < MAX_MEMORIES; i++) {
					cameras_set->entry_widgets[i] = gtk_entry_new ();
					gtk_widget_set_size_request (cameras_set->entry_widgets[i], thumbnail_width + 6, 34);
					gtk_widget_set_margin_start (cameras_set->entry_widgets[i], 2);
					gtk_widget_set_margin_end (cameras_set->entry_widgets[i], 2);
					gtk_entry_set_max_length (GTK_ENTRY (cameras_set->entry_widgets[i]), MEMORIES_NAME_LENGTH);
					gtk_entry_set_width_chars (GTK_ENTRY (cameras_set->entry_widgets[i]), MEMORIES_NAME_LENGTH);
					gtk_entry_set_alignment (GTK_ENTRY (cameras_set->entry_widgets[i]), 0.5);
					gtk_box_pack_start (GTK_BOX (box2), cameras_set->entry_widgets[i], FALSE, FALSE, 0);
				}
					widget = gtk_drawing_area_new ();
					gtk_widget_set_size_request (widget, 4, 34);
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
			gtk_container_add (GTK_CONTAINER (scrolled_window), box2);
			cameras_set->entry_scrolled_window_hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
			g_signal_connect (G_OBJECT (cameras_set->entry_scrolled_window_hadjustment), "value-changed", G_CALLBACK (entry_scrolled_window_hadjustment_value_changed), cameras_set);
		gtk_box_pack_start (GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), box1, FALSE, FALSE, 0);

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

		box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			cameras_set->memories_labels_padding = gtk_drawing_area_new ();
			gtk_widget_set_size_request (cameras_set->memories_labels_padding, thumbnail_height + 13, 10);
		gtk_box_pack_start (GTK_BOX (box1), cameras_set->memories_labels_padding, FALSE, FALSE, 0);

			scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolled_window), GTK_CORNER_BOTTOM_LEFT);
			gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 50);
				box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				for (i = 0; i < MAX_MEMORIES; i++) {
					cameras_set->memories_labels[i] = gtk_label_new (NULL);
					gtk_widget_set_size_request (cameras_set->memories_labels[i], thumbnail_width + 6, 10);
					gtk_widget_set_margin_start (cameras_set->memories_labels[i], 2);
					gtk_widget_set_margin_end (cameras_set->memories_labels[i], 2);
					gtk_label_set_xalign (GTK_LABEL (cameras_set->memories_labels[i]), 0.5);
					gtk_box_pack_start (GTK_BOX (box2), cameras_set->memories_labels[i], FALSE, FALSE, 0);
					g_signal_connect (G_OBJECT (cameras_set->entry_widgets[i]), "activate", G_CALLBACK (entry_activate), cameras_set->memories_labels[i]);
				}
					widget = gtk_drawing_area_new ();
					gtk_widget_set_size_request (widget, 4, 10);
				gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
			gtk_container_add (GTK_CONTAINER (scrolled_window), box2);
			cameras_set->label_scrolled_window_hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
			g_signal_connect (G_OBJECT (cameras_set->label_scrolled_window_hadjustment), "value-changed", G_CALLBACK (label_scrolled_window_hadjustment_value_changed), cameras_set);
		gtk_box_pack_start (GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cameras_set->page), box1, FALSE, FALSE, 0);

	gtk_widget_show_all (cameras_set->page);

	widget = gtk_label_new (cameras_set->name);
	cameras_set->page_num = gtk_notebook_append_page (GTK_NOTEBOOK (main_window_notebook), cameras_set->page, widget);
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (main_window_notebook), cameras_set->page, TRUE);
}

