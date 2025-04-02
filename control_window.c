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
#include "trackball.h"


#ifdef _WIN32
extern GdkPixbuf *pixbuf_pad;
extern GdkPixbuf *pixbuf_up;
extern GdkPixbuf *pixbuf_down;
#endif


ptz_t *current_ptz = NULL;

GtkWidget *control_window_gtk_window;
GtkWidget *control_window_tally[4];
GtkWidget *control_window_name_drawing_area;

GtkWidget *control_window_auto_focus_toggle_button;
gulong control_window_auto_focus_handler_id;
GtkWidget *control_window_focus_box;

GtkWidget *control_window_otaf_button;
GtkWidget *control_window_focus_level_bar_drawing_area;
GtkWidget *control_window_focus_far_button;
GtkWidget *control_window_focus_near_button;

GtkWidget *control_window_zoom_level_bar_drawing_area;
GtkWidget *control_window_zoom_tele_button;
GtkWidget *control_window_zoom_wide_button;

gdouble control_window_x;
gdouble control_window_y;
int control_window_pan_speed = 0;
int control_window_tilt_speed = 0;

guint control_window_focus_timeout_id = 0;
guint control_window_pan_tilt_timeout_id = 0;
guint control_window_zoom_timeout_id = 0;

gboolean focus_is_moving = FALSE;
gboolean pan_tilt_is_moving = FALSE;
gboolean zoom_is_moving = FALSE;

char focus_near_speed_cmd[5] = "#F25";
char focus_far_speed_cmd[5] = "#F75";
char focus_stop_cmd[5] = "#F50";

char zoom_wide_speed_cmd[5] = "#Z25";
char zoom_tele_speed_cmd[5] = "#Z75";
char zoom_stop_cmd[5] = "#Z50";

char pan_tilt_speed_cmd[9] = "#PTS5050";
char pan_tilt_stop_cmd[9] = "#PTS5050";


