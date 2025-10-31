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

#include "ptz.h"

#include "cameras_set.h"
#include "control_window.h"
#include "controller.h"
#include "error.h"
#include "free_d.h"
#include "interface.h"
#include "logging.h"
#include "main_window.h"
#include "protocol.h"
#include "settings.h"
#include "sw_p_08.h"
#include "tally.h"

#include <unistd.h>	//usleep


char *ptz_model[] = { "AW-HE130", "AW-HE145", "AW-UE80", "AW-UE150", "AW-UE150A", "AW-UE160", "UNKNOWN" };
gint32 ptz_optical_axis_height[] = { 175 * 64, 210.5 * 64, 147.5 * 64, 210.5 * 64, 211.1 * 64, 219.7 * 64, 0 };


void init_ptz (ptz_t *ptz)
{
	int i;

	ptz->active = TRUE;
	ptz->is_on = FALSE;
	ptz->ip_address[0] = '\0';
	ptz->ip_address_is_valid = FALSE;

	ptz->other_ptz_slist = NULL;

	ptz->model = AW_UE150;

	init_ptz_cmd (ptz);

	ptz->error_code = 0x00;

	ptz->tally_data = 0x00;
	ptz->tally_1_is_on = FALSE;

	for (i = 0; i < MAX_MEMORIES; i++) {
		ptz->memories[i].ptz_ptr = ptz;
		ptz->memories[i].index = i;
		ptz->memories[i].empty = TRUE;
		ptz->memories[i].is_loaded = FALSE;
		ptz->memories[i].image = NULL;
		ptz->memories[i].pan_tilt_position_cmd[0] = '#';
		ptz->memories[i].pan_tilt_position_cmd[1] = 'A';
		ptz->memories[i].pan_tilt_position_cmd[2] = 'P';
		ptz->memories[i].pan_tilt_position_cmd[3] = 'C';
		ptz->memories[i].pan_tilt_position_cmd[12] = '\0';
		ptz->memories[i].name[0] = '\0';
		ptz->memories[i].name_len = 0;
	}

	ptz->number_of_memories = 0;
	ptz->previous_loaded_memory = NULL;

	ptz->name_grid = NULL;

	ptz->enter_notify_name_drawing_area = FALSE;
	ptz->enter_notify_ultimatte_picto = FALSE;

	ptz->error_drawing_area_tooltip = NULL;

	ptz->zoom_position = 0xAAA;

	ptz->zoom_position_cmd[0] = '#';
	ptz->zoom_position_cmd[1] = 'A';
	ptz->zoom_position_cmd[2] = 'X';
	ptz->zoom_position_cmd[3] = 'Z';
	ptz->zoom_position_cmd[4] = 'A';
	ptz->zoom_position_cmd[5] = 'A';
	ptz->zoom_position_cmd[6] = 'A';
	ptz->zoom_position_cmd[7] = '\0';

	ptz->focus_position = 0xAAA;

	ptz->focus_position_cmd[0] = '#';
	ptz->focus_position_cmd[1] = 'A';
	ptz->focus_position_cmd[2] = 'X';
	ptz->focus_position_cmd[3] = 'F';
	ptz->focus_position_cmd[4] = 'A';
	ptz->focus_position_cmd[5] = 'A';
	ptz->focus_position_cmd[6] = 'A';
	ptz->focus_position_cmd[7] = '\0';

	g_mutex_init (&ptz->lens_information_mutex);

	ptz->free_d_Pan = 0x00;
	ptz->free_d_Tilt = 0x00;

	ptz->free_d_X = 0x00;
	ptz->free_d_Y = 0x00;
	ptz->free_d_Z = 0x00;
	ptz->free_d_P = 0x00;

	ptz->free_d_optical_axis_height = 0x00;

	ptz->incomming_free_d_X = 0x00;
	ptz->incomming_free_d_Y = 0x00;
	ptz->incomming_free_d_Z = 0x00;
	ptz->incomming_free_d_Pan = 0x00;

	ptz->monitor_pan_tilt = FALSE;
	ptz->monitor_pan_tilt_thread = NULL;

	g_mutex_init (&ptz->free_d_mutex);

	ptz->ultimatte = NULL;
}

