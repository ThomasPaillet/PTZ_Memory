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

#ifndef __CAMERAS_SET_H
#define __CAMERAS_SET_H


#include "ptz.h"
#include "interface.h"


#define CAMERAS_SET_NAME_LENGTH 20


typedef struct cameras_set_s {
	char name[CAMERAS_SET_NAME_LENGTH + 1];

	int number_of_cameras;
	ptz_t *cameras[MAX_CAMERAS];

	int number_of_ghost_cameras;

	interface_param_t layout;

	GtkWidget *page;
	gint page_num;
	GtkWidget *page_box;

	GtkWidget *linked_memories_names_entries;
	GtkWidget *entry_widgets_padding;
	GtkWidget *entry_widgets[MAX_MEMORIES];
	GtkAdjustment *entry_scrolled_window_adjustment;

	GtkWidget *name_grid_box;
	char ptz_name_font[17];
	char ghost_ptz_name_font[17];

	GtkWidget *memories_grid_box;
	char memory_name_font[17];
	GtkAdjustment *memories_scrolled_window_adjustment;

	GtkWidget *linked_memories_names_labels;
	GtkWidget *memories_labels_padding;
	GtkWidget *memories_labels[MAX_MEMORIES];
	GtkAdjustment *label_scrolled_window_adjustment;

	GtkWidget *memories_scrollbar_padding;
	GtkAdjustment *memories_scrollbar_adjustment;

	GtkAdjustment *scrolled_window_adjustment;

	GtkWidget *list_box_row;

	struct cameras_set_s *next;
} cameras_set_t;


extern const char cameras_label[];

extern GMutex cameras_sets_mutex;

extern int number_of_cameras_sets;

extern cameras_set_t *cameras_sets;

extern cameras_set_t *current_cameras_set;

extern cameras_set_t *cameras_set_with_error;


void show_cameras_set_configuration_window (void);

void add_cameras_set (void);

void delete_cameras_set (void);

void fill_cameras_set_page (cameras_set_t *cameras_set);

void add_cameras_set_to_main_window_notebook (cameras_set_t *cameras_set);

void update_current_cameras_set_vertical_margins (void);

void update_current_cameras_set_horizontal_margins (void);


#endif

