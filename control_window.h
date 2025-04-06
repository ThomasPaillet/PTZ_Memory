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


#include "ptz.h"


extern ptz_t *current_ptz;

extern PangoFontDescription *control_window_font_description;

extern GtkWidget *control_window_focus_level_bar_drawing_area;
extern GtkWidget *control_window_zoom_level_bar_drawing_area;

extern char focus_near_speed_cmd[5];
extern char focus_far_speed_cmd[5];

extern char zoom_wide_speed_cmd[5];
extern char zoom_tele_speed_cmd[5];


gboolean control_window_motion_notify (GtkWidget *window, GdkEventMotion *event);

gboolean update_auto_focus_state (void);

void update_control_window_tally (void);

void show_control_window (ptz_t *ptz, GtkWindowPosition position);

gboolean hide_control_window (void);

void create_control_window (void);


#endif

