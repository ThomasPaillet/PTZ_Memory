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

#include "control_window.h"

#include "cameras_set.h"
#include "controller.h"
#include "main_window.h"
#include "protocol.h"
#include "sw_p_08.h"
#include "tally.h"


#ifdef _WIN32
extern GdkPixbuf *pixbuf_pad;
extern GdkPixbuf *pixbuf_up;
extern GdkPixbuf *pixbuf_down;
#endif


char focus_near_speed_cmd[5] = "#F25";
char focus_far_speed_cmd[5] = "#F75";
char focus_stop_cmd[5] = "#F50";

char zoom_wide_speed_cmd[5] = "#Z25";
char zoom_tele_speed_cmd[5] = "#Z75";
char zoom_stop_cmd[5] = "#Z50";

char pan_tilt_speed_cmd[9] = "#PTS5050";
char pan_tilt_stop_cmd[9] = "#PTS5050";


gboolean get_control_window_position_and_size (GtkWidget *window, gpointer cr, ptz_t *ptz)
{
	gdk_window_get_origin (ptz->control_window.gdk_window, &ptz->control_window.x_root, &ptz->control_window.y_root);
	ptz->control_window.width = gdk_window_get_width (ptz->control_window.gdk_window);
	ptz->control_window.height = gdk_window_get_height (ptz->control_window.gdk_window);

	return GDK_EVENT_PROPAGATE;
}

gboolean hide_control_window (GtkWidget *window, GdkEvent *event, ptz_t *ptz)
{
	ptz->control_window.is_on_screen = FALSE;
	current_ptz = NULL;

	gtk_window_set_transient_for (GTK_WINDOW (window), NULL);

	gtk_widget_hide (window);

	gtk_event_box_set_above_child (main_event_box, FALSE);

	return GDK_EVENT_STOP;
}