gboolean control_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	int i;
	ptz_t *new_ptz;
	ptz_thread_t *controller_thread;

	if (event->keyval == GDK_KEY_Escape) {
		hide_control_window ();

		return GDK_EVENT_STOP;
	} else if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_KP_Up)) {
		pan_tilt_speed_cmd[4] = '5';
		pan_tilt_speed_cmd[5] = '0';

		pan_tilt_speed_cmd[6] = '7';
		pan_tilt_speed_cmd[7] = '5';

		send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, TRUE);
		pan_tilt_is_moving = TRUE;
	} else if ((event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_KP_Down)) {
		pan_tilt_speed_cmd[4] = '5';
		pan_tilt_speed_cmd[5] = '0';

		pan_tilt_speed_cmd[6] = '2';
		pan_tilt_speed_cmd[7] = '5';

		send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, TRUE);
		pan_tilt_is_moving = TRUE;
	} else if ((event->keyval == GDK_KEY_Left) || (event->keyval == GDK_KEY_KP_Left)) {
		pan_tilt_speed_cmd[4] = '2';
		pan_tilt_speed_cmd[5] = '5';

		pan_tilt_speed_cmd[6] = '5';
		pan_tilt_speed_cmd[7] = '0';

		send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, TRUE);
		pan_tilt_is_moving = TRUE;
	} else if ((event->keyval == GDK_KEY_Right) || (event->keyval == GDK_KEY_KP_Right)) {
		pan_tilt_speed_cmd[4] = '7';
		pan_tilt_speed_cmd[5] = '5';

		pan_tilt_speed_cmd[6] = '5';
		pan_tilt_speed_cmd[7] = '0';

		send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, TRUE);
		pan_tilt_is_moving = TRUE;
	} else if ((event->keyval == GDK_KEY_Page_Up) || (event->keyval == GDK_KEY_KP_Page_Up)) {
		gtk_widget_set_state_flags (control_window_zoom_tele_button, GTK_STATE_FLAG_ACTIVE, FALSE);
		send_ptz_control_command (current_ptz, zoom_tele_speed_cmd, TRUE);
		zoom_is_moving = TRUE;
	} else if ((event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Down)) {
		gtk_widget_set_state_flags (control_window_zoom_wide_button, GTK_STATE_FLAG_ACTIVE, FALSE);
		send_ptz_control_command (current_ptz, zoom_wide_speed_cmd, TRUE);
		zoom_is_moving = TRUE;
	} else if (((event->keyval == GDK_KEY_Home) || (event->keyval == GDK_KEY_KP_Home)) && !current_ptz->auto_focus) {
		gtk_widget_set_state_flags (control_window_otaf_button, GTK_STATE_FLAG_ACTIVE, FALSE);
		send_cam_control_command (current_ptz, "OSE:69:1");
	} else if ((event->state & GDK_MOD1_MASK) && ((event->keyval == GDK_KEY_q) || (event->keyval == GDK_KEY_Q))) {
		hide_control_window ();

		show_exit_confirmation_window ();

		return GDK_EVENT_STOP;
	} else if ((GDK_KEY_F1 <= event->keyval) && (event->keyval <= GDK_KEY_F15)) {
		i = event->keyval - GDK_KEY_F1;

		if (i != current_ptz->index) {
			if (i < current_cameras_set->number_of_cameras) {
				new_ptz = current_cameras_set->cameras[i];

				if (new_ptz->active && gtk_widget_get_sensitive (new_ptz->name_grid)) {
					show_control_window (new_ptz, 0);

					if (controller_is_used && controller_ip_address_is_valid) {
						controller_thread = g_malloc (sizeof (ptz_thread_t));
						controller_thread->ptz_ptr = new_ptz;
						controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
					}
				} else hide_control_window ();
			} else hide_control_window ();

			ask_to_connect_ptz_to_ctrl_opv (new_ptz);
		}
	} else if ((event->state & GDK_MOD1_MASK) && ((event->keyval == GDK_KEY_i) || (event->keyval == GDK_KEY_I))) send_jpeg_image_request_cmd (current_ptz);

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_key_release (GtkWidget *window, GdkEventKey *event)
{
	if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_Left) || (event->keyval == GDK_KEY_Right)) {
		send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
		pan_tilt_is_moving = FALSE;
	} else if ((event->keyval == GDK_KEY_KP_Up) || (event->keyval == GDK_KEY_KP_Down) || (event->keyval == GDK_KEY_KP_Left) || (event->keyval == GDK_KEY_KP_Right)) {
		send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
		pan_tilt_is_moving = FALSE;
	} else if ((event->keyval == GDK_KEY_Page_Up) || (event->keyval == GDK_KEY_KP_Page_Up)) {
		send_ptz_control_command (current_ptz, zoom_stop_cmd, TRUE);
		gtk_widget_unset_state_flags (GTK_WIDGET (control_window_zoom_tele_button), GTK_STATE_FLAG_ACTIVE);
		zoom_is_moving = FALSE;
	} else if ((event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Down)) {
		send_ptz_control_command (current_ptz, zoom_stop_cmd, TRUE);
		gtk_widget_unset_state_flags (GTK_WIDGET (control_window_zoom_wide_button), GTK_STATE_FLAG_ACTIVE);
		zoom_is_moving = FALSE;
	} else if (((event->keyval == GDK_KEY_Home) || (event->keyval == GDK_KEY_KP_Home)) && !current_ptz->auto_focus) {
		gtk_widget_unset_state_flags (GTK_WIDGET (control_window_otaf_button), GTK_STATE_FLAG_ACTIVE);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_button_press (GtkWidget *window, GdkEventButton *event)
{
	if ((gdk_event_get_source_device ((GdkEvent *)event) == trackball) && (event->button > 0) && (event->button <= 10)) {
		switch (trackball_button_action[event->button - 1]) {
			case 1: gtk_widget_set_state_flags (control_window_zoom_tele_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (current_ptz, zoom_tele_speed_cmd, TRUE);
				zoom_is_moving = TRUE;
				break;
			case 2: gtk_widget_set_state_flags (control_window_zoom_wide_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (current_ptz, zoom_wide_speed_cmd, TRUE);
				zoom_is_moving = TRUE;
				break;
			case 3: gtk_widget_set_state_flags (control_window_otaf_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_cam_control_command (current_ptz, "OSE:69:1");
				break;
			case 4: gtk_widget_set_state_flags (control_window_focus_far_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (current_ptz, focus_far_speed_cmd, TRUE);
				focus_is_moving = TRUE;
				break;
			case 5: gtk_widget_set_state_flags (control_window_focus_near_button, GTK_STATE_FLAG_ACTIVE, FALSE);
				send_ptz_control_command (current_ptz, focus_near_speed_cmd, TRUE);
				focus_is_moving = TRUE;
				break;
			case 6: send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
				break;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean control_window_button_release (GtkWidget *window, GdkEventButton *event)
{
	if ((gdk_event_get_source_device ((GdkEvent *)event) == trackball) && (event->button > 0) && (event->button <= 10)) {
		switch (trackball_button_action[event->button - 1]) {
			case 1: send_ptz_control_command (current_ptz, zoom_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (control_window_zoom_tele_button), GTK_STATE_FLAG_ACTIVE);
				zoom_is_moving = FALSE;
				break;
			case 2: send_ptz_control_command (current_ptz, zoom_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (control_window_zoom_wide_button), GTK_STATE_FLAG_ACTIVE);
				zoom_is_moving = FALSE;
				break;
			case 3: gtk_widget_unset_state_flags (GTK_WIDGET (control_window_otaf_button), GTK_STATE_FLAG_ACTIVE);
				break;
			case 4: send_ptz_control_command (current_ptz, focus_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (control_window_focus_far_button), GTK_STATE_FLAG_ACTIVE);
				focus_is_moving = FALSE;
				break;
			case 5: send_ptz_control_command (current_ptz, focus_stop_cmd, TRUE);
				gtk_widget_unset_state_flags (GTK_WIDGET (control_window_focus_near_button), GTK_STATE_FLAG_ACTIVE);
				focus_is_moving = FALSE;
				break;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pan_tilt_speed_cmd_delayed (ptz_t *ptz)
{
	pan_tilt_speed_cmd[4] = '0' + (control_window_pan_speed / 10);
	pan_tilt_speed_cmd[5] = '0' + (control_window_pan_speed % 10);

	pan_tilt_speed_cmd[6] = '0' + (control_window_tilt_speed / 10);
	pan_tilt_speed_cmd[7] = '0' + (control_window_tilt_speed % 10);

	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, pan_tilt_speed_cmd, FALSE);
	pan_tilt_is_moving = TRUE;

	control_window_pan_tilt_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean control_window_motion_notify (GtkWidget *window, GdkEventMotion *event)
{
	int pan_speed, tilt_speed;
	struct timeval current_time, elapsed_time;

	if (gdk_event_get_source_device ((GdkEvent *)event) == trackball) {
		if (!pan_tilt_is_moving) {
			control_window_x = event->x_root;
			control_window_y = event->y_root;

			control_window_pan_speed = 0;
			control_window_tilt_speed = 0;

			pan_tilt_is_moving = TRUE;
		} else {
			pan_speed = (int)((event->x_root - control_window_x) / 4) + 50;
			if (pan_speed < 1) pan_speed = 1;
			else if (pan_speed > 99) pan_speed = 99;

			tilt_speed = (int)((control_window_y - event->y_root) / 4) + 50;
			if (tilt_speed < 1) tilt_speed = 1;
			else if (tilt_speed > 99) tilt_speed = 99;

			if ((pan_speed < control_window_pan_speed) && (pan_speed < pan_tilt_stop_sensibility) && (tilt_speed < control_window_tilt_speed) && (tilt_speed < pan_tilt_stop_sensibility)) {
				send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
				pan_tilt_is_moving = FALSE;

				return GDK_EVENT_PROPAGATE;
			}

			control_window_pan_speed = pan_speed;
			control_window_tilt_speed = tilt_speed;

			gettimeofday (&current_time, NULL);
			timersub (&current_time, &current_ptz->last_time, &elapsed_time);

			if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {

				if (control_window_pan_tilt_timeout_id == 0) {
					if (control_window_zoom_timeout_id == 0)
						control_window_pan_tilt_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
					else control_window_pan_tilt_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
				}
			} else {
				if (control_window_pan_tilt_timeout_id != 0) {
					g_source_remove (control_window_pan_tilt_timeout_id);
					control_window_pan_tilt_timeout_id = 0;
				}

				if (control_window_zoom_timeout_id == 0) {
					pan_tilt_speed_cmd[4] = '0' + (pan_speed / 10);
					pan_tilt_speed_cmd[5] = '0' + (pan_speed % 10);

					pan_tilt_speed_cmd[6] = '0' + (tilt_speed / 10);
					pan_tilt_speed_cmd[7] = '0' + (tilt_speed % 10);

					current_ptz->last_time = current_time;
					send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, FALSE);
					pan_tilt_is_moving = TRUE;
				} else {
					control_window_pan_tilt_timeout_id = g_timeout_add (130, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
				}
			}
		}
	}

	return GDK_EVENT_PROPAGATE;
}

void auto_focus_toggle_button_clicked (GtkToggleButton *auto_focus_toggle_button)
{
	if (gtk_toggle_button_get_active (auto_focus_toggle_button)) {
		current_ptz->auto_focus = TRUE;
		gtk_widget_set_sensitive (control_window_focus_box, FALSE);
		send_cam_control_command (current_ptz, "OAF:1");
	} else {
		current_ptz->auto_focus = FALSE;
		gtk_widget_set_sensitive (control_window_focus_box, TRUE);
		send_cam_control_command (current_ptz, "OAF:0");
	}
}

gboolean one_touch_auto_focus_button_pressed (void)
{
	send_cam_control_command (current_ptz, "OSE:69:1");

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
	focus_is_moving = TRUE;

	control_window_focus_timeout_id = 0;

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
	focus_is_moving = TRUE;

	control_window_focus_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean focus_far_button_pressed (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	if (control_window_focus_timeout_id != 0) {
		g_source_remove (control_window_focus_timeout_id);
		control_window_focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, current_ptz);
			else control_window_focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, focus_near_speed_cmd, FALSE);
				focus_is_moving = TRUE;
			} else control_window_focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_near_delayed, current_ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, current_ptz);
			else control_window_focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, focus_far_speed_cmd, FALSE);
				focus_is_moving = TRUE;
			} else control_window_focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_far_delayed, current_ptz);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean focus_near_button_pressed (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	if (control_window_focus_timeout_id != 0) {
		g_source_remove (control_window_focus_timeout_id);
		control_window_focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, current_ptz);
			else control_window_focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_far_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, focus_far_speed_cmd, FALSE);
				focus_is_moving = TRUE;
			} else control_window_focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_far_delayed, current_ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, current_ptz);
			else control_window_focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_near_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, focus_near_speed_cmd, FALSE);
				focus_is_moving = TRUE;
			} else control_window_focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_near_delayed, current_ptz);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pts_focus_stop_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, focus_stop_cmd, FALSE);
	focus_is_moving = FALSE;

	control_window_focus_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean focus_speed_button_released (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	gtk_widget_unset_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE);

	if (control_window_focus_timeout_id != 0) {
		g_source_remove (control_window_focus_timeout_id);
		control_window_focus_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
		if (control_window_pan_tilt_timeout_id == 0)
			control_window_focus_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_stop_delayed, current_ptz);
		else control_window_focus_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_focus_stop_delayed, current_ptz);
	} else {
		if (control_window_pan_tilt_timeout_id == 0) {
			current_ptz->last_time = current_time;
			send_ptz_control_command (current_ptz, focus_stop_cmd, FALSE);
			focus_is_moving = FALSE;
		} else control_window_focus_timeout_id = g_timeout_add (130, (GSourceFunc)pts_focus_stop_delayed, current_ptz);
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean focus_level_bar_draw (GtkWidget *widget, cairo_t *cr)
{
	int value;

	value = (0xFFF - current_ptz->focus_position) / 10;

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

gboolean pad_button_press (GtkWidget *widget, GdkEventButton *event)
{
	if (gdk_event_get_source_device ((GdkEvent *)event) != trackball) {
		control_window_x = event->x;
		control_window_y = event->y;
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pad_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	int pan_speed, tilt_speed;
	struct timeval current_time, elapsed_time;

	if ((event->state & GDK_BUTTON1_MASK) && (gdk_event_get_source_device ((GdkEvent *)event) != trackball)) {
		pan_speed = (int)((event->x - control_window_x) / 4) + 50;
		if (pan_speed < 1) pan_speed = 1;
		else if (pan_speed > 99) pan_speed = 99;

		tilt_speed = (int)((control_window_y - event->y) / 4) + 50;
		if (tilt_speed < 1) tilt_speed = 1;
		else if (tilt_speed > 99) tilt_speed = 99;

		gettimeofday (&current_time, NULL);
		timersub (&current_time, &current_ptz->last_time, &elapsed_time);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			control_window_pan_speed = pan_speed;
			control_window_tilt_speed = tilt_speed;

			if (control_window_pan_tilt_timeout_id == 0) {
				if (control_window_zoom_timeout_id == 0)
					control_window_pan_tilt_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
				else control_window_pan_tilt_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
			}
		} else {
			if (control_window_pan_tilt_timeout_id != 0) {
				g_source_remove (control_window_pan_tilt_timeout_id);
				control_window_pan_tilt_timeout_id = 0;
			}

			if (control_window_zoom_timeout_id == 0) {
				pan_tilt_speed_cmd[4] = '0' + (pan_speed / 10);
				pan_tilt_speed_cmd[5] = '0' + (pan_speed % 10);

				pan_tilt_speed_cmd[6] = '0' + (tilt_speed / 10);
				pan_tilt_speed_cmd[7] = '0' + (tilt_speed % 10);

				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, pan_tilt_speed_cmd, FALSE);
				pan_tilt_is_moving = TRUE;
			} else {
				control_window_pan_speed = pan_speed;
				control_window_tilt_speed = tilt_speed;
				control_window_pan_tilt_timeout_id = g_timeout_add (130, (GSourceFunc)pan_tilt_speed_cmd_delayed, current_ptz);
			}
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pad_button_release (GtkWidget *widget, GdkEventButton *event)
{
	if ((pan_tilt_is_moving == TRUE) && (event->state & GDK_BUTTON1_MASK) && (gdk_event_get_source_device ((GdkEvent *)event) != trackball)) {
		if (control_window_pan_tilt_timeout_id != 0) {
			g_source_remove (control_window_pan_tilt_timeout_id);
			control_window_pan_tilt_timeout_id = 0;
		}

		send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
		pan_tilt_is_moving = FALSE;
	}

	return GDK_EVENT_PROPAGATE;
}

void home_button_clicked (GtkButton *button)
{
	send_ptz_control_command (current_ptz, "#APC80008000", TRUE);
}

gboolean zoom_level_bar_draw (GtkWidget *widget, cairo_t *cr)
{
	int value;

	value = (0xFFF - current_ptz->zoom_position) / 10;

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
	zoom_is_moving = TRUE;

	control_window_zoom_timeout_id = 0;

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
	zoom_is_moving = TRUE;

	control_window_zoom_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean zoom_tele_button_pressed (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	if (control_window_zoom_timeout_id != 0) {
		g_source_remove (control_window_zoom_timeout_id);
		control_window_zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
			else control_window_zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, zoom_wide_speed_cmd, FALSE);
				zoom_is_moving = TRUE;
			} else control_window_zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
			else control_window_zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, zoom_tele_speed_cmd, FALSE);
				zoom_is_moving = TRUE;
			} else control_window_zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean zoom_wide_button_pressed (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	if (control_window_zoom_timeout_id != 0) {
		g_source_remove (control_window_zoom_timeout_id);
		control_window_zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE, FALSE);

		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
			else control_window_zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, zoom_tele_speed_cmd, FALSE);
				zoom_is_moving = TRUE;
			} else control_window_zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_tele_delayed, current_ptz);
		}
	} else {
		if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
			if (control_window_pan_tilt_timeout_id == 0)
				control_window_zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
			else control_window_zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
		} else {
			if (control_window_pan_tilt_timeout_id == 0) {
				current_ptz->last_time = current_time;
				send_ptz_control_command (current_ptz, zoom_wide_speed_cmd, FALSE);
				zoom_is_moving = TRUE;
			} else control_window_zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_wide_delayed, current_ptz);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean pts_zoom_stop_delayed (ptz_t *ptz)
{
	ptz->last_time.tv_usec += 130000;
	if (ptz->last_time.tv_usec >= 1000000) {
		ptz->last_time.tv_sec++;
		ptz->last_time.tv_usec -= 1000000;
	}

	send_ptz_control_command (ptz, zoom_stop_cmd, FALSE);
	zoom_is_moving = FALSE;

	control_window_zoom_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

gboolean zoom_speed_button_released (GtkButton *button, GdkEventButton *event)
{
	struct timeval current_time, elapsed_time;

	gtk_widget_unset_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_ACTIVE);

	if (control_window_zoom_timeout_id != 0) {
		g_source_remove (control_window_zoom_timeout_id);
		control_window_zoom_timeout_id = 0;
	}

	gettimeofday (&current_time, NULL);
	timersub (&current_time, &current_ptz->last_time, &elapsed_time);

	if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) {
		if (control_window_pan_tilt_timeout_id == 0)
			control_window_zoom_timeout_id = g_timeout_add ((130000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_stop_delayed, current_ptz);
		else control_window_zoom_timeout_id = g_timeout_add ((260000 - elapsed_time.tv_usec) / 1000, (GSourceFunc)pts_zoom_stop_delayed, current_ptz);
	} else {
		if (control_window_pan_tilt_timeout_id == 0) {
			current_ptz->last_time = current_time;
			send_ptz_control_command (current_ptz, zoom_stop_cmd, FALSE);
			zoom_is_moving = FALSE;
		} else control_window_zoom_timeout_id = g_timeout_add (130, (GSourceFunc)pts_zoom_stop_delayed, current_ptz);
	}

	return FALSE;
}

