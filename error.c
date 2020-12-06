/*
 * copyright (c) 2020 Thomas Paillet <thomas.paillet@net-c.fr

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
 * along with PTZ-Memory.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ptz.h"

#include <stdio.h>


#define PTZ_ERROR(f,c,s) gboolean f (ptz_t *ptz) \
{ \
	struct timeval current_time; \
	struct tm *time; \
 \
	ptz->error_code = c; \
	gtk_widget_queue_draw (ptz->error_drawing_area); \
	gtk_widget_set_tooltip_text (ptz->error_drawing_area, s); \
 \
	gettimeofday (&current_time, NULL); \
	time = localtime (&current_time.tv_sec); \
	fprintf (error_log_file, "%02dh %02dm %02ds: Camera %s (%s) -> %s\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_adresse, s); \
 \
	return G_SOURCE_REMOVE; \
}


FILE *error_log_file = NULL;


gboolean clear_ptz_error (ptz_t *ptz)
{
	struct timeval current_time;
	struct tm *time;

	ptz->error_code = 0x00;
	gtk_widget_queue_draw (ptz->error_drawing_area);
	gtk_widget_set_tooltip_text (ptz->error_drawing_area, NULL);

	gettimeofday (&current_time, NULL);
	time = localtime (&current_time.tv_sec);
	fprintf (error_log_file, "%02dh %02dm %02ds: Camera %s (%s) -> Normal\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_adresse);

	return G_SOURCE_REMOVE;
}

PTZ_ERROR(Motor_Driver_Error,0x03,"Motor Driver Error")
PTZ_ERROR(Pan_Sensor_Error,0x04,"Pan Sensor Error")
PTZ_ERROR(Tilt_Sensor_Error,0x05,"Tilt Sensor Error")
PTZ_ERROR(Controller_RX_Over_run_Error,0x06,"Controller RX Over run Error")
PTZ_ERROR(Controller_RX_Framing_Error,0x07,"Controller RX Framing Error")
PTZ_ERROR(Network_RX_Over_run_Error,0x08,"Network RX Over run Error")
PTZ_ERROR(Network_RX_Framing_Error,0x09,"Network RX Framing Error")
PTZ_ERROR(Controller_RX_Command_Buffer_Overflow,0x17,"Controller RX Command Buffer Overflow")
PTZ_ERROR(Network_RX_Command_Buffer_Overflow,0x19,"Network RX Command Buffer Overflow")
PTZ_ERROR(System_Error,0x21,"System Error")
PTZ_ERROR(Spec_Limit_Over,0x22,"Spec Limit Over")
PTZ_ERROR(Network_communication_Error,0x24,"Network communication Error")
PTZ_ERROR(CAMERA_communication_Error,0x25,"CAMERA communication Error")
PTZ_ERROR(CAMERA_RX_Over_run_Error,0x26,"CAMERA RX Over run Error")
PTZ_ERROR(CAMERA_RX_Framing_Error,0x27,"CAMERA RX Framing Error")
PTZ_ERROR(CAMERA_RX_Command_Buffer_Overflow,0x28,"CAMERA RX Command Buffer Overflow")

gboolean camera_is_unreachable (ptz_t *ptz)	//error_code = 0x30
{
	struct tm *time;

	ptz_is_off (ptz);
	gtk_widget_queue_draw (ptz->error_drawing_area);
	gtk_widget_set_tooltip_text (ptz->error_drawing_area, "La caméra n'est pas connectée au réseau");

	gettimeofday (&ptz->last_time, NULL);

	time = localtime (&ptz->last_time.tv_sec);
	fprintf (error_log_file, "%02dh %02dm %02ds: La caméra %s (%s) n'est pas connectée au réseau\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_adresse);

	return G_SOURCE_REMOVE;
}

PTZ_ERROR(ABB_execution_failed,0x40,"ABB execution failed")
PTZ_ERROR(ABB_execution_failed_busy_status,0x41,"ABB execution failed (busy status)")

gboolean error_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	if (ptz->error_code == 0x00) {
		if ((gtk_widget_is_sensitive (widget)) && !(GTK_STATE_FLAG_BACKDROP & gtk_widget_get_state_flags (widget))) {
			if (ptz->enter_notify_name_drawing_area) cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);
			else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);
		} else cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);
	} else if (ptz->error_code == 0x30) cairo_set_source_rgb (cr, 0.8, 0.0, 0.0);
	else cairo_set_source_rgb (cr, 0.960784314, 0.474509804, 0.0);

	cairo_paint (cr);

	return GDK_EVENT_PROPAGATE;
}

void start_error_log (void)
{
	struct timeval current_time;
	struct tm *time;
	char error_log_file_name[24];

	gettimeofday (&current_time, NULL);
	time = localtime (&current_time.tv_sec);
	sprintf (error_log_file_name, "20%02d-%02d-%02d_Errors.log", time->tm_year - 100, time->tm_mon + 1, time->tm_mday);
	error_log_file = fopen (error_log_file_name, "a");
}

void stop_error_log (void)
{
	fclose (error_log_file);
}