gboolean control_window_key_press (GtkWidget *window, GdkEventKey *event, ptz_t *ptz)
{
	int i;
	ptz_t *new_ptz;
	ptz_thread_t *controller_thread;

	if (!ptz->control_window.key_pressed) {
		if (event->keyval == GDK_KEY_Escape) {
			hide_control_window (window, NULL, ptz);

			return GDK_EVENT_STOP;
		} else if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_KP_Up)) {
			ptz->control_window.key_pressed = TRUE;
			pan_tilt_speed_cmd[4] = '5';
			pan_tilt_speed_cmd[5] = '0';

			pan_tilt_speed_cmd[6] = '7';
			pan_tilt_speed_cmd[7] = '5';

			send_ptz_control_command (ptz, pan_tilt_speed_cmd, TRUE);
		} else if ((event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_KP_Down)) {
			ptz->control_window.key_pressed = TRUE;
			pan_tilt_speed_cmd[4] = '5';
			pan_tilt_speed_cmd[5] = '0';

			pan_tilt_speed_cmd[6] = '2';
			pan_tilt_speed_cmd[7] = '5';

			send_ptz_control_command (ptz, pan_tilt_speed_cmd, TRUE);
		} else if ((event->keyval == GDK_KEY_Left) || (event->keyval == GDK_KEY_KP_Left)) {
			ptz->control_window.key_pressed = TRUE;
			pan_tilt_speed_cmd[4] = '2';
			pan_tilt_speed_cmd[5] = '5';

			pan_tilt_speed_cmd[6] = '5';
			pan_tilt_speed_cmd[7] = '0';

			send_ptz_control_command (ptz, pan_tilt_speed_cmd, TRUE);
		} else if ((event->keyval == GDK_KEY_Right) || (event->keyval == GDK_KEY_KP_Right)) {
			ptz->control_window.key_pressed = TRUE;
			pan_tilt_speed_cmd[4] = '7';
			pan_tilt_speed_cmd[5] = '5';

			pan_tilt_speed_cmd[6] = '5';
			pan_tilt_speed_cmd[7] = '0';

			send_ptz_control_command (ptz, pan_tilt_speed_cmd, TRUE);
		} else if ((event->keyval == GDK_KEY_Page_Up) || (event->keyval == GDK_KEY_KP_Page_Up)) {
			ptz->control_window.key_pressed = TRUE;
			gtk_widget_set_state_flags (ptz->control_window.zoom_tele_button, GTK_STATE_FLAG_ACTIVE, FALSE);
			send_ptz_control_command (ptz, zoom_tele_speed_cmd, TRUE);
		} else if ((event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Down)) {
			ptz->control_window.key_pressed = TRUE;
			gtk_widget_set_state_flags (ptz->control_window.zoom_wide_button, GTK_STATE_FLAG_ACTIVE, FALSE);
			send_ptz_control_command (ptz, zoom_wide_speed_cmd, TRUE);
		} else if (((event->keyval == GDK_KEY_Home) || (event->keyval == GDK_KEY_KP_Home)) && !ptz->auto_focus) {
			ptz->control_window.key_pressed = TRUE;
			gtk_widget_set_state_flags (ptz->control_window.otaf_button, GTK_STATE_FLAG_ACTIVE, FALSE);
			send_cam_control_command (ptz, "OSE:69:1");
		} else if ((event->state & GDK_MOD1_MASK) && ((event->keyval == GDK_KEY_q) || (event->keyval == GDK_KEY_Q))) {
			hide_control_window (window, NULL, ptz);

			show_exit_confirmation_window ();

			return GDK_EVENT_STOP;
		} else if ((GDK_KEY_F1 <= event->keyval) && (event->keyval <= GDK_KEY_F15)) {
			i = event->keyval - GDK_KEY_F1;

			if ((i != ptz->index) && (i < current_cameras_set->number_of_cameras)) {
				hide_control_window (window, NULL, ptz);

				new_ptz = current_cameras_set->cameras[i];

				if (new_ptz->active && gtk_widget_get_sensitive (new_ptz->name_grid)) {
					gtk_window_set_position (GTK_WINDOW (new_ptz->control_window.window), GTK_WIN_POS_CENTER);
					show_control_window (new_ptz);

					if (trackball != NULL) gdk_device_get_position_double (mouse, NULL, &new_ptz->control_window.x, &new_ptz->control_window.y);

					ask_to_connect_ptz_to_ctrl_opv (new_ptz);

					if (controller_is_used && controller_ip_address_is_valid) {
						controller_thread = g_malloc (sizeof (ptz_thread_t));
						controller_thread->ptz_ptr = new_ptz;
						controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
					}
				}
			}
		} else if ((event->keyval == GDK_KEY_i) || (event->keyval == GDK_KEY_I)) send_jpeg_image_request_cmd (ptz);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_key_release (GtkWidget *window, GdkEventKey *event, ptz_t *ptz)
{
	if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_Left) || (event->keyval == GDK_KEY_Right)) {
		send_ptz_control_command (ptz, pan_tilt_stop_cmd, TRUE);
		ptz->control_window.key_pressed = FALSE;
	} else if ((event->keyval == GDK_KEY_KP_Up) || (event->keyval == GDK_KEY_KP_Down) || (event->keyval == GDK_KEY_KP_Left) || (event->keyval == GDK_KEY_KP_Right)) {
		send_ptz_control_command (ptz, pan_tilt_stop_cmd, TRUE);
		ptz->control_window.key_pressed = FALSE;
	} else if ((event->keyval == GDK_KEY_Page_Up) || (event->keyval == GDK_KEY_KP_Page_Up)) {
		send_ptz_control_command (ptz, zoom_stop_cmd, TRUE);
		gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.zoom_tele_button), GTK_STATE_FLAG_ACTIVE);
		ptz->control_window.key_pressed = FALSE;
	} else if ((event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Down)) {
		send_ptz_control_command (ptz, zoom_stop_cmd, TRUE);
		gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.zoom_wide_button), GTK_STATE_FLAG_ACTIVE);
		ptz->control_window.key_pressed = FALSE;
	} else if (((event->keyval == GDK_KEY_Home) || (event->keyval == GDK_KEY_KP_Home)) && !ptz->auto_focus) {
		gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.otaf_button), GTK_STATE_FLAG_ACTIVE);
		ptz->control_window.key_pressed = FALSE;
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_button_press (GtkWidget *window, GdkEventButton *event, ptz_t *ptz)
{
	if (event->button == GDK_BUTTON_PRIMARY) {
		if ((event->x_root < ptz->control_window.x_root) || (event->x_root > ptz->control_window.x_root + ptz->control_window.width) || \
			(event->y_root < ptz->control_window.y_root) || (event->y_root > ptz->control_window.y_root + ptz->control_window.height)) {
			hide_control_window (window, NULL, ptz);

			return GDK_EVENT_STOP;
		}
	}

	if ((gdk_event_get_source_device ((GdkEvent *)event) == trackball) && (!ptz->control_window.key_pressed)) {
		switch (event->button) {
			case 2: ptz->control_window.key_pressed = TRUE;
				gtk_widget_set_state_flags (ptz->control_window.otaf_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_cam_control_command (ptz, "OSE:69:1");
				break;
			case 8: ptz->control_window.key_pressed = TRUE;
				gtk_widget_set_state_flags (ptz->control_window.zoom_tele_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (ptz, zoom_tele_speed_cmd, TRUE);
				break;
			case 3: ptz->control_window.key_pressed = TRUE;
				gtk_widget_set_state_flags (ptz->control_window.zoom_wide_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (ptz, zoom_wide_speed_cmd, TRUE);
				break;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pan_tilt_speed_cmd_delayed (ptz_t *ptz)
{
	pan_tilt_speed_cmd[4] = '0' + (ptz->control_window.pan_speed / 10);
	pan_tilt_speed_cmd[5] = '0' + (ptz->control_window.pan_speed % 10);

	pan_tilt_speed_cmd[6] = '0' + (ptz->control_window.tilt_speed / 10);
	pan_tilt_speed_cmd[7] = '0' + (ptz->control_window.tilt_speed % 10);

	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, pan_tilt_speed_cmd, FALSE);

	ptz->control_window.pan_tilt_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean control_window_motion_notify (GtkWidget *window, GdkEventMotion *event, ptz_t *ptz)
{
	int pan_speed, tilt_speed;
	struct timeval current_time, elapsed_time;

	if (gdk_event_get_source_device ((GdkEvent *)event) == trackball) {
		pan_speed = (int)((event->x_root - ptz->control_window.x) / 4) + 50;
		if (pan_speed < 1) pan_speed = 1;
		else if (pan_speed > 99) pan_speed = 99;

		tilt_speed = (int)((ptz->control_window.y - event->y_root) / 4) + 50;
		if (tilt_speed < 1) tilt_speed = 1;
		else if (tilt_speed > 99) tilt_speed = 99;

		gettimeofday (&current_time, NULL);
		timersub (&current_time, &ptz->last_time, &elapsed_time);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			ptz->control_window.pan_speed = pan_speed;
			ptz->control_window.tilt_speed = tilt_speed;

			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				if (ptz->control_window.zoom_timeout_id == 0)
					ptz->control_window.pan_tilt_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
				else ptz->control_window.pan_tilt_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
			}
		} else {
			if (ptz->control_window.pan_tilt_timeout_id != 0) {
				g_source_remove (ptz->control_window.pan_tilt_timeout_id);
				ptz->control_window.pan_tilt_timeout_id = 0;
			}

			if (ptz->control_window.zoom_timeout_id == 0) {
				pan_tilt_speed_cmd[4] = '0' + (pan_speed / 10);
				pan_tilt_speed_cmd[5] = '0' + (pan_speed % 10);

				pan_tilt_speed_cmd[6] = '0' + (tilt_speed / 10);
				pan_tilt_speed_cmd[7] = '0' + (tilt_speed % 10);

				ptz->last_time = current_time;
				send_ptz_control_command (ptz, pan_tilt_speed_cmd, FALSE);
			} else {
				ptz->control_window.pan_speed = pan_speed;
				ptz->control_window.tilt_speed = tilt_speed;
				ptz->control_window.pan_tilt_timeout_id = g_timeout_add (130, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
			}
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_button_release (GtkWidget *window, GdkEventButton *event, ptz_t *ptz)
{
	if (gdk_event_get_source_device ((GdkEvent *)event) == trackball) {
		switch (event->button) {
			case 1: send_ptz_control_command (ptz, pan_tilt_stop_cmd, TRUE);
				ptz->control_window.x = event->x_root;
				ptz->control_window.y = event->y_root;
				break;
			case 2: gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.otaf_button), GTK_STATE_FLAG_ACTIVE);
				ptz->control_window.key_pressed = FALSE;
				break;
			case 8: send_ptz_control_command (ptz, zoom_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.zoom_tele_button), GTK_STATE_FLAG_ACTIVE);
				ptz->control_window.key_pressed = FALSE;
				break;
			case 3: send_ptz_control_command (ptz, zoom_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (ptz->control_window.zoom_wide_button), GTK_STATE_FLAG_ACTIVE);
				ptz->control_window.key_pressed = FALSE;
				break;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

void auto_focus_toggle_button_clicked (GtkToggleButton *auto_focus_toggle_button, ptz_t *ptz)
{
	if (gtk_toggle_button_get_active (auto_focus_toggle_button)) {
		ptz->auto_focus = TRUE;
		gtk_widget_set_sensitive (ptz->control_window.focus_box, FALSE);
		send_cam_control_command (ptz, "OAF:1");
	} else send_cam_control_command (ptz, "OAF:0");
}

gboolean one_touch_auto_focus_button_pressed (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	send_cam_control_command (ptz, "OSE:69:1");

	return GDK_EVENT_PROPAGATE;
}

gboolean pts_focus_far_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, focus_far_speed_cmd, FALSE);

	ptz->control_window.focus_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean pts_focus_near_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, focus_near_speed_cmd, FALSE);

	ptz->control_window.focus_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean focus_far_button_pressed (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	if (ptz->control_window.focus_timeout_id != 0) {
		g_source_remove (ptz->control_window.focus_timeout_id);
		ptz->control_window.focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, ptz);
			else ptz->control_window.focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, focus_near_speed_cmd, FALSE);
			} else ptz->control_window.focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_near_delayed, ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, ptz);
			else ptz->control_window.focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, focus_far_speed_cmd, FALSE);
			} else ptz->control_window.focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_far_delayed, ptz);
		}
	}

	return FALSE;
}

