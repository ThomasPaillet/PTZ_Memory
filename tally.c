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

#include "tally.h"

#include "cameras_set.h"
#include "control_window.h"
#include "interface.h"
#include "protocol.h"


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


gboolean send_ip_tally = FALSE;

struct sockaddr_in tsl_umd_v5_address;
SOCKET tsl_umd_v5_socket;

GThread *tsl_umd_v5_thread = NULL;


gboolean g_source_ptz_tally_queue_draw (ptz_t *ptz)
{
	if (current_ptz == ptz) update_control_window_tally ();

	gtk_widget_queue_draw (ptz->name_drawing_area);
	gtk_widget_queue_draw (ptz->tally[0]);
	gtk_widget_queue_draw (ptz->tally[1]);
	gtk_widget_queue_draw (ptz->tally[2]);
	gtk_widget_queue_draw (ptz->tally[3]);
	gtk_widget_queue_draw (ptz->tally[4]);
	gtk_widget_queue_draw (ptz->tally[5]);

	return G_SOURCE_REMOVE;
}

gboolean ptz_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	if (ptz->tally_data & 0x30) {
		if ((ptz->tally_data & 0x10) && !(ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
		else if ((ptz->tally_data & 0x20) && !(ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
	} else if (ptz->tally_data & 0x03) {
		if (!ptz->tally_1_is_on) {
			if ((ptz->tally_data & 0x01) && !(ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
			else if ((ptz->tally_data & 0x02) && !(ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
		}
	} else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);

	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

gboolean ghost_ptz_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	if (ptz->tally_data & 0x30) {
		if ((ptz->tally_data & 0x10) && !(ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
		else if ((ptz->tally_data & 0x20) && !(ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
	} else if (ptz->tally_data & 0x03) {
		if (!ptz->tally_1_is_on) {
			if ((ptz->tally_data & 0x01) && !(ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
			else if ((ptz->tally_data & 0x02) && !(ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
		}
	} else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);

	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_tally_draw (GtkWidget *widget, cairo_t *cr)
{
	if (current_ptz->tally_data & 0x30) {
		if ((current_ptz->tally_data & 0x10) && !(current_ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, current_ptz->tally_brightness, 0.0, 0.0);
		else if ((current_ptz->tally_data & 0x20) && !(current_ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, current_ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * current_ptz->tally_brightness, 0.764705882 * current_ptz->tally_brightness, 0.0);
	} else if (current_ptz->tally_data & 0x03) {
		if (!current_ptz->tally_1_is_on) {
			if ((current_ptz->tally_data & 0x01) && !(current_ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, current_ptz->tally_brightness, 0.0, 0.0);
			else if ((current_ptz->tally_data & 0x02) && !(current_ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, current_ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * current_ptz->tally_brightness, 0.764705882 * current_ptz->tally_brightness, 0.0);
		}
	} else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);

	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

gboolean ptz_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	PangoLayout *pl;

	if (gtk_widget_is_sensitive (widget) && (ptz->enter_notify_name_drawing_area)) cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);
	else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);

	cairo_paint (cr);

	if (ptz->tally_data & 0x30) {
		if ((ptz->tally_data & 0x10) && !(ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
		else if ((ptz->tally_data & 0x20) && !(ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
	} else if (ptz->tally_data & 0x03) {
		if (!ptz->tally_1_is_on) {
			if ((ptz->tally_data & 0x01) && !(ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
			else if ((ptz->tally_data & 0x02) && !(ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
		}
	} else if ((gtk_widget_is_sensitive (widget)) && !(GTK_STATE_FLAG_BACKDROP & gtk_widget_get_state_flags (widget))) cairo_set_source_rgb (cr, 0.933333333, 0.933333333, 0.925490196);
	else cairo_set_source_rgb (cr, 0.568627451, 0.580392157, 0.580392157);

	pl = pango_cairo_create_layout (cr);

	if (interface_default.orientation) {
		if (ptz->name[1] == '\0') cairo_translate (cr, 50 * current_cameras_set->layout.thumbnail_size, 20 * current_cameras_set->layout.thumbnail_size);
		else cairo_translate (cr, 10 * current_cameras_set->layout.thumbnail_size, 20 * current_cameras_set->layout.thumbnail_size);
	} else {
		if (ptz->name[1] == '\0') cairo_translate (cr, 50 * current_cameras_set->layout.thumbnail_size, 20 * current_cameras_set->layout.thumbnail_size);
		else cairo_translate (cr, 10 * current_cameras_set->layout.thumbnail_size, 20 * current_cameras_set->layout.thumbnail_size);
	}

	pango_layout_set_text (pl, ptz->name, -1);
	pango_layout_set_font_description (pl, current_cameras_set->layout.ptz_name_font_description);

	pango_cairo_show_layout (cr, pl);

	g_object_unref(pl);

	return GDK_EVENT_PROPAGATE;
}

gboolean ghost_ptz_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	PangoLayout *pl;

	cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);

	cairo_paint (cr);

	if (ptz->tally_data & 0x30) {
		if ((ptz->tally_data & 0x10) && !(ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
		else if ((ptz->tally_data & 0x20) && !(ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
	} else if (ptz->tally_data & 0x03) {
		if (!ptz->tally_1_is_on) {
			if ((ptz->tally_data & 0x01) && !(ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, ptz->tally_brightness, 0.0, 0.0);
			else if ((ptz->tally_data & 0x02) && !(ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * ptz->tally_brightness, 0.764705882 * ptz->tally_brightness, 0.0);
		}
	} else cairo_set_source_rgb (cr, 0.568627451, 0.580392157, 0.580392157);

	pl = pango_cairo_create_layout (cr);

	if (interface_default.orientation) {
		if (ptz->name[1] == '\0') cairo_translate (cr, 60 * current_cameras_set->layout.thumbnail_size, -10 * current_cameras_set->layout.thumbnail_size);
		else cairo_translate (cr, 30 * current_cameras_set->layout.thumbnail_size, -10 * current_cameras_set->layout.thumbnail_size);
	} else {
		if (ptz->name[1] == '\0') cairo_translate (cr, 60 * current_cameras_set->layout.thumbnail_size, -10 * current_cameras_set->layout.thumbnail_size);
		else cairo_translate (cr, 30 * current_cameras_set->layout.thumbnail_size, -10 * current_cameras_set->layout.thumbnail_size);
	}

	pango_layout_set_text (pl, ptz->name, -1);
	pango_layout_set_font_description (pl, current_cameras_set->layout.ghost_ptz_name_font_description);

	pango_cairo_show_layout (cr, pl);

	g_object_unref(pl);

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_name_draw (GtkWidget *widget, cairo_t *cr)
{
	PangoLayout *pl;

	cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);

	cairo_paint (cr);

	if (current_ptz->tally_data & 0x30) {
		if ((current_ptz->tally_data & 0x10) && !(current_ptz->tally_data & 0x20)) cairo_set_source_rgb (cr, current_ptz->tally_brightness, 0.0, 0.0);
		else if ((current_ptz->tally_data & 0x20) && !(current_ptz->tally_data & 0x10)) cairo_set_source_rgb (cr, 0.0, current_ptz->tally_brightness, 0.0);
		else cairo_set_source_rgb (cr, 0.941176471 * current_ptz->tally_brightness, 0.764705882 * current_ptz->tally_brightness, 0.0);
	} else if (current_ptz->tally_data & 0x03) {
		if (!current_ptz->tally_1_is_on) {
			if ((current_ptz->tally_data & 0x01) && !(current_ptz->tally_data & 0x02)) cairo_set_source_rgb (cr, current_ptz->tally_brightness, 0.0, 0.0);
			else if ((current_ptz->tally_data & 0x02) && !(current_ptz->tally_data & 0x01)) cairo_set_source_rgb (cr, 0.0, current_ptz->tally_brightness, 0.0);
			else cairo_set_source_rgb (cr, 0.941176471 * current_ptz->tally_brightness, 0.764705882 * current_ptz->tally_brightness, 0.0);
		}
	} else cairo_set_source_rgb (cr, 0.933333333, 0.933333333, 0.925490196);

	pl = pango_cairo_create_layout (cr);

	if (current_ptz->name[1] == '\0') cairo_translate (cr, 18, 0);
	else cairo_translate (cr, 5, 0);

	pango_layout_set_text (pl, current_ptz->name, -1);
	pango_layout_set_font_description (pl, control_window_font_description);

	pango_cairo_show_layout (cr, pl);

	g_object_unref(pl);

	return GDK_EVENT_PROPAGATE;
}

void init_tally (void)
{
	memset (&tsl_umd_v5_address, 0, sizeof (struct sockaddr_in));

	tsl_umd_v5_address.sin_family = AF_INET;
	tsl_umd_v5_address.sin_port = htons (TSL_UMD_V5_UDP_PORT);
	tsl_umd_v5_address.sin_addr.s_addr = inet_addr (my_ip_address);
}

gpointer receive_tsl_umd_v5_msg (gpointer data)
{
	int msg_len;
	tsl_umd_v5_packet_t packet;
	ptz_t *ptz;

	while ((msg_len = recv (tsl_umd_v5_socket, (char*)&packet, 2048, 0)) > 1) {
		if (current_cameras_set != NULL) {
			if (packet.index < current_cameras_set->number_of_cameras) {
				ptz = current_cameras_set->cameras[packet.index];
				ptz->tally_data = packet.control;

				if ((packet.control & 0x80) && (packet.control & 0x40)) ptz->tally_brightness = 1.0;
				else if (packet.control & 0x80) ptz->tally_brightness = 0.8;
				else if (packet.control & 0x40) ptz->tally_brightness = 0.6;
				else ptz->tally_brightness = 0.4;

				if (packet.control & 0x30) {
					if (send_ip_tally && !ptz->tally_1_is_on && ptz->ip_address_is_valid && (ptz->error_code != 0x30)) send_ptz_control_command (ptz, "#DA1", TRUE);
					ptz->tally_1_is_on = TRUE;
				} else {
					if (send_ip_tally && ptz->tally_1_is_on && ptz->ip_address_is_valid && (ptz->error_code != 0x30)) send_ptz_control_command (ptz, "#DA0", TRUE);
					ptz->tally_1_is_on = FALSE;
				}

				g_idle_add ((GSourceFunc)g_source_ptz_tally_queue_draw, ptz);
			}
		}
	}

	return NULL;
}

void start_tally (void)
{
	tsl_umd_v5_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind (tsl_umd_v5_socket, (struct sockaddr *)&tsl_umd_v5_address, sizeof (struct sockaddr_in));

	tsl_umd_v5_thread = g_thread_new (NULL, receive_tsl_umd_v5_msg, NULL);
}

void stop_tally (void)
{
	shutdown (tsl_umd_v5_socket, SHUT_RD);
	closesocket (tsl_umd_v5_socket);

	if (tsl_umd_v5_thread != NULL) {
		g_thread_join (tsl_umd_v5_thread);
		tsl_umd_v5_thread = NULL;
	}
}