gboolean ptz_is_on (ptz_t *ptz)
{
	int i;

	ptz->is_on = TRUE;

	gtk_widget_set_sensitive (ptz->name_grid, TRUE);
	gtk_widget_set_sensitive (ptz->memories_grid, TRUE);

	if (current_cameras_set != NULL) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			if (ptz == current_cameras_set->cameras[i]) {
				if ((ptz->ultimatte != NULL) && !ptz->ultimatte->connected && (ptz->ultimatte->connection_thread == NULL))
					ptz->ultimatte->connection_thread = g_thread_new (NULL, (GThreadFunc)connect_ultimatte, ptz->ultimatte);

				break;
			}
		}
	}

	return G_SOURCE_REMOVE;
}

gboolean ptz_is_off (ptz_t *ptz)
{
	ptz->is_on = FALSE;

	if (ptz == current_ptz) hide_control_window ();

	if (((ptz->error_code != CAMERA_IS_UNREACHABLE_ERROR) && (ptz->error_code != 0x00)) || !ptz->ip_address_is_valid) {
		ptz->error_code = 0x00;
		gtk_widget_queue_draw (ptz->error_drawing_area);
		gtk_widget_set_tooltip_text (ptz->error_drawing_area, NULL);

		if (ptz->error_drawing_area_tooltip != NULL) {
			g_free (ptz->error_drawing_area_tooltip);
			ptz->error_drawing_area_tooltip = NULL;
		}
	}

	if (ptz->previous_loaded_memory != NULL) {
		ptz->previous_loaded_memory->is_loaded = FALSE;
		ptz->previous_loaded_memory = NULL;
	}

	gtk_widget_set_sensitive (ptz->name_grid, FALSE);
	gtk_widget_set_sensitive (ptz->memories_grid, FALSE);

	if ((ptz->ultimatte != NULL) && ptz->ultimatte->connected) disconnect_ultimatte (ptz->ultimatte);

	return G_SOURCE_REMOVE;
}

gpointer monitor_ptz_pan_tilt_position (ptz_t *ptz)
{
	char buffer[64];
	gint32 pan, tilt;
	GSList *slist_itr;
	ptz_t *other_ptz;
	gint64 current_time, elapsed_time;

	while (ptz->monitor_pan_tilt) {
		if (ptz == current_ptz) {
			g_mutex_lock (&ptz->cmd_mutex);

			if ((control_window_focus_timeout_id != 0) || (control_window_pan_tilt_timeout_id != 0) || (control_window_zoom_timeout_id != 0)) {
				g_mutex_unlock (&ptz->cmd_mutex);

				usleep (130000);

				continue;
			} else g_mutex_unlock (&ptz->cmd_mutex);
		}

		send_ptz_request_command_string (ptz, "#APC", buffer);

		if (buffer[3] <= '9') pan = (buffer[3] - '0') * 4096;
		else pan = (buffer[3] - '7') * 4096;
		if (buffer[4] <= '9') pan += (buffer[4] - '0') * 256;
		else pan += (buffer[4] - '7') * 256;
		if (buffer[5] <= '9') pan += (buffer[5] - '0') * 16;
		else pan += (buffer[5] - '7') * 16;
		if (buffer[6] <= '9') pan += buffer[6] - '0';
		else pan += buffer[6] - '7';

		pan = (0x8000 - pan) * 270;

		if (buffer[7] <= '9') tilt = (buffer[7] - '0') * 4096;
		else tilt = (buffer[7] - '7') * 256;
		if (buffer[8] <= '9') tilt += (buffer[8] - '0') * 256;
		else tilt += (buffer[8] - '7') * 256;
		if (buffer[9] <= '9') tilt += (buffer[9] - '0') * 16;
		else tilt += (buffer[9] - '7') * 16;
		if (buffer[10] <= '9') tilt += buffer[10] - '0';
		else tilt += buffer[10] - '7';

		tilt = (0x8000 - tilt) * 270;

		g_mutex_lock (&ptz->free_d_mutex);

		ptz->free_d_Pan = pan;
		ptz->free_d_Tilt = tilt;

		g_mutex_unlock (&ptz->free_d_mutex);

		if (ptz == current_ptz) gtk_widget_queue_draw (control_window_pan_tilt_label);

		for (slist_itr = ptz->other_ptz_slist; slist_itr != NULL; slist_itr = slist_itr->next) {
			other_ptz = slist_itr->data;

			g_mutex_lock (&other_ptz->free_d_mutex);

			other_ptz->free_d_Pan = pan;
			other_ptz->free_d_Tilt = tilt;

			g_mutex_unlock (&other_ptz->free_d_mutex);

			if (other_ptz == current_ptz) gtk_widget_queue_draw (control_window_pan_tilt_label);
		}

		g_mutex_lock (&ptz->cmd_mutex);
		current_time = g_get_monotonic_time ();
		elapsed_time = current_time - ptz->last_time;
		g_mutex_unlock (&ptz->cmd_mutex);

		if (elapsed_time < 130000) usleep (130000 - elapsed_time);
	}

	return NULL;
}

