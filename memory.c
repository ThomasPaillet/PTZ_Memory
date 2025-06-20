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

#include "memory.h"

#include "cameras_set.h"
#include "controller.h"
#include "interface.h"
#include "main_window.h"
#include "protocol.h"
#include "settings.h"
#include "sw_p_08.h"


memory_t *current_memory;

GtkWidget *memory_name_window;
GtkEntryBuffer *memory_name_entry_buffer;


gboolean free_memory_thread (memory_thread_t *memory_thread)
{
	g_thread_join (memory_thread->thread);
	g_free (memory_thread);

	return G_SOURCE_REMOVE;
}

gboolean update_button (memory_t *memory)
{
	if (memory->image == NULL) {
		if (interface_default.thumbnail_width == 320) memory->image = gtk_image_new_from_pixbuf (memory->full_pixbuf);
		else memory->image = gtk_image_new_from_pixbuf (memory->scaled_pixbuf);
		gtk_button_set_image (GTK_BUTTON (memory->button), memory->image);
	} else {
		if (interface_default.thumbnail_width == 320) gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->full_pixbuf);
		else gtk_image_set_from_pixbuf (GTK_IMAGE (memory->image), memory->scaled_pixbuf);
	}

	g_signal_handler_unblock (memory->button, memory->button_handler_id);

	return G_SOURCE_REMOVE;
}

