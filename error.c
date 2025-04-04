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

#include "error.h"

#include "cameras_set.h"

#include <stdio.h>


#define PTZ_ERROR(f,c,s) gboolean f (struct in_addr *src_in_addr) \
{ \
	ptz_t *ptz; \
	cameras_set_t *cameras_set_itr; \
	int i; \
	struct timeval current_time; \
	struct tm *time; \
 \
	ptz = NULL; \
 \
	g_mutex_lock (&cameras_sets_mutex); \
 \
	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) { \
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) { \
			if (cameras_set_itr->cameras[i]->address.sin_addr.s_addr == src_in_addr->s_addr) { \
				if (ptz == NULL) { \
					ptz = cameras_set_itr->cameras[i]; \
 \
					gettimeofday (&current_time, NULL); \
					time = localtime (&current_time.tv_sec); \
 \
					fprintf (error_log_file, "%02dh %02dm %02ds: Camera %s (%s) -> %s\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_address, s); \
				} else ptz = cameras_set_itr->cameras[i]; \
 \
				ptz->error_code = c; \
				gtk_widget_queue_draw (ptz->error_drawing_area); \
				gtk_widget_set_tooltip_text (ptz->error_drawing_area, s); \
 \
				break; \
			} \
		} \
	} \
 \
	g_mutex_unlock (&cameras_sets_mutex); \
 \
	g_free (src_in_addr); \
 \
	return G_SOURCE_REMOVE; \
}

#define PTZ_ERROR_S(f,c,s,m) gboolean f##_##m (struct in_addr *src_in_addr) \
{ \
	ptz_t *ptz; \
	cameras_set_t *cameras_set_itr; \
	int i; \
	struct timeval current_time; \
	struct tm *time; \
 \
	ptz = NULL; \
 \
	g_mutex_lock (&cameras_sets_mutex); \
 \
	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) { \
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) { \
			if (cameras_set_itr->cameras[i]->address.sin_addr.s_addr == src_in_addr->s_addr) { \
				if (ptz == NULL) { \
					ptz = cameras_set_itr->cameras[i]; \
 \
					gettimeofday (&current_time, NULL); \
					time = localtime (&current_time.tv_sec); \
 \
					fprintf (error_log_file, "%02dh %02dm %02ds: Camera %s (%s) -> %s\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_address, s); \
				} else ptz = cameras_set_itr->cameras[i]; \
 \
				ptz->error_code = c; \
				gtk_widget_queue_draw (ptz->error_drawing_area); \
				gtk_widget_set_tooltip_text (ptz->error_drawing_area, s); \
 \
				break; \
			} \
		} \
	} \
 \
	g_mutex_unlock (&cameras_sets_mutex); \
 \
	g_free (src_in_addr); \
 \
	return G_SOURCE_REMOVE; \
}


FILE *error_log_file = NULL;


gboolean camera_is_unreachable (ptz_t *ptz)
{
	struct timeval current_time;
	struct tm *time;

	ptz_is_off (ptz);

	gtk_widget_queue_draw (ptz->error_drawing_area);
	gtk_widget_set_tooltip_text (ptz->error_drawing_area, "La caméra n'est pas connectée au réseau");

	gettimeofday (&current_time, NULL);
	time = localtime (&current_time.tv_sec);

	fprintf (error_log_file, "%02dh %02dm %02ds: La caméra %s (%s) n'est pas connectée au réseau\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_address);

	return G_SOURCE_REMOVE;
}

gboolean clear_ptz_error (struct in_addr *src_in_addr)
{
	ptz_t *ptz;
	cameras_set_t *cameras_set_itr;
	int i;
	struct timeval current_time;
	struct tm *time;

	ptz = NULL;

	g_mutex_lock (&cameras_sets_mutex);

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			if (cameras_set_itr->cameras[i]->address.sin_addr.s_addr == src_in_addr->s_addr) {
				if (ptz == NULL) {
					ptz = cameras_set_itr->cameras[i];

					gettimeofday (&current_time, NULL);
					time = localtime (&current_time.tv_sec);

					fprintf (error_log_file, "%02dh %02dm %02ds: Camera %s (%s) -> Normal\n", time->tm_hour, time->tm_min, time->tm_sec, ptz->name, ptz->ip_address);
				} else ptz = cameras_set_itr->cameras[i];

				ptz->error_code = 0x00;
				gtk_widget_queue_draw (ptz->error_drawing_area);
				gtk_widget_set_tooltip_text (ptz->error_drawing_area, NULL);

				break;
			}
		}
	}

	g_mutex_unlock (&cameras_sets_mutex);

	g_free (src_in_addr);

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
PTZ_ERROR(ABB_execution_failed,0x60,"ABB execution failed")
PTZ_ERROR(ABB_execution_failed_busy_status,0x61,"ABB execution failed (busy status)")

PTZ_ERROR_S(Network_communication_Error,0x24,"Network communication Error",AW_HE130)
PTZ_ERROR_S(CAMERA_communication_Error,0x25,"CAMERA communication Error",AW_HE130)
PTZ_ERROR_S(CAMERA_RX_Over_run_Error,0x26,"CAMERA RX Over run Error",AW_HE130)
PTZ_ERROR_S(CAMERA_RX_Framing_Error,0x27,"CAMERA RX Framing Error",AW_HE130)
PTZ_ERROR_S(CAMERA_RX_Command_Buffer_Overflow,0x28,"CAMERA RX Command Buffer Overflow",AW_HE130)

PTZ_ERROR_S(FPGA_Config_Error,0x23,"FPGA Config Error",AW_UE150)
PTZ_ERROR_S(NET_Life_monitoring_Error,0x24,"NET Life-monitoring Error",AW_UE150)
PTZ_ERROR_S(BE_Life_monitoring_Error,0x25,"BE Life-monitoring Error",AW_UE150)
PTZ_ERROR_S(IF_BE_UART_Buffer_Overflow,0x26,"IF/BE UART Buffer Overflow",AW_UE150)
PTZ_ERROR_S(IF_BE_UART_Framing_Error,0x27,"IF/BE UART Framing Error",AW_UE150)
PTZ_ERROR_S(IF_BE_UART_Buffer_Overflow_2,0x28,"IF/BE UART Buffer Overflow",AW_UE150)
PTZ_ERROR_S(CAM_Life_monitoring_Error,0x29,"CAM Life-monitoring Error",AW_UE150)
PTZ_ERROR_S(Fan1_error,0x31,"Fan1 error",AW_UE150)
PTZ_ERROR_S(Fan2_error,0x32,"Fan2 error",AW_UE150)
PTZ_ERROR_S(High_Temp,0x33,"High Temp",AW_UE150)
PTZ_ERROR_S(Low_Temp,0x36,"Low Temp",AW_UE150)
PTZ_ERROR_S(Temp_Sensor_Error,0x40,"Temp Sensor Error",AW_UE150)
PTZ_ERROR_S(Lens_Initialize_Error,0x41,"Lens Initialize Error",AW_UE150)
PTZ_ERROR_S(PT_Initialize_Error,0x42,"PT. Initialize Error",AW_UE150)
PTZ_ERROR_S(MR_Level_Error,0x50,"MR Level Error",AW_UE150)
PTZ_ERROR_S(MR_Offset_Error,0x52,"MR Offset Error",AW_UE150)
PTZ_ERROR_S(Origin_Offset_Error,0x53,"Origin Offset Error",AW_UE150)
PTZ_ERROR_S(Angle_MR_Sensor_Error,0x54,"Angle MR Sensor Error",AW_UE150)
PTZ_ERROR_S(PT_Gear_Error,0x55,"PT. Gear Error",AW_UE150)
PTZ_ERROR_S(Motor_Disconnect_Error,0x56,"Motor Disconnect Error",AW_UE150)

gboolean error_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz)
{
	if (ptz->error_code == 0x00) {
		if (gtk_widget_is_sensitive (widget) && (ptz->enter_notify_name_drawing_area)) cairo_set_source_rgb (cr, 0.2, 0.223529412, 0.231372549);
		else cairo_set_source_rgb (cr, 0.176470588, 0.196078431, 0.203921569);
	} else if (ptz->error_code == CAMERA_IS_UNREACHABLE_ERROR) cairo_set_source_rgb (cr, 0.8, 0.0, 0.0);
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