gboolean focus_near_button_pressed (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	if (ptz->control_window.focus_timeout_id != 0) {
		g_source_remove (ptz->control_window.focus_timeout_id);
		ptz->control_window.focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, ptz);
			else ptz->control_window.focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, focus_far_speed_cmd, FALSE);
			} else ptz->control_window.focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_far_delayed, ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, ptz);
			else ptz->control_window.focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, focus_near_speed_cmd, FALSE);
			} else ptz->control_window.focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_near_delayed, ptz);
		}
	}

	return FALSE;
}

gboolean pts_focus_stop_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, focus_stop_cmd, FALSE);

	ptz->control_window.focus_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean focus_speed_button_released (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	gtk_widget_unset_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE);

	if (ptz->control_window.focus_timeout_id != 0) {
		g_source_remove (ptz->control_window.focus_timeout_id);
		ptz->control_window.focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
		if (ptz->control_window.pan_tilt_timeout_id == 0)
			ptz->control_window.focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_stop_delayed, ptz);
		else ptz->control_window.focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_stop_delayed, ptz);
	} else {
		if (ptz->control_window.pan_tilt_timeout_id == 0) {
			ptz->last_time = current_time;
			send_ptz_control_command (ptz, focus_stop_cmd, FALSE);
		} else ptz->control_window.focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_stop_delayed, ptz);
	}

	return FALSE;
}