gpointer save_memory (memory_thread_t *memory_thread)
{
	memory_t *memory = memory_thread->memory;
	ptz_t *ptz = memory->ptz_ptr;
	char buf[128];
	int len;

	if (ptz->model == AW_HE130) send_thumbnail_320_request_cmd (memory);
	else send_thumbnail_640_request_cmd (memory);

	if (!ptz->is_on) return NULL;

	if (interface_default.thumbnail_width != 320) {
		if (!memory->empty) g_object_unref (G_OBJECT (memory->scaled_pixbuf));
		memory->scaled_pixbuf = gdk_pixbuf_scale_simple (memory->full_pixbuf, interface_default.thumbnail_width, interface_default.thumbnail_height, GDK_INTERP_BILINEAR);
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

	if (ptz->previous_loaded_memory != NULL) {
		if (ptz->previous_loaded_memory != memory) {
			ptz->previous_loaded_memory->is_loaded = FALSE;
			gtk_widget_queue_draw (ptz->previous_loaded_memory->button);
		}

		ptz->previous_loaded_memory = NULL;
	}

	if ((ptz->ultimatte != NULL) && (ptz->ultimatte->connected)) {
		len = sprintf (buf, "FILE:\nSave: %s %s %d\n\n", current_cameras_set->name, ptz->name, memory->index);
		send (ptz->ultimatte->socket, buf, len, 0);
	}

	send_ptz_request_command_string (ptz, "#APC", buf);
	memory->pan_tilt_position_cmd[4] = buf[3];
	memory->pan_tilt_position_cmd[5] = buf[4];
	memory->pan_tilt_position_cmd[6] = buf[5];
	memory->pan_tilt_position_cmd[7] = buf[6];
	memory->pan_tilt_position_cmd[8] = buf[7];
	memory->pan_tilt_position_cmd[9] = buf[8];
	memory->pan_tilt_position_cmd[10] = buf[9];
	memory->pan_tilt_position_cmd[11] = buf[10];

	g_idle_add ((GSourceFunc)update_button, memory);
	g_idle_add ((GSourceFunc)free_memory_thread, memory_thread);

	backup_needed = TRUE;

	return NULL;
}

gpointer load_memory (memory_thread_t *memory_thread)
{
	memory_t *memory = memory_thread->memory;
	ptz_t *ptz = memory->ptz_ptr;
	ptz_thread_t *controller_thread;
	char buf[128];
	int len;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	if (controller_is_used && controller_ip_address_is_valid) {
		controller_thread = g_malloc (sizeof (ptz_thread_t));
		controller_thread->ptz = ptz;
		controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
	}

	if (memory != ptz->previous_loaded_memory) {
		memory->is_loaded = TRUE;
		gtk_widget_queue_draw (memory->button);
	
		if (ptz->previous_loaded_memory != NULL) {
			ptz->previous_loaded_memory->is_loaded = FALSE;
			gtk_widget_queue_draw (ptz->previous_loaded_memory->button);
		}
	
		ptz->previous_loaded_memory = memory;
	}

	g_mutex_lock (&ptz->lens_information_mutex);

	if ((ptz->ultimatte != NULL) && (ptz->ultimatte->connected)) {
		len = sprintf (buf, "FILE:\nLoad: %s %s %d\n\n", current_cameras_set->name, ptz->name, memory->index);
		send (ptz->ultimatte->socket, buf, len, 0);
	}

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
	memory_t *memory = memory_thread->memory;
	ptz_t *ptz = memory->ptz_ptr;

	send_ptz_control_command (ptz, memory->pan_tilt_position_cmd, TRUE);

	if (memory != ptz->previous_loaded_memory) {
		memory->is_loaded = TRUE;
		gtk_widget_queue_draw (memory->button);
	
		if (ptz->previous_loaded_memory != NULL) {
			ptz->previous_loaded_memory->is_loaded = FALSE;
			gtk_widget_queue_draw (ptz->previous_loaded_memory->button);
		}
	
		ptz->previous_loaded_memory = memory;
	}

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
	char buf[128];
	int len;

	if (event->button != GDK_BUTTON_PRIMARY) {
		if (!memory->empty) {
			current_memory = memory;

			if (memory->name_len == 0) gtk_entry_buffer_delete_text (memory_name_entry_buffer, 0, -1);
			else gtk_entry_buffer_set_text (memory_name_entry_buffer, memory->name, memory->name_len);

			gtk_widget_show_all (memory_name_window);

			return GDK_EVENT_PROPAGATE;
		} else return GDK_EVENT_STOP;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_toggle_button))) {
		if (!memory->empty) {
			ptz = memory->ptz_ptr;

			memory->empty = TRUE;
			ptz->number_of_memories--;

			memory->name[0] = '\0';
			memory->name_len = 0;

			if (ptz->previous_loaded_memory == memory) {
				ptz->previous_loaded_memory = NULL;
				memory->is_loaded = FALSE;
			}

			gtk_widget_destroy (memory->image);
			memory->image = NULL;

			g_object_unref (G_OBJECT (memory->full_pixbuf));
			if (interface_default.thumbnail_width != 320) g_object_unref (G_OBJECT (memory->scaled_pixbuf));

			if ((ptz->ultimatte != NULL) && (ptz->ultimatte->connected)) {
				len = sprintf (buf, "FILE:\nDelete: %s %s %d\n\n", current_cameras_set->name, ptz->name, memory->index);
				send (ptz->ultimatte->socket, buf, len, 0);
			}

			backup_needed = TRUE;
		}

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delete_toggle_button), FALSE);
	} else if (memory->empty || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (store_toggle_button))) {
		g_signal_handler_block (memory->button, memory->button_handler_id);

		memory_thread = g_malloc (sizeof (memory_thread_t));
		memory_thread->memory = memory;
		memory_thread->thread = g_thread_new (NULL, (GThreadFunc)save_memory, memory_thread);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (store_toggle_button), FALSE);
	} else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (link_toggle_button))) {
			for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
				ptz = current_cameras_set->cameras[i];

				if (ptz->ip_address_is_valid && ptz->is_on && !ptz->memories[memory->index].empty) {
					if (ptz->memories + memory->index == memory) {
						g_signal_handler_block (memory->button, memory->button_handler_id);

						memory_thread = g_malloc (sizeof (memory_thread_t));
						memory_thread->memory = memory;
						memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, memory_thread);
					} else {
//						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_ACTIVE, FALSE);
						gtk_widget_set_state_flags (ptz->memories[memory->index].button, GTK_STATE_FLAG_PRELIGHT, FALSE);
						g_signal_handler_block (ptz->memories[memory->index].button, ptz->memories[memory->index].button_handler_id);

						memory_thread = g_malloc (sizeof (ptz_thread_t));
						memory_thread->memory = ptz->memories + memory->index;
						memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_other_memory, memory_thread);
					}
				}
			}
		} else {
			g_signal_handler_block (memory->button, memory->button_handler_id);

			memory_thread = g_malloc (sizeof (memory_thread_t));
			memory_thread->memory = memory;
			memory_thread->thread = g_thread_new (NULL, (GThreadFunc)load_memory, memory_thread);
		}

		tell_memory_is_selected (memory->index);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean memory_name_and_outline_draw (GtkWidget *widget, cairo_t *cr, memory_t *memory)
{
	PangoLayout *pl;

	if (memory->is_loaded) {
		cairo_set_source_rgba (cr, 0.8, 0.545, 0.0, 1.0);
//Top
		cairo_rectangle (cr, 3.0, 2.0, interface_default.thumbnail_width + 4.0, 1.0);
		cairo_fill (cr);
		cairo_rectangle (cr, 2.0, 3.0, interface_default.thumbnail_width + 6.0, 2.0);
		cairo_fill (cr);
//Bottom
		cairo_rectangle (cr, 2.0, 5.0 + interface_default.thumbnail_height, interface_default.thumbnail_width + 6.0, 2.0);
		cairo_fill (cr);
		cairo_rectangle (cr, 3.0, 7.0 + interface_default.thumbnail_height, interface_default.thumbnail_width + 4.0, 1.0);
		cairo_fill (cr);
//Left
		cairo_rectangle (cr, 2.0, 5.0, 3.0, interface_default.thumbnail_height);
		cairo_fill (cr);
//Right
		cairo_rectangle (cr, 5.0 + interface_default.thumbnail_width, 5.0, 3.0, interface_default.thumbnail_height);
		cairo_fill (cr);
	}

	if (memory->name_len != 0) {
		cairo_set_source_rgba (cr, interface_default.memories_name_backdrop_color_red, interface_default.memories_name_backdrop_color_green, interface_default.memories_name_backdrop_color_blue, interface_default.memories_name_backdrop_color_alpha);
		cairo_rectangle (cr, 5.0, 5 + interface_default.thumbnail_height - (int)(24.0 * interface_default.thumbnail_size), interface_default.thumbnail_width, ((int)(24.0 * interface_default.thumbnail_size)));
		cairo_fill (cr);

		cairo_set_source_rgb (cr, interface_default.memories_name_color_red, interface_default.memories_name_color_green, interface_default.memories_name_color_blue);

		pl = pango_cairo_create_layout (cr);

		cairo_translate (cr, 5.0 + (interface_default.thumbnail_width / 2.0) - ((memory->name_len / 2.0) * 7.0 * (interface_default.thumbnail_size * 2.0)), interface_default.thumbnail_height - (7.0 + (interface_default.thumbnail_size - 0.5) * 28.0));

		pango_layout_set_text (pl, memory->name, -1);
		pango_layout_set_font_description (pl, interface_default.memory_name_font_description);

		pango_cairo_show_layout (cr, pl);

		g_object_unref(pl);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean memory_name_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Escape) {
		gtk_widget_hide (memory_name_window);

		return GDK_EVENT_STOP;
	} else if (event->keyval == GDK_KEY_Delete) {
		gtk_entry_buffer_delete_text (memory_name_entry_buffer, 0, -1);
	}

	return GDK_EVENT_PROPAGATE;
}