gboolean update_auto_focus_state (void)
{
	g_signal_handler_block (control_window_auto_focus_toggle_button, control_window_auto_focus_handler_id);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (control_window_auto_focus_toggle_button), current_ptz->auto_focus);
	g_signal_handler_unblock (control_window_auto_focus_toggle_button, control_window_auto_focus_handler_id);

	gtk_widget_set_sensitive (control_window_focus_box, !current_ptz->auto_focus);

	return G_SOURCE_REMOVE;
}

void update_control_window_tally (void)
{
	gtk_widget_queue_draw (control_window_name_drawing_area);
	gtk_widget_queue_draw (control_window_tally[0]);
	gtk_widget_queue_draw (control_window_tally[1]);
	gtk_widget_queue_draw (control_window_tally[2]);
	gtk_widget_queue_draw (control_window_tally[3]);
}

void show_control_window (ptz_t *ptz, GtkWindowPosition position)
{
	gboolean control_window_is_hidden;

	if (current_ptz == NULL) control_window_is_hidden = TRUE;
	else control_window_is_hidden = FALSE;

	current_ptz = ptz;

	update_auto_focus_state ();

	update_control_window_tally ();

	gtk_widget_queue_draw (control_window_focus_level_bar_drawing_area);
	gtk_widget_queue_draw (control_window_zoom_level_bar_drawing_area);

	if (control_window_is_hidden) {
		gtk_window_set_position (GTK_WINDOW (control_window_gtk_window), position);

		gtk_widget_show_all (control_window_gtk_window);

		gtk_event_box_set_above_child (GTK_EVENT_BOX (main_event_box), TRUE);
		g_signal_handler_unblock (main_event_box, main_event_box_motion_notify_handler_id);
	}
}

