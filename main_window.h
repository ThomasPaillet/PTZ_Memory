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

#ifndef __MAIN_WINDOW_H
#define __MAIN_WINDOW_H


#include <gtk/gtk.h>


#define MARGIN_VALUE 5


extern const char warning_txt[];

extern GtkWidget *main_window;
extern GtkWidget *main_event_box;
extern GtkWidget *main_window_notebook;

extern GtkWidget *interface_button;

extern GtkWidget *store_toggle_button;
extern GtkWidget *delete_toggle_button;
extern GtkWidget *link_toggle_button;

extern GtkWidget *switch_cameras_on_button;
extern GtkWidget *switch_cameras_off_button;

extern GdkSeat *seat;
extern GList *pointing_devices;
extern GdkDevice *trackball;


gboolean digit_key_press (GtkEntry *entry, GdkEventKey *event);

gboolean show_exit_confirmation_window (void);


#endif