void memory_name_entry_activate (void)
{
	strcpy (current_memory->name, gtk_entry_buffer_get_text (memory_name_entry_buffer));
	current_memory->name_len = gtk_entry_buffer_get_length (memory_name_entry_buffer);

	gtk_widget_queue_draw (current_memory->button);

	gtk_widget_hide (memory_name_window);

	backup_needed = TRUE;
}

void create_memory_name_window (void)
{
	GtkWidget *memory_name_entry;

	memory_name_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (memory_name_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (memory_name_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (memory_name_window), TRUE);
	gtk_window_set_decorated (GTK_WINDOW (memory_name_window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (memory_name_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (memory_name_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (memory_name_window), GTK_WIN_POS_MOUSE);
	g_signal_connect (G_OBJECT (memory_name_window), "key-press-event", G_CALLBACK (memory_name_window_key_press), NULL);
	g_signal_connect (G_OBJECT (memory_name_window), "focus-out-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
	g_signal_connect (G_OBJECT (memory_name_window), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
		memory_name_entry = gtk_entry_new ();
		gtk_entry_set_max_length (GTK_ENTRY (memory_name_entry), MEMORIES_NAME_LENGTH);
		gtk_entry_set_width_chars (GTK_ENTRY (memory_name_entry), MEMORIES_NAME_LENGTH + 4);
		gtk_entry_set_alignment (GTK_ENTRY (memory_name_entry), 0.5);
		g_signal_connect (G_OBJECT (memory_name_entry), "activate", G_CALLBACK (memory_name_entry_activate), NULL);
		memory_name_entry_buffer = gtk_entry_get_buffer (GTK_ENTRY (memory_name_entry));
	gtk_container_add (GTK_CONTAINER (memory_name_window), memory_name_entry);
}