gboolean hide_control_window (void)
{
	g_signal_handler_block (main_event_box, main_event_box_motion_notify_handler_id);
	gtk_event_box_set_above_child (GTK_EVENT_BOX (main_event_box), FALSE);

	gtk_widget_hide (control_window_gtk_window);


	if (control_window_focus_timeout_id != 0) {
		g_source_remove (control_window_focus_timeout_id);
		control_window_focus_timeout_id = 0;
	}

	if (focus_is_moving) {
		if (current_ptz->is_on) send_ptz_control_command (current_ptz, focus_stop_cmd, FALSE);
		focus_is_moving = FALSE;
	}

	if (control_window_pan_tilt_timeout_id != 0) {
		g_source_remove (control_window_pan_tilt_timeout_id);
		control_window_pan_tilt_timeout_id = 0;
	}

	if (pan_tilt_is_moving) {
		if (current_ptz->is_on) send_ptz_control_command (current_ptz, pan_tilt_stop_cmd, TRUE);
		pan_tilt_is_moving = FALSE;
	}

	if (control_window_zoom_timeout_id != 0) {
		g_source_remove (control_window_zoom_timeout_id);
		control_window_zoom_timeout_id = 0;
	}

	if (zoom_is_moving) {
		if (current_ptz->is_on) send_ptz_control_command (current_ptz, zoom_stop_cmd, FALSE);
		zoom_is_moving = FALSE;
	}

	current_ptz = NULL;

	return GDK_EVENT_STOP;
}