gboolean focus_level_bar_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	int value;

	value = (0xFFF - ptz->focus_position) / 10;

	if (value <= 0) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value == 1) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value == 272) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value >= 273) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);
	} else {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	}

	cairo_rectangle (cr, 4, 1, 1, 271);
	cairo_set_source_rgb (cr, 0.6, 0.611764706, 0.615686275);
	cairo_fill (cr);

	return GDK_EVENT_PROPAGATE;
}

gboolean ptz_pad_button_press (GtkWidget *widget, GdkEventButton *event, ptz_t *ptz)
{
	if (gdk_event_get_source_device ((GdkEvent *)event) != trackball) {
		ptz->control_window.x = event->x;
		ptz->control_window.y = event->y;
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean ptz_pad_motion_notify (GtkWidget *widget, GdkEventMotion *event, ptz_t *ptz)
{
	int pan_speed, tilt_speed;
	struct timeval current_time, elapsed_time;

	if (event->state & GDK_BUTTON1_MASK) {
		pan_speed = (int)((event->x - ptz->control_window.x) / 4) + 50;
		if (pan_speed < 1) pan_speed = 1;
		else if (pan_speed > 99) pan_speed = 99;

		tilt_speed = (int)((ptz->control_window.y - event->y) / 4) + 50;
		if (tilt_speed < 1) tilt_speed = 1;
		else if (tilt_speed > 99) tilt_speed = 99;

		gettimeofday (&current_time, NULL);
		timersub (&current_time, &ptz->last_time, &elapsed_time);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			ptz->control_window.pan_speed = pan_speed;
			ptz->control_window.tilt_speed = tilt_speed;

			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				if (ptz->control_window.zoom_timeout_id == 0)
					ptz->control_window.pan_tilt_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
				else ptz->control_window.pan_tilt_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
			}
		} else {
			if (ptz->control_window.pan_tilt_timeout_id != 0) {
				g_source_remove (ptz->control_window.pan_tilt_timeout_id);
				ptz->control_window.pan_tilt_timeout_id = 0;
			}

			if (ptz->control_window.zoom_timeout_id == 0) {
				pan_tilt_speed_cmd[4] = '0' + (pan_speed / 10);
				pan_tilt_speed_cmd[5] = '0' + (pan_speed % 10);

				pan_tilt_speed_cmd[6] = '0' + (tilt_speed / 10);
				pan_tilt_speed_cmd[7] = '0' + (tilt_speed % 10);

				ptz->last_time = current_time;
				send_ptz_control_command (ptz, pan_tilt_speed_cmd, FALSE);
			} else {
				ptz->control_window.pan_speed = pan_speed;
				ptz->control_window.tilt_speed = tilt_speed;
				ptz->control_window.pan_tilt_timeout_id = g_timeout_add (130, (GSourceFunc)pan_tilt_speed_cmd_delayed, ptz);
			}
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean ptz_pad_button_release (GtkWidget *widget, GdkEventButton *event, ptz_t *ptz)
{
	if (gdk_event_get_source_device ((GdkEvent *)event) != trackball) {
		if (event->state & GDK_BUTTON1_MASK) {
			if (ptz->control_window.pan_tilt_timeout_id != 0) {
				g_source_remove (ptz->control_window.pan_tilt_timeout_id);
				ptz->control_window.pan_tilt_timeout_id = 0;
			}

			send_ptz_control_command (ptz, pan_tilt_stop_cmd, TRUE);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

void home_button_clicked (GtkButton *button, ptz_t *ptz)
{
	send_ptz_control_command (ptz, "#APC80008000", TRUE);
}

gboolean zoom_level_bar_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	int value;

	value = (0xFFF - ptz->zoom_position) / 10;

	if (value <= 0) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value == 1) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value == 272) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	} else if (value >= 273) {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, 271);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);
	} else {
		cairo_rectangle (cr, 0, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.164705882, 0.184313725, 0.188235294);	//(42, 47, 48)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.125490196, 0.129411765);	//(28, 32, 33)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.109803922, 0.129411765, 0.133333333);	//(42, 33, 34)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.290196078, 0.305882353, 0.309803922);	//(74, 78, 79)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 0, 1, 1);
		cairo_set_source_rgb (cr, 0.31372549, 0.333333333, 0.341176471);	//(80, 85, 87)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.137254902, 0.156862745, 0.160784314);	//(35, 40, 41)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.152941176, 0.17254902, 0.176470588);	//(39, 44, 45)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 1, 1, value - 1);
		cairo_set_source_rgb (cr, 0.105882353, 0.121568627, 0.125490196);	//(27, 31, 32)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.121568627, 0.384313725, 0.666666667);	//(31, 98, 170)
		cairo_fill (cr);
		cairo_rectangle (cr, 2, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.129411765, 0.356862745, 0.596078431);	//(33, 91, 152)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, value, 1, 272 - value);
		cairo_set_source_rgb (cr, 0.058823529, 0.168627451, 0.282352941);	//(15, 43, 72)
		cairo_fill (cr);

		cairo_rectangle (cr, 0, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.125490196, 0.215686275, 0.298039216);	//(32, 55, 76)
		cairo_fill (cr);
		cairo_rectangle (cr, 1, 272, 1, 2);
		cairo_set_source_rgb (cr, 0.066666667, 0.188235294, 0.31372549);	//(17, 48, 80)
		cairo_fill (cr);
		cairo_rectangle (cr, 3, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.203921569, 0.294117647, 0.376470588);	//(52, 75, 96)
		cairo_fill (cr);
		cairo_rectangle (cr, 4, 272, 1, 1);
		cairo_set_source_rgb (cr, 0.321568627, 0.341176471, 0.345098039);	//(82, 87, 88)
		cairo_fill (cr);
	}

	cairo_rectangle (cr, 4, 1, 1, 271);
	cairo_set_source_rgb (cr, 0.6, 0.611764706, 0.615686275);
	cairo_fill (cr);

	return GDK_EVENT_PROPAGATE;
}