gboolean free_ptz_thread (ptz_thread_t *ptz_thread)
{
	g_thread_join (ptz_thread->thread);
	g_free (ptz_thread);

	return G_SOURCE_REMOVE;
}

gpointer start_ptz (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = ptz_thread->ptz, *other_ptz;
	GSList *slist_itr;
	int response, i;
	char buffer[16];
	gboolean auto_focus;
	guint32 zoom, focus;
	gint32 free_d_optical_axis_height;

	send_update_start_cmd (ptz);

	if (ptz->error_code != CAMERA_IS_UNREACHABLE_ERROR) {
		send_ptz_request_command (ptz, "#O", &response);			//Power On/Standby

		if (response == 1) {
			send_cam_request_command_string (ptz, "QID", buffer);	//Model

			for (i = 0; i < UNKNOWN_PTZ; i++) {
				if (strcmp (buffer, ptz_model[i]) == 0) break;
			}
			ptz->model = i;

			send_cam_request_command (ptz, "QAF", &response);		//Focus Auto/Manual

			if (response == 1) auto_focus = TRUE;
			else auto_focus = FALSE;

			g_mutex_lock (&ptz->lens_information_mutex);

			ptz->auto_focus = auto_focus;

			send_ptz_request_command_string (ptz, "#LPI", buffer);	//Lens information

			ptz->zoom_position_cmd[4] = buffer[3];
			ptz->zoom_position_cmd[5] = buffer[4];
			ptz->zoom_position_cmd[6] = buffer[5];

			if (buffer[3] <= '9') zoom = (buffer[3] - '0') * 256;
			else zoom = (buffer[3] - '7') * 256;
			if (buffer[4] <= '9') zoom += (buffer[4] - '0') * 16;
			else zoom += (buffer[4] - '7') * 16;
			if (buffer[5] <= '9') zoom += buffer[5] - '0';
			else zoom += buffer[5] - '7';

			ptz->focus_position_cmd[4] = buffer[6];
			ptz->focus_position_cmd[5] = buffer[7];
			ptz->focus_position_cmd[6] = buffer[8];

			if (buffer[6] <= '9') focus = (buffer[6] - '0') * 256;
			else focus = (buffer[6] - '7') * 256;
			if (buffer[7] <= '9') focus += (buffer[7] - '0') * 16;
			else focus += (buffer[7] - '7') * 16;
			if (buffer[8] <= '9') focus += buffer[8] - '0';
			else focus += buffer[8] - '7';

			send_ptz_request_command (ptz, "#INS", &response);		//Installation position

			if (response == 1) free_d_optical_axis_height = - ptz_optical_axis_height[ptz->model];
			else free_d_optical_axis_height = ptz_optical_axis_height[ptz->model];

			g_mutex_lock (&ptz->free_d_mutex);

			ptz->zoom_position = zoom;
			ptz->focus_position = focus;

			ptz->free_d_optical_axis_height = free_d_optical_axis_height;

			g_mutex_unlock (&ptz->free_d_mutex);

			g_mutex_unlock (&ptz->lens_information_mutex);

			send_ptz_control_command (ptz, "#LPC1", TRUE);			//Lens information notification On

			g_idle_add ((GSourceFunc)ptz_is_on, ptz);

			g_mutex_lock (&cameras_sets_mutex);

			for (slist_itr = ptz->other_ptz_slist; slist_itr != NULL; slist_itr = slist_itr->next) {
				other_ptz = slist_itr->data;

				other_ptz->model = ptz->model;

				g_mutex_lock (&other_ptz->lens_information_mutex);

				other_ptz->auto_focus = auto_focus;

				other_ptz->zoom_position_cmd[4] = buffer[3];
				other_ptz->zoom_position_cmd[5] = buffer[4];
				other_ptz->zoom_position_cmd[6] = buffer[5];

				other_ptz->focus_position_cmd[4] = buffer[6];
				other_ptz->focus_position_cmd[5] = buffer[7];
				other_ptz->focus_position_cmd[6] = buffer[8];

				g_mutex_lock (&other_ptz->free_d_mutex);

				other_ptz->zoom_position = zoom;
				other_ptz->focus_position = focus;

				other_ptz->free_d_optical_axis_height = free_d_optical_axis_height;

				g_mutex_unlock (&other_ptz->free_d_mutex);

				g_mutex_unlock (&other_ptz->lens_information_mutex);

				g_idle_add ((GSourceFunc)ptz_is_on, other_ptz);
			}

			g_mutex_unlock (&cameras_sets_mutex);

			if (outgoing_free_d_started && (ptz->model == AW_HE130)) {
				g_mutex_lock (&ptz->free_d_mutex);

				if (ptz->monitor_pan_tilt_thread == NULL) {
					ptz->monitor_pan_tilt = TRUE;
					ptz->monitor_pan_tilt_thread = g_thread_new (NULL, (GThreadFunc)monitor_ptz_pan_tilt_position, ptz);
				}

				g_mutex_unlock (&ptz->free_d_mutex);
			}
		} else {
			g_idle_add ((GSourceFunc)ptz_is_off, ptz);

			g_mutex_lock (&cameras_sets_mutex);

			for (slist_itr = ptz->other_ptz_slist; slist_itr != NULL; slist_itr = slist_itr->next)
				g_idle_add ((GSourceFunc)ptz_is_off, slist_itr->data);

			g_mutex_unlock (&cameras_sets_mutex);
		}
	}

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gpointer switch_ptz_on (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = ptz_thread->ptz;

	if (ptz->error_code == CAMERA_IS_UNREACHABLE_ERROR) send_update_start_cmd (ptz);

	if (ptz->error_code != CAMERA_IS_UNREACHABLE_ERROR) send_ptz_control_command (ptz, "#O1", TRUE);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gpointer switch_ptz_off (ptz_thread_t *ptz_thread)
{
	ptz_t *ptz = ptz_thread->ptz;

	if (ptz->monitor_pan_tilt) {
		g_mutex_lock (&ptz->free_d_mutex);

		ptz->monitor_pan_tilt = FALSE;
//		g_thread_join (ptz->monitor_pan_tilt_thread);
		ptz->monitor_pan_tilt_thread = NULL;

		g_mutex_unlock (&ptz->free_d_mutex);
	}

	if (ptz->error_code == CAMERA_IS_UNREACHABLE_ERROR) send_update_start_cmd (ptz);

	if (ptz->error_code != CAMERA_IS_UNREACHABLE_ERROR) send_ptz_control_command (ptz, "#O0", TRUE);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gboolean name_drawing_area_button_press_event (GtkButton *widget, GdkEventButton *event, ptz_t *ptz)
{
	ultimatte_t *ultimatte;
	ptz_thread_t *controller_thread;

	if (event->button == GDK_BUTTON_PRIMARY) {
		ultimatte = ptz->ultimatte;

		if ((ultimatte != NULL) && (event->x > ultimatte_picto_x) && (event->y < ultimatte_picto_y)) {
			if (!ultimatte->connected && (ultimatte->connection_thread == NULL))
				ultimatte->connection_thread = g_thread_new (NULL, (GThreadFunc)connect_ultimatte, ultimatte);
			else disconnect_ultimatte (ultimatte);
		} else {
			show_control_window (ptz, GTK_WIN_POS_MOUSE);

			ask_to_connect_ptz_to_ctrl_opv (ptz);

			if (controller_is_used && controller_ip_address_is_valid) {
				controller_thread = g_malloc (sizeof (ptz_thread_t));
				controller_thread->ptz = ptz;
				controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
			}
		}
	}

	return GDK_EVENT_STOP;
}

gboolean name_drawing_area_enter_notify_event (GtkWidget *widget, GdkEvent *event, ptz_t *ptz)
{
	ptz->enter_notify_name_drawing_area = TRUE;

	gtk_widget_queue_draw (widget);
	gtk_widget_queue_draw (ptz->error_drawing_area);

	return GDK_EVENT_PROPAGATE;
}

gboolean ultimatte_picto_enter_or_leave_notify_event (GtkWidget *widget, GdkEventMotion *event, ptz_t *ptz)
{
	if (ptz->ultimatte != NULL) {
		if ((event->x > ultimatte_picto_x) && (event->y < ultimatte_picto_y)) {
			if (!ptz->enter_notify_ultimatte_picto) {
				ptz->enter_notify_ultimatte_picto = TRUE;

				gtk_widget_queue_draw (widget);
			}
		} else if (ptz->enter_notify_ultimatte_picto) {
			ptz->enter_notify_ultimatte_picto = FALSE;

			gtk_widget_queue_draw (widget);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean name_drawing_area_leave_notify_event (GtkWidget *widget, GdkEvent *event, ptz_t *ptz)
{
	ptz->enter_notify_name_drawing_area = FALSE;
	ptz->enter_notify_ultimatte_picto = FALSE;

	gtk_widget_queue_draw (widget);
	gtk_widget_queue_draw (ptz->error_drawing_area);

	return GDK_EVENT_PROPAGATE;
}

void create_ptz_widgets_horizontal (ptz_t *ptz)
{
/*
name_grid                                       memories_grid
+--------+-----------------+------------------+ +----            ----+--------+
|                   tally[0]                  | |           tally[3]          |
+--------+-----------------+------------------+ +----            ----+--------+
|tally[1]|name_drawing_area|error_drawing_area| | memories[i].button |tally[5]|
+--------+-----------------+------------------+ +----            ----+--------+
|                   tally[2]                  | |           tally[4]          |
+--------+-----------------+------------------+ +----            ----+--------+
*/
	int i;

	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		gtk_widget_set_margin_top (ptz->tally[0], interface_default.memories_button_horizontal_margins);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 0, 0, 3, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 1, 1, 1);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, interface_default.thumbnail_height, interface_default.thumbnail_height + 10);
		gtk_widget_set_events (ptz->name_drawing_area, gtk_widget_get_events (ptz->name_drawing_area) | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (ptz_name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (name_drawing_area_button_press_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "enter-notify-event", G_CALLBACK (name_drawing_area_enter_notify_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "motion-notify-event", G_CALLBACK (ultimatte_picto_enter_or_leave_notify_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "leave-notify-event", G_CALLBACK (name_drawing_area_leave_notify_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->error_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->error_drawing_area, 8, interface_default.thumbnail_height + 10);
		g_signal_connect (G_OBJECT (ptz->error_drawing_area), "draw", G_CALLBACK (error_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->error_drawing_area, 2, 1, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		gtk_widget_set_margin_bottom (ptz->tally[2], interface_default.memories_button_horizontal_margins);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 0, 2, 3, 1);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		gtk_widget_set_margin_top (ptz->tally[3], interface_default.memories_button_horizontal_margins);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, MAX_MEMORIES + 1, 1);

		for (i = 0; i < MAX_MEMORIES; i++) {
			ptz->memories[i].button = gtk_button_new ();
			gtk_widget_set_size_request (ptz->memories[i].button, interface_default.thumbnail_width + 10, interface_default.thumbnail_height + 10);
			if (i != 0) gtk_widget_set_margin_start (ptz->memories[i].button, interface_default.memories_button_vertical_margins);
			if (i != MAX_MEMORIES -1) gtk_widget_set_margin_end (ptz->memories[i].button, interface_default.memories_button_vertical_margins);
			ptz->memories[i].button_handler_id = g_signal_connect (G_OBJECT (ptz->memories[i].button), "button-press-event", G_CALLBACK (memory_button_button_press_event), ptz->memories + i);
			g_signal_connect_after (G_OBJECT (ptz->memories[i].button), "draw", G_CALLBACK (memory_name_and_outline_draw), ptz->memories + i);
			gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->memories[i].button, i, 1, 1, 1);
		}

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		gtk_widget_set_margin_bottom (ptz->tally[4], interface_default.memories_button_horizontal_margins);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 0, 2, MAX_MEMORIES + 1, 1);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], MAX_MEMORIES, 1, 1, 1);
}

void create_ptz_widgets_vertical (ptz_t *ptz)
{
/*
name_grid
+--------+------------------+--------+
|        |     tally[0]     |        |
+        +------------------+        +
|tally[1]|name_drawing_area |tally[2]|
+        +------------------+        +
|        |error_drawing_area|        |
+--------+------------------+--------+

memories_grid
+--------+------------------+--------+
|        |                  |        |

|        |memories[i].button|        |
 tally[3]                    tally[4]
|        |                  |        |
+        +------------------+        +
|        |     tally[5]     |        |
+--------+------------------+--------+
*/
	int i;

	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 1, 0, 1, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		gtk_widget_set_margin_start (ptz->tally[1], interface_default.memories_button_vertical_margins);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 0, 1, 3);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, interface_default.thumbnail_width + 10, interface_default.thumbnail_height);
		gtk_widget_set_events (ptz->name_drawing_area, gtk_widget_get_events (ptz->name_drawing_area) | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (ptz_name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (name_drawing_area_button_press_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "enter-notify-event", G_CALLBACK (name_drawing_area_enter_notify_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "motion-notify-event", G_CALLBACK (ultimatte_picto_enter_or_leave_notify_event), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "leave-notify-event", G_CALLBACK (name_drawing_area_leave_notify_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->error_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->error_drawing_area, interface_default.thumbnail_width + 10, 8);
		g_signal_connect (G_OBJECT (ptz->error_drawing_area), "draw", G_CALLBACK (error_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->error_drawing_area, 1, 2, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		gtk_widget_set_margin_end (ptz->tally[2], interface_default.memories_button_vertical_margins);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 2, 0, 1, 3);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		gtk_widget_set_margin_start (ptz->tally[3], interface_default.memories_button_vertical_margins);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, 1, MAX_MEMORIES + 1);

		for (i = 0; i < MAX_MEMORIES; i++) {
			ptz->memories[i].button = gtk_button_new ();
			gtk_widget_set_size_request (ptz->memories[i].button, interface_default.thumbnail_width + 10, interface_default.thumbnail_height + 10);
			if (i != 0) gtk_widget_set_margin_top (ptz->memories[i].button, interface_default.memories_button_horizontal_margins);
			if (i != MAX_MEMORIES -1) gtk_widget_set_margin_bottom (ptz->memories[i].button, interface_default.memories_button_horizontal_margins);
			ptz->memories[i].button_handler_id = g_signal_connect (G_OBJECT (ptz->memories[i].button), "button-press-event", G_CALLBACK (memory_button_button_press_event), ptz->memories + i);
			g_signal_connect_after (G_OBJECT (ptz->memories[i].button), "draw", G_CALLBACK (memory_name_and_outline_draw), ptz->memories + i);
			gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->memories[i].button, 1, i, 1, 1);
		}

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		gtk_widget_set_margin_end (ptz->tally[4], interface_default.memories_button_vertical_margins);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 2, 0, 1, MAX_MEMORIES + 1);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], 1, MAX_MEMORIES, 1, 1);
}

gboolean ghost_name_drawing_area_button_press_event (GtkButton *widget, GdkEventButton *event, ptz_t *ptz)
{
	if (event->button == GDK_BUTTON_PRIMARY) ask_to_connect_ptz_to_ctrl_opv (ptz);

	return GDK_EVENT_STOP;
}

gboolean ghost_body_draw (GtkWidget *widget, cairo_t *cr)
{
	cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);
	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

void create_ghost_ptz_widgets_horizontal (ptz_t *ptz)
{
/*
name_grid                    memories_grid
+--------+-----------------+ +----            ----+--------+
|         tally[0]         | |      tally[3]      |        |
+--------+-----------------+ +----            ----+        +
|tally[1]|name_drawing_area| |     ghost_body     |tally[5]|
+--------+-----------------+ +----            ----+        +
|         tally[2]         | |      tally[4]      |        |
+--------+-----------------+ +----            ----+--------+
*/
	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 0, 0, 2, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 1, 1, 1);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, interface_default.thumbnail_height + 8, interface_default.thumbnail_height / 2);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (ghost_ptz_name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (ghost_name_drawing_area_button_press_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 0, 2, 2, 1);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, MAX_MEMORIES, 1);

		ptz->ghost_body = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->ghost_body, ((interface_default.thumbnail_width + 10) * MAX_MEMORIES) + (2 * interface_default.memories_button_vertical_margins * (MAX_MEMORIES - 1)), interface_default.thumbnail_height / 2);
		g_signal_connect (G_OBJECT (ptz->ghost_body), "draw", G_CALLBACK (ghost_body_draw), NULL);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->ghost_body, 0, 1, MAX_MEMORIES, 1);

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 0, 2, MAX_MEMORIES, 1);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], MAX_MEMORIES, 0, 1, 3);
}