void create_control_window (void)
{
	GtkWidget *main_grid, *grid, *box, *widget, *image, *event_box;

	control_window_gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated (GTK_WINDOW (control_window_gtk_window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (control_window_gtk_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (control_window_gtk_window), FALSE);
	gtk_window_set_transient_for (GTK_WINDOW (control_window_gtk_window), GTK_WINDOW (main_window));
	gtk_window_set_resizable (GTK_WINDOW (control_window_gtk_window), FALSE);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "key-press-event", G_CALLBACK (control_window_key_press), NULL);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "key-release-event", G_CALLBACK (control_window_key_release), NULL);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "button-press-event", G_CALLBACK (control_window_button_press), NULL);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "button-release-event", G_CALLBACK (control_window_button_release), NULL);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "motion-notify-event", G_CALLBACK (control_window_motion_notify), NULL);
	g_signal_connect (G_OBJECT (control_window_gtk_window), "delete-event", G_CALLBACK (hide_control_window), NULL);

	main_grid = gtk_grid_new ();
		control_window_tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window_tally[0], 4, 4);
		g_signal_connect (G_OBJECT (control_window_tally[0]), "draw", G_CALLBACK (control_window_tally_draw), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), control_window_tally[0], 0, 0, 7, 1);

		control_window_tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window_tally[1], 4, 4);
		gtk_widget_set_margin_end (control_window_tally[1], MARGIN_VALUE);
		g_signal_connect (G_OBJECT (control_window_tally[1]), "draw", G_CALLBACK (control_window_tally_draw), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), control_window_tally[1], 0, 1, 1, 3);

		control_window_tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window_tally[2], 4, 4);
		gtk_widget_set_margin_start (control_window_tally[2], MARGIN_VALUE);
		g_signal_connect (G_OBJECT (control_window_tally[2]), "draw", G_CALLBACK (control_window_tally_draw), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), control_window_tally[2], 6, 1, 1, 3);

		control_window_tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window_tally[3], 4, 4);
		g_signal_connect (G_OBJECT (control_window_tally[3]), "draw", G_CALLBACK (control_window_tally_draw), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), control_window_tally[3], 0, 4, 7, 1);

		widget = gtk_label_new ("Focus");
	gtk_grid_attach (GTK_GRID (main_grid), widget, 1, 1, 1, 1);

		grid = gtk_grid_new ();
		gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
		gtk_widget_set_margin_bottom (grid, MARGIN_VALUE);
			control_window_auto_focus_toggle_button = gtk_toggle_button_new_with_label ("Auto");
			control_window_auto_focus_handler_id = g_signal_connect (G_OBJECT (control_window_auto_focus_toggle_button), "toggled", G_CALLBACK (auto_focus_toggle_button_clicked), NULL);
		gtk_grid_attach (GTK_GRID (grid), control_window_auto_focus_toggle_button, 0, 0, 1, 1);

			box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			control_window_focus_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
			gtk_widget_set_margin_bottom (control_window_focus_box, 50);