gboolean pts_zoom_tele_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, zoom_tele_speed_cmd, FALSE);

	ptz->control_window.zoom_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean pts_zoom_wide_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, zoom_wide_speed_cmd, FALSE);

	ptz->control_window.zoom_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean zoom_tele_button_pressed (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	if (ptz->control_window.zoom_timeout_id != 0) {
		g_source_remove (ptz->control_window.zoom_timeout_id);
		ptz->control_window.zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, ptz);
			else ptz->control_window.zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, zoom_wide_speed_cmd, FALSE);
			} else ptz->control_window.zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_wide_delayed, ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, ptz);
			else ptz->control_window.zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, zoom_tele_speed_cmd, FALSE);
			} else ptz->control_window.zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_tele_delayed, ptz);
		}
	}

	return FALSE;
}

gboolean zoom_wide_button_pressed (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	if (ptz->control_window.zoom_timeout_id != 0) {
		g_source_remove (ptz->control_window.zoom_timeout_id);
		ptz->control_window.zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, ptz);
			else ptz->control_window.zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, zoom_tele_speed_cmd, FALSE);
			} else ptz->control_window.zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_tele_delayed, ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (ptz->control_window.pan_tilt_timeout_id == 0)
				ptz->control_window.zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, ptz);
			else ptz->control_window.zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, ptz);
		} else {
			if (ptz->control_window.pan_tilt_timeout_id == 0) {
				ptz->last_time = current_time;
				send_ptz_control_command (ptz, zoom_wide_speed_cmd, FALSE);
			} else ptz->control_window.zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_wide_delayed, ptz);
		}
	}

	return FALSE;
}

