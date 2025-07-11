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

#ifndef __INTERFACE_SETTINGS_H
#define __INTERFACE_SETTINGS_H


#include <glib-2.0/glib.h>
#include <pango/pango.h>


typedef struct interface_param_s {
	gboolean orientation;

	gboolean show_linked_memories_names_entries;
	gboolean show_linked_memories_names_labels;
	gboolean dont_show_not_active_cameras;

	gdouble thumbnail_size;
	int thumbnail_width;
	int thumbnail_height;

	char ptz_name_font[20];
	char ghost_ptz_name_font[20];
	char memory_name_font[20];
	char ultimatte_picto_font[20];

	PangoFontDescription *ptz_name_font_description;
	PangoFontDescription *ghost_ptz_name_font_description;
	PangoFontDescription *memory_name_font_description;
	PangoFontDescription *ultimatte_picto_font_description;

	int memories_button_vertical_margins;
	int memories_button_horizontal_margins;

	double memories_name_color_red;
	double memories_name_color_green;
	double memories_name_color_blue;

	double memories_name_backdrop_color_red;
	double memories_name_backdrop_color_green;
	double memories_name_backdrop_color_blue;
	double memories_name_backdrop_color_alpha;
} interface_param_t;


extern interface_param_t interface_default;

extern gdouble ultimatte_picto_x;
extern gdouble ultimatte_picto_y;

extern const char interface_settings_txt[];


void show_interface_settings_window (void);

void create_interface_settings_window (void);


#endif