void create_ghost_ptz_widgets_vertical (ptz_t *ptz)
{
/*
name_grid
+--------+-----------------+--------+
|             tally[0]              |
+--------+-----------------+--------+
|tally[1]|name_drawing_area|tally[2]|
+--------+-----------------+--------+

memories_grid
+--------+-----------------+--------+
|        |                 |        |

|tally[3]|   ghost_body    |tally[4]|

|        |                 |        |
+--------+-----------------+--------+
|             tally[5]              |
+--------+-----------------+--------+
*/
	ptz->name_separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

	ptz->name_grid = gtk_grid_new ();
		ptz->tally[0] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[0], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[0]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[0], 0, 0, 3, 1);

		ptz->tally[1] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[1], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[1]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[1], 0, 1, 1, 1);

		ptz->name_drawing_area = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->name_drawing_area, interface_default.thumbnail_height / 1.5, interface_default.thumbnail_height + 8);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "draw", G_CALLBACK (ghost_ptz_name_draw), ptz);
		g_signal_connect (G_OBJECT (ptz->name_drawing_area), "button-press-event", G_CALLBACK (ghost_name_drawing_area_button_press_event), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->name_drawing_area, 1, 1, 1, 1);

		ptz->tally[2] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[2], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[2]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->name_grid), ptz->tally[2], 2, 1, 1, 1);

	ptz->memories_separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

	ptz->memories_grid = gtk_grid_new ();
		ptz->tally[3] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[3], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[3]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[3], 0, 0, 1, MAX_MEMORIES);

		ptz->ghost_body = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->ghost_body, interface_default.thumbnail_height / 1.5, ((interface_default.thumbnail_height + 10) * MAX_MEMORIES) + (2 * interface_default.memories_button_horizontal_margins * (MAX_MEMORIES - 1)));
		g_signal_connect (G_OBJECT (ptz->ghost_body), "draw", G_CALLBACK (ghost_body_draw), NULL);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->ghost_body, 1, 0, 1, MAX_MEMORIES);

		ptz->tally[4] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[4], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[4]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[4], 2, 0, 1, MAX_MEMORIES);

		ptz->tally[5] = gtk_drawing_area_new ();
		gtk_widget_set_size_request (ptz->tally[5], 4, 4);
		g_signal_connect (G_OBJECT (ptz->tally[5]), "draw", G_CALLBACK (ghost_ptz_tally_draw), ptz);
	gtk_grid_attach (GTK_GRID (ptz->memories_grid), ptz->tally[5], 0, MAX_MEMORIES, 3, 1);
}