gboolean pts_zoom_stop_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, zoom_stop_cmd, FALSE);

	ptz->control_window.zoom_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean zoom_speed_button_released (GtkButton *button, GdkEventButton *event, ptz_t *ptz)
{
	struct timeval current_time, elapsed_time;

	gtk_widget_unset_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE);

	if (ptz->control_window.zoom_timeout_id != 0) {
		g_source_remove (ptz->control_window.zoom_timeout_id);
		ptz->control_window.zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &ptz->last_time, &elapsed_time);

	if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
		if (ptz->control_window.pan_tilt_timeout_id == 0)
			ptz->control_window.zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_stop_delayed, ptz);
		else ptz->control_window.zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_stop_delayed, ptz);
	} else {
		if (ptz->control_window.pan_tilt_timeout_id == 0) {
			ptz->last_time = current_time;
			send_ptz_control_command (ptz, zoom_stop_cmd, FALSE);
		} else ptz->control_window.zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_stop_delayed, ptz);
	}

	return FALSE;
}

void create_control_window (control_window_t *control_window, gpointer ptz)
{
	GtkWidget *main_grid, *grid, *box, *widget, *image, *event_box;

	widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated (GTK_WINDOW (widget), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (widget), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (widget), FALSE);
	g_signal_connect (G_OBJECT (widget), "draw", G_CALLBACK (get_control_window_position_and_size), ptz);
	g_signal_connect (G_OBJECT (widget), "button-press-event", G_CALLBACK (control_window_button_press), ptz);
	g_signal_connect (G_OBJECT (widget), "motion-notify-event", G_CALLBACK (control_window_motion_notify), ptz);
	g_signal_connect (G_OBJECT (widget), "button-release-event", G_CALLBACK (control_window_button_release), ptz);
	g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (control_window_key_press), ptz);
	g_signal_connect (G_OBJECT (widget), "key-release-event", G_CALLBACK (control_window_key_release), ptz);
	g_signal_connect (G_OBJECT (widget), "focus-out-event", G_CALLBACK (hide_control_window), ptz);
	g_signal_connect (G_OBJECT (widget), "delete-event", G_CALLBACK (hide_control_window), ptz);
	control_window->window = widget;

	main_grid = gtk_grid_new ();
		control_window->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (control_window->tally[0]), "draw", G_CALLBACK (control_window_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), control_window->tally[0], 0, 0, 7, 1);

		control_window->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window->tally[1], 4, 4);
		gtk_widget_set_margin_end (control_window->tally[1], MARGIN_VALUE);
		g_signal_connect (G_OBJECT (control_window->tally[1]), "draw", G_CALLBACK (control_window_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), control_window->tally[1], 0, 1, 1, 3);

		control_window->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window->tally[2], 4, 4);
		gtk_widget_set_margin_start (control_window->tally[2], MARGIN_VALUE);
		g_signal_connect (G_OBJECT (control_window->tally[2]), "draw", G_CALLBACK (control_window_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), control_window->tally[2], 6, 1, 1, 3);

		control_window->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window->tally[3], 4, 4);
		g_signal_connect (G_OBJECT (control_window->tally[3]), "draw", G_CALLBACK (control_window_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), control_window->tally[3], 0, 4, 7, 1);

		widget = gtk_label_new ("Focus");
	gtk_grid_attach (GTK_GRID (main_grid), widget, 1, 1, 1, 1);

		grid = gtk_grid_new ();
		gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
		gtk_widget_set_margin_bottom (grid, MARGIN_VALUE);
			control_window->auto_focus_toggle_button = gtk_toggle_button_new_with_label ("Auto");
			control_window->auto_focus_handler_id = g_signal_connect (G_OBJECT (control_window->auto_focus_toggle_button), "toggled", G_CALLBACK (auto_focus_toggle_button_clicked), ptz);
		gtk_grid_attach (GTK_GRID (grid), control_window->auto_focus_toggle_button, 0, 0, 1, 1);

			box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			control_window->focus_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			gtk_widget_set_margin_bottom (control_window->focus_box, 50);
