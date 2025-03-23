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

#ifndef __CONTROL_WINDOW_H
#define __CONTROL_WINDOW_H


#include <gtk/gtk.h>


typedef struct {
	GtkWidget *window;
	gboolean is_on_screen;
	GtkWidget *tally[4];
	GtkWidget *name_drawing_area;

	GtkWidget *auto_focus_toggle_button;
	gulong auto_focus_handler_id;
	GtkWidget *focus_box;
	GdkWindow *gdk_window;
	gint x_root;
	gint y_root;
	int width;
	int height;

	GtkWidget *otaf_button;
	GtkWidget *focus_level_bar_drawing_area;

	GtkWidget *zoom_level_bar_drawing_area;
	GtkWidget *zoom_tele_button;
	GtkWidget *zoom_wide_button;

	gdouble x;
	gdouble y;
	int pan_speed;
	int tilt_speed;

	guint focus_timeout_id;
	guint pan_tilt_timeout_id;
	guint zoom_timeout_id;

	gboolean key_pressed;
} control_window_t;


void create_control_window (control_window_t *control_window, gpointer ptz);


extern char focus_near_speed_cmd[5];
extern char focus_far_speed_cmd[5];

extern char zoom_wide_speed_cmd[5];
extern char zoom_tele_speed_cmd[5];


#endif

