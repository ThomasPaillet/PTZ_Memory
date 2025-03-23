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


#ifdef _WIN32
	#include <winsock2.h>
#elif defined (__linux)
	#include <sys/socket.h>
    #include <netinet/in.h>
#endif

#include <gtk/gtk.h>
#include <sys/time.h>

#include "memory.h"
#include "control_window.h"


#define AW_HE130 0
#define AW_UE150 1

#define MAX_MEMORIES 20


typedef struct {
	char name[3];
	int index;
	gboolean active;
	gboolean is_on;

	char ip_address[16];
	char new_ip_address[16];
	gboolean ip_address_is_valid;

	struct sockaddr_in address;

	int model;

	int jpeg_page;
	char cmd_buffer[272];
	char *last_cmd;
	gboolean cam_ptz;
	char last_ctrl_cmd[16];
	int last_ctrl_cmd_len;

	struct timeval last_time;
	GMutex cmd_mutex;

	memory_t memories[MAX_MEMORIES];
	int number_of_memories;
	memory_t *previous_loaded_memory;

	gboolean auto_focus;

	int zoom_position;
	char zoom_position_cmd[8];

	int focus_position;
	char focus_position_cmd[8];

	GMutex lens_information_mutex;

	control_window_t control_window;

	GtkWidget *name_separator;
	GtkWidget *name_grid;
	GtkWidget *name_drawing_area;
	gboolean enter_notify_name_drawing_area;
	GtkWidget *memories_separator;
	GtkWidget *memories_grid;
	GtkWidget *tally[6];

	guint16 tally_data;
	double tally_brightness;
	gboolean tally_1_is_on;

	GtkWidget *error_drawing_area;
	int error_code;

	GtkWidget *ghost_body;
} ptz_t;

typedef struct {
	ptz_t *ptz_ptr;
	GThread *thread;
} ptz_thread_t;


void init_ptz (ptz_t *ptz);

gboolean ptz_is_on (ptz_t *ptz);

gboolean ptz_is_off (ptz_t *ptz);

gboolean free_ptz_thread (ptz_thread_t *ptz_thread);

gpointer start_ptz (ptz_thread_t *ptz_thread);

gpointer switch_ptz_on (ptz_thread_t *ptz_thread);

gpointer switch_ptz_off (ptz_thread_t *ptz_thread);

void show_control_window (ptz_t *ptz);

gboolean update_auto_focus_toggle_button (ptz_t *ptz);

void create_ptz_widgets_horizontal (ptz_t *ptz);

void create_ptz_widgets_vertical (ptz_t *ptz);

void create_ghost_ptz_widgets_horizontal (ptz_t *ptz);

void create_ghost_ptz_widgets_vertical (ptz_t *ptz);


extern ptz_t *current_ptz_control_window;

//cameras_set.h
#define MAX_CAMERAS_SET 8


#endif