#ifdef _WIN32
				image = gtk_image_new_from_pixbuf (pixbuf_up);
#elif defined (__linux)
				image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/up.png");
#endif
				widget = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (widget), image);
				gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (widget), "button_press_event", G_CALLBACK (focus_far_button_pressed), ptz);
				g_signal_connect (G_OBJECT (widget), "button_release_event", G_CALLBACK (focus_speed_button_released), ptz);
			gtk_box_pack_start (GTK_BOX (control_window->focus_box), widget, FALSE, FALSE, 0);

				control_window->otaf_button = gtk_button_new_with_label ("OTAF");
				g_signal_connect (G_OBJECT (control_window->otaf_button), "button_press_event", G_CALLBACK (one_touch_auto_focus_button_pressed), ptz);
			gtk_box_pack_start (GTK_BOX (control_window->focus_box), control_window->otaf_button, FALSE, FALSE, 0);

#ifdef _WIN32
				image = gtk_image_new_from_pixbuf (pixbuf_down);
#elif defined (__linux)
				image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/down.png");
#endif
				widget = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (widget), image);
				gtk_widget_set_margin_top (widget, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (widget), "button_press_event", G_CALLBACK (focus_near_button_pressed), ptz);
				g_signal_connect (G_OBJECT (widget), "button_release_event", G_CALLBACK (focus_speed_button_released), ptz);
			gtk_box_pack_start (GTK_BOX (control_window->focus_box), widget, FALSE, FALSE, 0);
		gtk_box_set_center_widget (GTK_BOX (box), control_window->focus_box);
		gtk_grid_attach (GTK_GRID (grid), box, 0, 1, 1, 6);

			control_window->focus_level_bar_drawing_area = gtk_drawing_area_new ();
			gtk_widget_set_size_request (control_window->focus_level_bar_drawing_area, 5, 273);
			gtk_widget_set_margin_start (control_window->focus_level_bar_drawing_area, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window->focus_level_bar_drawing_area), "draw", G_CALLBACK (focus_level_bar_draw), ptz);
		gtk_grid_attach (GTK_GRID (grid), control_window->focus_level_bar_drawing_area, 1, 0, 1, 7);
	gtk_grid_attach (GTK_GRID (main_grid), grid, 1, 2, 1, 2);

		control_window->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window->name_drawing_area, 60, 40);
		gtk_widget_set_margin_top (control_window->name_drawing_area, MARGIN_VALUE);
		gtk_widget_set_halign (control_window->name_drawing_area, GTK_ALIGN_CENTER);
		g_signal_connect (G_OBJECT (control_window->name_drawing_area), "draw", G_CALLBACK (control_window_name_draw), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), control_window->name_drawing_area, 3, 1, 1, 1);

		event_box = gtk_event_box_new ();
		gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
		g_signal_connect (G_OBJECT (event_box), "button-press-event", G_CALLBACK (ptz_pad_button_press), ptz);
		g_signal_connect (G_OBJECT (event_box), "motion-notify-event", G_CALLBACK (ptz_pad_motion_notify), ptz);
		g_signal_connect (G_OBJECT (event_box), "button-release-event", G_CALLBACK (ptz_pad_button_release), ptz);