#ifdef _WIN32
				image = gtk_image_new_from_pixbuf (pixbuf_up);
#elif defined (__linux)
				image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/up.png");
#endif
				control_window_focus_far_button = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (control_window_focus_far_button), image);
				gtk_widget_set_margin_bottom (control_window_focus_far_button, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (control_window_focus_far_button), "button_press_event", G_CALLBACK (focus_far_button_pressed), NULL);
				g_signal_connect (G_OBJECT (control_window_focus_far_button), "button_release_event", G_CALLBACK (focus_speed_button_released), NULL);
			gtk_box_pack_start (GTK_BOX (control_window_focus_box), control_window_focus_far_button, FALSE, FALSE, 0);

				control_window_otaf_button = gtk_button_new_with_label ("OTAF");
				g_signal_connect (G_OBJECT (control_window_otaf_button), "button_press_event", G_CALLBACK (one_touch_auto_focus_button_pressed), NULL);
			gtk_box_pack_start (GTK_BOX (control_window_focus_box), control_window_otaf_button, FALSE, FALSE, 0);

#ifdef _WIN32
				image = gtk_image_new_from_pixbuf (pixbuf_down);
#elif defined (__linux)
				image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/down.png");
#endif
				control_window_focus_near_button = gtk_button_new ();
				gtk_button_set_image (GTK_BUTTON (control_window_focus_near_button), image);
				gtk_widget_set_margin_top (control_window_focus_near_button, MARGIN_VALUE);
				g_signal_connect (G_OBJECT (control_window_focus_near_button), "button_press_event", G_CALLBACK (focus_near_button_pressed), NULL);
				g_signal_connect (G_OBJECT (control_window_focus_near_button), "button_release_event", G_CALLBACK (focus_speed_button_released), NULL);
			gtk_box_pack_start (GTK_BOX (control_window_focus_box), control_window_focus_near_button, FALSE, FALSE, 0);
		gtk_box_set_center_widget (GTK_BOX (box), control_window_focus_box);
		gtk_grid_attach (GTK_GRID (grid), box, 0, 1, 1, 6);

			control_window_focus_level_bar_drawing_area = gtk_drawing_area_new ();
			gtk_widget_set_size_request (control_window_focus_level_bar_drawing_area, 5, 273);
			gtk_widget_set_margin_start (control_window_focus_level_bar_drawing_area, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window_focus_level_bar_drawing_area), "draw", G_CALLBACK (focus_level_bar_draw), NULL);
		gtk_grid_attach (GTK_GRID (grid), control_window_focus_level_bar_drawing_area, 1, 0, 1, 7);
	gtk_grid_attach (GTK_GRID (main_grid), grid, 1, 2, 1, 2);

		control_window_name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (control_window_name_drawing_area, 60, 40);
		gtk_widget_set_margin_top (control_window_name_drawing_area, MARGIN_VALUE);
		gtk_widget_set_halign (control_window_name_drawing_area, GTK_ALIGN_CENTER);
		g_signal_connect (G_OBJECT (control_window_name_drawing_area), "draw", G_CALLBACK (control_window_name_draw), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), control_window_name_drawing_area, 3, 1, 1, 1);

		event_box = gtk_event_box_new ();
		gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
		g_signal_connect (G_OBJECT (event_box), "button-press-event", G_CALLBACK (pad_button_press), NULL);
		g_signal_connect (G_OBJECT (event_box), "motion-notify-event", G_CALLBACK (pad_motion_notify), NULL);
		g_signal_connect (G_OBJECT (event_box), "button-release-event", G_CALLBACK (pad_button_release), NULL);
