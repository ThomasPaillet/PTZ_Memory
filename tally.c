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
#include "logging.h"
#include "protocol.h"


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
		if (ptz->name[1] == '\0') cairo_translate (cr, 50 * interface_default.thumbnail_size, 42 * interface_default.thumbnail_size);
		else cairo_translate (cr, 21 * interface_default.thumbnail_size, 42 * interface_default.thumbnail_size);
	} else {
		if (ptz->name[1] == '\0') cairo_translate (cr, 129 * interface_default.thumbnail_size + 5, 38 * interface_default.thumbnail_size);
		else cairo_translate (cr, 96 * interface_default.thumbnail_size + 5, 38 * interface_default.thumbnail_size);
	}

	pango_layout_set_text (pl, ptz->name, -1);
	pango_layout_set_font_description (pl, interface_default.ptz_name_font_description);

	pango_cairo_show_layout (cr, pl);

	if (ptz->ultimatte != NULL) {
		if (interface_default.orientation) {
			if (ptz->name[1] == '\0') cairo_translate (cr, interface_default.thumbnail_height - (72 * interface_default.thumbnail_size), - (42 * interface_default.thumbnail_size));
			else cairo_translate (cr, interface_default.thumbnail_height - (43 * interface_default.thumbnail_size), - (42 * interface_default.thumbnail_size));
		} else {
			if (ptz->name[1] == '\0') cairo_translate (cr, interface_default.thumbnail_width - (151 * interface_default.thumbnail_size) + 5, - (38 * interface_default.thumbnail_size));
			else cairo_translate (cr, interface_default.thumbnail_width - (118 * interface_default.thumbnail_size) + 5, - (38 * interface_default.thumbnail_size));
		}

		if (ptz->is_on) {
			if (ptz->ultimatte->connected) {
				if (ptz->enter_notify_ultimatte_picto) cairo_set_source_rgb (cr, 0.0, 0.9, 0.0);
				else cairo_set_source_rgb (cr, 0.0, 0.7, 0.0);
			} else {
				if (ptz->enter_notify_ultimatte_picto) cairo_set_source_rgb (cr, 0.9, 0.0, 0.0);
				else cairo_set_source_rgb (cr, 0.7, 0.0, 0.0);

				pango_layout_set_text (pl, "/", 1);
				pango_layout_set_font_description (pl, interface_default.ultimatte_picto_font_description);

				pango_cairo_show_layout (cr, pl);
			}
		} else {
			cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);

			pango_layout_set_text (pl, "/", 1);
			pango_layout_set_font_description (pl, interface_default.ultimatte_picto_font_description);

			pango_cairo_show_layout (cr, pl);
		}

		pango_layout_set_text (pl, "U", 1);
		pango_layout_set_font_description (pl, interface_default.ultimatte_picto_font_description);

		pango_cairo_show_layout (cr, pl);
	}

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
		if (ptz->name[1] == '\0') cairo_translate (cr, 64 * interface_default.thumbnail_size, 6 * interface_default.thumbnail_size);
		else cairo_translate (cr, 40 * interface_default.thumbnail_size, 6 * interface_default.thumbnail_size);
	} else {
		if (ptz->name[1] == '\0') cairo_translate (cr, 34 * interface_default.thumbnail_size, 52 * interface_default.thumbnail_size);
		else cairo_translate (cr, 10 * interface_default.thumbnail_size, 52 * interface_default.thumbnail_size);
	}

	pango_layout_set_text (pl, ptz->name, -1);
	pango_layout_set_font_description (pl, interface_default.ghost_ptz_name_font_description);

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

	if (current_ptz->name[1] == '\0') cairo_translate (cr, 23, 2);
	else cairo_translate (cr, 11, 2);

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

/*
typedef struct tsl_umd_v5_packet_s {
	guint16 total_byte_count;
	guint8 minor_version_number;
	guint8 flags;
	guint16 screen;

	guint16 index;
	guint16 control;
	guint16 length;
	char text[2036];
} tsl_umd_v5_packet_t;
*/

