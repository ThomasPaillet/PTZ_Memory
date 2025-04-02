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

#ifndef __TRACKBALL_H
#define __TRACKBALL_H


#include <gtk/gtk.h>


extern GList *pointing_devices;
extern GdkDevice *trackball;

extern char *trackball_name;
extern size_t trackball_name_len;

extern gdouble trackball_sensibility;
extern int pan_tilt_stop_sensibility;

extern gint trackball_button_action[10];


void device_added_to_seat (GdkSeat *seat, GdkDevice *device);

void device_removed_from_seat (GdkSeat *seat, GdkDevice *device);

gboolean trackball_settings_button_press (GtkWidget *window, GdkEventButton *event);

gboolean trackball_settings_button_release (GtkWidget *window, GdkEventButton *event);

GtkWidget* create_trackball_settings_frame (void);


#endif

