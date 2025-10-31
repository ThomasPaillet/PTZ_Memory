/*
 * copyright (c) 2020 2021 2025 Thomas Paillet <thomas.paillet@net-c.fr>

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

#ifndef __PTZ_H
#define __PTZ_H


#include <gtk/gtk.h>
#include "network_header.h"

#include "memory.h"
#include "ultimatte.h"


#define AW_HE130 0
#define AW_HE145 1
#define AW_UE80 2
#define AW_UE150 3
#define AW_UE150A 4
#define AW_UE160 5
#define UNKNOWN_PTZ 6

#define MAX_CAMERAS_SET 8
#define MAX_CAMERAS 15
#define MAX_MEMORIES 15


typedef struct {
	char name[9];
	int index;
	gboolean active;
	gboolean is_on;

	char ip_address[16];
	gboolean ip_address_is_valid;

	struct sockaddr_in address;

	GSList *other_ptz_slist;	//with_the_same_ip_address;

	int model;

	int jpeg_page;
	char cmd_buffer[272];
	char *last_cmd;
	gboolean cam_ptz;
	char last_ctrl_cmd[16];
	int last_ctrl_cmd_len;

	gint64 last_time;
	GMutex cmd_mutex;

	int error_code;

	guint16 tally_data;
	double tally_brightness;
	gboolean tally_1_is_on;

	memory_t memories[MAX_MEMORIES];
	int number_of_memories;
	memory_t *previous_loaded_memory;

	GtkWidget *name_separator;
	GtkWidget *name_grid;
	GtkWidget *name_drawing_area;
	gboolean enter_notify_name_drawing_area;
	gboolean enter_notify_ultimatte_picto;
	GtkWidget *error_drawing_area;
	gchar *error_drawing_area_tooltip;
	GtkWidget *memories_separator;
	GtkWidget *memories_grid;
	GtkWidget *tally[6];

	GtkWidget *ghost_body;

	gboolean auto_focus;

	unsigned int zoom_position;
	char zoom_position_cmd[8];

	unsigned int focus_position;
	char focus_position_cmd[8];

	GMutex lens_information_mutex;

	gint32 free_d_Pan;
	gint32 free_d_Tilt;

	gint32 free_d_optical_axis_height;

	gint32 free_d_X;
	gint32 free_d_Y;
	gint32 free_d_Z;
	gint32 free_d_P;

	gint32 incomming_free_d_X;
	gint32 incomming_free_d_Y;
	gint32 incomming_free_d_Z;
	gint32 incomming_free_d_Pan;

	gboolean monitor_pan_tilt;
	GThread *monitor_pan_tilt_thread;

	GMutex free_d_mutex;

	ultimatte_t *ultimatte;
} ptz_t;

typedef struct {
	ptz_t *ptz;
	GThread *thread;
} ptz_thread_t;


extern char *ptz_model[];
extern gint32 ptz_optical_axis_height[];


void init_ptz (ptz_t *ptz);

gboolean ptz_is_on (ptz_t *ptz);

gboolean ptz_is_off (ptz_t *ptz);

gpointer monitor_ptz_pan_tilt_position (ptz_t *ptz);

gboolean free_ptz_thread (ptz_thread_t *ptz_thread);

gpointer start_ptz (ptz_thread_t *ptz_thread);

gpointer switch_ptz_on (ptz_thread_t *ptz_thread);

gpointer switch_ptz_off (ptz_thread_t *ptz_thread);

gboolean update_auto_focus_toggle_button (ptz_t *ptz);

void create_ptz_widgets_horizontal (ptz_t *ptz);

void create_ptz_widgets_vertical (ptz_t *ptz);

void create_ghost_ptz_widgets_horizontal (ptz_t *ptz);

void create_ghost_ptz_widgets_vertical (ptz_t *ptz);


#endif

