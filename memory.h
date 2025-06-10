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

#ifndef __MEMORY_H
#define __MEMORY_H


#include <gtk/gtk.h>


#define MEMORIES_NAME_LENGTH 18


typedef struct {
	gpointer ptz_ptr;
	int index;

	gboolean empty;
	gboolean is_loaded;

	GtkWidget *button;
	gulong button_handler_id;
	GtkWidget *image;
	GdkPixbuf *full_pixbuf;
	GdkPixbuf *scaled_pixbuf;

	char pan_tilt_position_cmd[13];

	int zoom_position;
	char zoom_position_hexa[3];

	int focus_position;
	char focus_position_hexa[3];

	char name[MEMORIES_NAME_LENGTH * 4 + 1];
	int name_len;
} memory_t;

typedef struct memory_thread_s {
	memory_t *memory;
	GThread *thread;
} memory_thread_t;


gpointer load_memory (memory_thread_t *memory_thread);

gboolean memory_button_button_press_event (GtkButton *button, GdkEventButton *event, memory_t *memory);

gboolean memory_name_and_outline_draw (GtkWidget *widget, cairo_t *cr, memory_t *memory);

void create_memory_name_window (void);


#endif

