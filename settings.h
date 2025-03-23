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

#ifndef __SETTINGS_H
#define __SETTINGS_H


#include <gtk/gtk.h>


extern const char settings_txt[];

extern gboolean backup_needed;

extern GtkWidget *settings_window;

extern GtkWidget *settings_list_box;
extern GtkWidget *settings_new_button;


void create_settings_window (void);

void load_config_file (void);

void save_config_file (void);


#endif