#ifdef _WIN32
			widget = gtk_image_new_from_pixbuf (pixbuf_pad);
#elif defined (__linux)
			widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/pad.png");
#endif
		gtk_container_add (GTK_CONTAINER (event_box), widget);
	gtk_grid_attach (GTK_GRID (main_grid), event_box, 2, 2, 3, 1);

		widget = gtk_button_new_with_label ("Home");
		gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (home_button_clicked), ptz);
	gtk_grid_attach (GTK_GRID (main_grid), widget, 3, 3, 1, 1);

		widget = gtk_label_new ("Zoom");
	gtk_grid_attach (GTK_GRID (main_grid), widget, 5, 1, 1, 1);

		grid = gtk_grid_new ();
		gtk_widget_set_margin_bottom (grid, MARGIN_VALUE);
			control_window->zoom_level_bar_drawing_area = gtk_drawing_area_new ();
			gtk_widget_set_size_request (control_window->zoom_level_bar_drawing_area, 5, 273);
			gtk_widget_set_margin_end (control_window->zoom_level_bar_drawing_area, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window->zoom_level_bar_drawing_area), "draw", G_CALLBACK (zoom_level_bar_draw), ptz);
		gtk_grid_attach (GTK_GRID (grid), control_window->zoom_level_bar_drawing_area, 0, 0, 1, 6);

#ifdef _WIN32
			image = gtk_image_new_from_pixbuf (pixbuf_up);
#elif defined (__linux)
			image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/up.png");
#endif
			widget = gtk_button_new ();
			gtk_button_set_image (GTK_BUTTON (widget), image);
			gtk_widget_set_size_request (widget, 50, 20);
			gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (widget), "button_press_event", G_CALLBACK (zoom_tele_button_pressed), ptz);
			g_signal_connect (G_OBJECT (widget), "button_release_event", G_CALLBACK (zoom_speed_button_released), ptz);
			control_window->zoom_tele_button = widget;
		gtk_grid_attach (GTK_GRID (grid), widget, 1, 2, 1, 1);

#ifdef _WIN32
			image = gtk_image_new_from_pixbuf (pixbuf_down);
#elif defined (__linux)
			image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/down.png");
#endif
			widget = gtk_button_new ();
			gtk_button_set_image (GTK_BUTTON (widget), image);
			gtk_widget_set_size_request (widget, 50, 20);
			gtk_widget_set_margin_top (widget, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (widget), "button_press_event", G_CALLBACK (zoom_wide_button_pressed), ptz);
			g_signal_connect (G_OBJECT (widget), "button_release_event", G_CALLBACK (zoom_speed_button_released), ptz);
			control_window->zoom_wide_button = widget;
		gtk_grid_attach (GTK_GRID (grid), widget, 1, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (main_grid), grid, 5, 2, 1, 2);

	gtk_container_add (GTK_CONTAINER (control_window->window), main_grid);

	gtk_widget_realize (control_window->window);
	gtk_window_set_resizable (GTK_WINDOW (control_window->window), FALSE);

	control_window->gdk_window = gtk_widget_get_window (control_window->window);
}

