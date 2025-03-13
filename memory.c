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

#include "ptz.h"


gboolean free_memory_thread (memory_thread_t *memory_thread)
{
	g_thread_join (memory_thread->thread);
	g_free (memory_thread);

	return G_SOURCE_REMOVE;
}

gboolean update_button (memory_t *memory)
{
	if (memory->image == NULL) {
		if (thumbnail_width == 320) memory->image = gtk_image_new_from_pixbuf (memory->full_pixbuf);
		else memory->image = gtk_image_new_from_pixbuf (memory->scaled_pixbuf);
		gtk_button_set_image (GTK_BUTTON (memory->button), memory->image);
	} else {
		if (thumbnail_width == 320) gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->full_pixbuf);
		else gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->scaled_pixbuf);
	}

	g_signal_handler_unblock (memory->button, memory->button_handler_id);

	return G_SOURCE_REMOVE;
}

gpointer save_memory (memory_thread_t *memory_thread)
{
	memory_t *memory = memory_thread->memory_ptr;
	ptz_t *ptz = memory->ptz_ptr;
	char response[16];

	if (ptz->model == AW_HE130) send_thumbnail_320_request_cmd (memory);
	else send_thumbnail_640_request_cmd (memory);

	if (thumbnail_width != 320) {
		if (!memory->empty) g_object_unref (G_OBJECT (memory->scaled_pixbuf));
		memory->scaled_pixbuf = gdk_pixbuf_scale_simple (memory->full_pixbuf, thumbnail_width, thumbnail_height, GDK_INTERP_BILINEAR);
	}

	g_mutex_lock (&ptz->lens_information_mutex);

	memory->zoom_position = ptz->zoom_position;
	memory->zoom_position_hexa[0] = ptz->zoom_position_cmd[4];
	memory->zoom_position_hexa[1] = ptz->zoom_position_cmd[5];
	memory->zoom_position_hexa[2] = ptz->zoom_position_cmd[6];

	memory->focus_position = ptz->focus_position;
	memory->focus_position_hexa[0] = ptz->focus_position_cmd[4];
	memory->focus_position_hexa[1] = ptz->focus_position_cmd[5];
	memory->focus_position_hexa[2] = ptz->focus_position_cmd[6];

	g_mutex_unlock (&ptz->lens_information_mutex);

	if (memory->empty) {
		memory->empty = FALSE;
		ptz->number_of_memories++;
	}

	memory->is_loaded = FALSE;
	ptz->previous_loaded_memory = NULL;

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
	g_idle_add ((GSourceFunc)free_memory_thread, memory_thread);

	backup_needed = TRUE;

	return NULL;
}

gpointer load_memory (memory_thread_t *memory_thread)
{
	memory_t *memory = memory_thread->memory_ptr;
	ptz_t *ptz = memory->ptz_ptr;
	ptz_thread_t *controller_thread;
	int i;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	if (controller_is_used && controller_ip_address_is_valid) {
		controller_thread = g_malloc (sizeof (ptz_thread_t));
		controller_thread->ptz_ptr = ptz;
		controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
	}

	memory->is_loaded = TRUE;
	gtk_widget_queue_draw (memory->image);

	if (ptz->previous_loaded_memory != NULL) {
		ptz->previous_loaded_memory->is_loaded = FALSE;
		gtk_widget_queue_draw (ptz->previous_loaded_memory->image);
	}

	ptz->previous_loaded_memory = memory;

	g_mutex_lock (&ptz->lens_information_mutex);

	if (ptz->zoom_position != memory->zoom_position) {
		ptz->zoom_position = memory->zoom_position;
		ptz->zoom_position_cmd[4] = memory->zoom_position_hexa[0];
		ptz->zoom_position_cmd[5] = memory->zoom_position_hexa[1];
		ptz->zoom_position_cmd[6] = memory->zoom_position_hexa[2];

		send_ptz_control_command (ptz, ptz->zoom_position_cmd, TRUE);
	}

	if (!ptz->auto_focus && (ptz->focus_position != memory->focus_position)) {
		ptz->focus_position = memory->focus_position;
		ptz->focus_position_cmd[4] = memory->focus_position_hexa[0];
		ptz->focus_position_cmd[5] = memory->focus_position_hexa[1];
		ptz->focus_position_cmd[6] = memory->focus_position_hexa[2];

		send_ptz_control_command (ptz, ptz->focus_position_cmd, TRUE);
	}

	g_mutex_unlock (&ptz->lens_information_mutex);

	g_signal_handler_unblock (memory->button, memory->button_handler_id);
	g_idle_add ((GSourceFunc)free_memory_thread, memory_thread);

	return NULL;
}

