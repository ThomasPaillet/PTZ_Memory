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

#ifndef __TALLY_H
#define __TALLY_H


#include "ptz.h"


#define TSL_UMD_V5_UDP_PORT 8900


typedef struct tsl_umd_v5_packet_s {
	guint16 total_byte_count;
	guint8 minor_version_number;
	guint8 flags;
	guint16 screen;

	guint16 index;
	guint16 control;
	guint16 length;
	char text[2038];
} tsl_umd_v5_packet_t;


extern gboolean send_ip_tally;

extern struct sockaddr_in tsl_umd_v5_address;


gboolean ptz_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean ghost_ptz_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean control_window_tally_draw (GtkWidget *widget, cairo_t *cr);

gboolean ptz_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean ghost_ptz_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean control_window_name_draw (GtkWidget *widget, cairo_t *cr);

void init_tally (void);

void start_tally (void);

void stop_tally (void);


#endif