#ifdef _WIN32
			widget = gtk_image_new_from_pixbuf (pixbuf_pad);
#elif defined (__linux)
			widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/pad.png");
#endif
		gtk_container_add (GTK_CONTAINER (event_box), widget);
	gtk_grid_attach (GTK_GRID (main_grid), event_box, 2, 2, 3, 1);

		widget = gtk_button_new_with_label ("Home");
		gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (home_button_clicked), NULL);
	gtk_grid_attach (GTK_GRID (main_grid), widget, 3, 3, 1, 1);

		widget = gtk_label_new ("Zoom");
	gtk_grid_attach (GTK_GRID (main_grid), widget, 5, 1, 1, 1);

		grid = gtk_grid_new ();
		gtk_widget_set_margin_bottom (grid, MARGIN_VALUE);
			control_window_zoom_level_bar_drawing_area = gtk_drawing_area_new ();
			gtk_widget_set_size_request (control_window_zoom_level_bar_drawing_area, 5, 273);
			gtk_widget_set_margin_end (control_window_zoom_level_bar_drawing_area, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window_zoom_level_bar_drawing_area), "draw", G_CALLBACK (zoom_level_bar_draw), NULL);
		gtk_grid_attach (GTK_GRID (grid), control_window_zoom_level_bar_drawing_area, 0, 0, 1, 6);