gboolean release_memory_button (memory_t *memory)
{
//	gtk_widget_unset_state_flags (memory->button, GTK_STATE_FLAG_ACTIVE);
	gtk_widget_unset_state_flags (memory->button, GTK_STATE_FLAG_PRELIGHT);
	g_signal_handler_unblock (memory->button, memory->button_handler_id);

	return G_SOURCE_REMOVE;
}

gpointer load_other_memory (memory_thread_t *memory_thread)
{
	memory_t *memory = memory_thread->memory_ptr;
	ptz_t *ptz = memory->ptz_ptr;
	int i;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	memory->is_loaded = TRUE;
	gtk_widget_queue_draw (memory->image);

	if (ptz->previous_loaded_memory != NULL) {
		ptz->previous_loaded_memory->is_loaded = FALSE;
		gtk_widget_queue_draw (ptz->previous_loaded_memory->image);
	}

	ptz->previous_loaded_memory = memory;

	g_mutex_lock (&ptz->lens_information_mutex);

	if (ptz->zoom_position != memory->zoom_position) {
		ptz->zoom_position = memory->zoom_position;
		ptz->zoom_position_cmd[4] = memory->zoom_position_hexa[0];
		ptz->zoom_position_cmd[5] = memory->zoom_position_hexa[1];
		ptz->zoom_position_cmd[6] = memory->zoom_position_hexa[2];

		send_ptz_control_command (ptz, ptz->zoom_position_cmd, TRUE);
	}

	if (!ptz->auto_focus && (ptz->focus_position != memory->focus_position)) {
		ptz->focus_position = memory->focus_position;
		ptz->focus_position_cmd[4] = memory->focus_position_hexa[0];
		ptz->focus_position_cmd[5] = memory->focus_position_hexa[1];
		ptz->focus_position_cmd[6] = memory->focus_position_hexa[2];

		send_ptz_control_command (ptz, ptz->focus_position_cmd, TRUE);
	}

	g_mutex_unlock (&ptz->lens_information_mutex);

	g_idle_add ((GSourceFunc)release_memory_button, memory);
	g_idle_add ((GSourceFunc)free_memory_thread, memory_thread);

	return NULL;
}