gpointer receive_tsl_umd_v5_packet (void)
{
	int size, ptr, i;
	char packet[2048];
	guint16 total_byte_count, index, control, length;
	ptz_t *ptz;

	while ((size = recv (tsl_umd_v5_socket, packet, 2048, 0)) > 1) {
		LOG_TSL_UMD_V5_PACKET(packet)

		if (current_cameras_set != NULL) {
			total_byte_count = *((guint16 *)packet);
			ptr = 6;

			do {
				index = *((guint16 *)(packet + ptr));
				control = *((guint16 *)(packet + ptr + 2));
				length = *((guint16 *)(packet + ptr + 4));

				if (index < current_cameras_set->number_of_cameras) {
					ptz = current_cameras_set->cameras[index];
					ptz->tally_data = control;

					if ((control & 0x80) && (control & 0x40)) ptz->tally_brightness = 1.0;
					else if (control & 0x80) ptz->tally_brightness = 0.8;
					else if (control & 0x40) ptz->tally_brightness = 0.6;
					else ptz->tally_brightness = 0.4;

					if (control & 0x30) {
						if (send_ip_tally && ptz->ip_address_is_valid && ptz->is_on && !ptz->tally_1_is_on) send_ptz_control_command (ptz, "#DA1", TRUE);
						ptz->tally_1_is_on = TRUE;
					} else {
						if (send_ip_tally && ptz->ip_address_is_valid && ptz->is_on && ptz->tally_1_is_on) send_ptz_control_command (ptz, "#DA0", TRUE);
						ptz->tally_1_is_on = FALSE;
					}

					g_idle_add ((GSourceFunc)g_source_ptz_tally_queue_draw, ptz);
				} else if (index == 0xFFFF) {
					for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
						ptz = current_cameras_set->cameras[i];
						ptz->tally_data = control;

						if ((control & 0x80) && (control & 0x40)) ptz->tally_brightness = 1.0;
						else if (control & 0x80) ptz->tally_brightness = 0.8;
						else if (control & 0x40) ptz->tally_brightness = 0.6;
						else ptz->tally_brightness = 0.4;

						if (control & 0x30) {
							if (send_ip_tally && ptz->ip_address_is_valid && ptz->is_on && !ptz->tally_1_is_on) send_ptz_control_command (ptz, "#DA1", TRUE);
							ptz->tally_1_is_on = TRUE;
						} else {
							if (send_ip_tally && ptz->ip_address_is_valid && ptz->is_on && ptz->tally_1_is_on) send_ptz_control_command (ptz, "#DA0", TRUE);
							ptz->tally_1_is_on = FALSE;
						}

						g_idle_add ((GSourceFunc)g_source_ptz_tally_queue_draw, ptz);
					}
				}

				ptr += 6 + length;
			} while (ptr <= total_byte_count - 6);
		}
	}

	return NULL;
}

void start_tally (void)
{
	tsl_umd_v5_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind (tsl_umd_v5_socket, (struct sockaddr *)&tsl_umd_v5_address, sizeof (struct sockaddr_in));

	tsl_umd_v5_thread = g_thread_new (NULL, (GThreadFunc)receive_tsl_umd_v5_packet, NULL);
}

void stop_tally (void)
{
	int i;
	ptz_t *ptz;

	shutdown (tsl_umd_v5_socket, SHUT_RD);
	closesocket (tsl_umd_v5_socket);

	if (tsl_umd_v5_thread != NULL) {
		g_thread_join (tsl_umd_v5_thread);
		tsl_umd_v5_thread = NULL;
	}

	if (send_ip_tally && (current_cameras_set != NULL)) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			ptz = current_cameras_set->cameras[i];

			if (ptz->ip_address_is_valid && ptz->is_on && ptz->tally_1_is_on) send_ptz_control_command (ptz, "#DA0", TRUE);
		}
	}
}