#ifdef _WIN32
			image = gtk_image_new_from_pixbuf (pixbuf_up);
#elif defined (__linux)
			image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/up.png");
#endif
			control_window_zoom_tele_button = gtk_button_new ();
			gtk_button_set_image (GTK_BUTTON (control_window_zoom_tele_button), image);
			gtk_widget_set_size_request (control_window_zoom_tele_button, 50, 20);
			gtk_widget_set_margin_bottom (control_window_zoom_tele_button, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window_zoom_tele_button), "button_press_event", G_CALLBACK (zoom_tele_button_pressed), NULL);
			g_signal_connect (G_OBJECT (control_window_zoom_tele_button), "button_release_event", G_CALLBACK (zoom_speed_button_released), NULL);
		gtk_grid_attach (GTK_GRID (grid), control_window_zoom_tele_button, 1, 2, 1, 1);

#ifdef _WIN32
			image = gtk_image_new_from_pixbuf (pixbuf_down);
#elif defined (__linux)
			image = gtk_image_new_from_resource ("/org/PTZ-Memory/images/down.png");
#endif
			control_window_zoom_wide_button = gtk_button_new ();
			gtk_button_set_image (GTK_BUTTON (control_window_zoom_wide_button), image);
			gtk_widget_set_size_request (control_window_zoom_wide_button, 50, 20);
			gtk_widget_set_margin_top (control_window_zoom_wide_button, MARGIN_VALUE);
			g_signal_connect (G_OBJECT (control_window_zoom_wide_button), "button_press_event", G_CALLBACK (zoom_wide_button_pressed), NULL);
			g_signal_connect (G_OBJECT (control_window_zoom_wide_button), "button_release_event", G_CALLBACK (zoom_speed_button_released), NULL);
		gtk_grid_attach (GTK_GRID (grid), control_window_zoom_wide_button, 1, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (main_grid), grid, 5, 2, 1, 2);

	gtk_container_add (GTK_CONTAINER (control_window_gtk_window), main_grid);

	gtk_widget_realize (control_window_gtk_window);
}