gboolean memory_button_button_press_event (GtkButton *button, GdkEventButton *event, memory_t *memory)
{
	memory_thread_t *memory_thread;
	int i;
	ptz_t *ptz;

	if (event->button != GDK_BUTTON_PRIMARY) {
		gtk_widget_show_all (memory->name_window);

		return GDK_EVENT_PROPAGATE;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_toggle_button))) {
		if (!memory->empty) {
			ptz = memory->ptz_ptr;

			memory->empty = TRUE;
			ptz->number_of_memories--;

			if (ptz->previous_loaded_memory == memory) {
				ptz->previous_loaded_memory = NULL;
				memory->is_loaded = FALSE;
			}

			gtk_widget_destroy (memory->image);
			memory->image = NULL;
			g_object_unref (G_OBJECT (memory->full_pixbuf));
			if (thumbnail_width != 320) g_object_unref (G_OBJECT (memory->scaled_pixbuf));

			memory->name[0] = '\0';

			backup_needed = TRUE;
		}

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delete_toggle_button), FALSE);
	} else if (memory->empty || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (store_toggle_button))) {
		g_signal_handler_block (memory->button, memory->button_handler_id);

		memory_thread = g_malloc (sizeof (memory_thread_t));
		memory_thread->memory_ptr = memory;
		memory_thread->thread = g_thread_new (NULL, (GThreadFunc)save_memory, memory_thread);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (store_toggle_button), FALSE);
	} else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (link_toggle_button))) {
			for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
				ptz = current_cameras_set->ptz_ptr_array[i];

				if (ptz->ip_address_is_valid && (ptz->error_code != 0x30) && !ptz->memories[memory->index].empty) {
					if (ptz->memories + memory->index == memory) {
						g_signal_handler_block (memory->button, memory->button_handler_id);

						memory_thread = g_malloc (sizeof (memory_thread_t));
						memory_thread->memory_ptr = memory;
						memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, memory_thread);
					} else {
//						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_ACTIVE, FALSE);
						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_PRELIGHT, FALSE);
						g_signal_handler_block (ptz->memories[memory->index].button, ptz->memories[memory->index].button_handler_id);

						memory_thread = g_malloc (sizeof (ptz_thread_t));
						memory_thread->memory_ptr = ptz->memories + memory->index;
						memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_other_memory, memory_thread);
					}
				}
			}
		} else {
			g_signal_handler_block (memory->button, memory->button_handler_id);

			memory_thread = g_malloc (sizeof (memory_thread_t));
			memory_thread->memory_ptr = memory;
			memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, memory_thread);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

extern char *font;

gboolean memory_name_draw (GtkWidget *widget, cairo_t *cr, char *name)
{
	PangoLayout *pl;
	PangoFontDescription *desc;

	if (name[0] != '\0') {
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.2);
		cairo_rectangle (cr, 5.0, thumbnail_height + 5.0 - (20.0 * thumbnail_size), thumbnail_width, 20.0 * thumbnail_size);
		cairo_fill (cr);

		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		pl = pango_cairo_create_layout (cr);

		cairo_translate (cr, 5.0 + 16.0 * (10 - (strlen (name) / 2)) * thumbnail_size, thumbnail_height - (19.0 * thumbnail_size) + (1.0 - thumbnail_size) * 4.0);

		sprintf (font + 13, "%d", (int)(20.0 * thumbnail_size));

		pango_layout_set_text (pl, name, -1);
		desc = pango_font_description_from_string (font);
		pango_layout_set_font_description (pl, desc);
		pango_font_description_free (desc);

		pango_cairo_show_layout (cr, pl);

		g_object_unref(pl);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean memory_outline_draw (GtkWidget *widget, cairo_t *cr, memory_t *memory)
{
	if (memory->is_loaded) {
		cairo_set_source_rgba (cr, 0.8, 0.545, 0.0, 1.0);
//Top
		cairo_rectangle (cr, 3.0, 2.0, thumbnail_width + 4.0, 1.0);
		cairo_fill (cr);
		cairo_rectangle (cr, 2.0, 3.0, thumbnail_width + 6.0, 2.0);
		cairo_fill (cr);
//Bottom
		cairo_rectangle (cr, 2.0, 5.0 + thumbnail_height, thumbnail_width + 6.0, 2.0);
		cairo_fill (cr);
		cairo_rectangle (cr, 3.0, 7.0 + thumbnail_height, thumbnail_width + 4.0, 1.0);
		cairo_fill (cr);
//Left
		cairo_rectangle (cr, 2.0, 5.0, 3.0, thumbnail_height);
		cairo_fill (cr);
//Right
		cairo_rectangle (cr, 5.0 + thumbnail_width, 5.0, 3.0, thumbnail_height);
		cairo_fill (cr);
	}

	return GDK_EVENT_PROPAGATE;
}

