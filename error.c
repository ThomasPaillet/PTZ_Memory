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
#include "logging.h"

#include <stdio.h>


#define PTZ_ERROR(f,c,s) gboolean f (struct in_addr *src_in_addr) \
{ \
	GSList *slist_itr1, *slist_itr2; \
	ptz_t *ptz, *other_ptz; \
 \
	for (slist_itr1 = ptz_slist; slist_itr1 != NULL; slist_itr1 = slist_itr1->next) { \
		ptz = slist_itr1->data; \
 \
		if (ptz->address.sin_addr.s_addr == src_in_addr->s_addr) { \
			LOG_PTZ_ERROR(s); \
 \
			ptz->error_code = c; \
			gtk_widget_queue_draw (ptz->error_drawing_area); \
			gtk_widget_set_tooltip_text (ptz->error_drawing_area, s); \
 \
			for (slist_itr2 = ptz->other_ptz_slist; slist_itr2 != NULL; slist_itr2 = slist_itr2->next) { \
				other_ptz = slist_itr2->data; \
 \
				other_ptz->error_code = c; \
				gtk_widget_queue_draw (other_ptz->error_drawing_area); \
				gtk_widget_set_tooltip_text (other_ptz->error_drawing_area, s); \
			} \
 \
			break; \
		} \
	} \
 \
	g_free (src_in_addr); \
 \
	return G_SOURCE_REMOVE; \
}


gboolean camera_is_unreachable (ptz_t *ptz)
{
	ptz_t *other_ptz;
	GSList *slist_itr;

	ptz_is_off (ptz);

	gtk_widget_queue_draw (ptz->error_drawing_area);
	gtk_widget_set_tooltip_text (ptz->error_drawing_area, "La caméra n'est pas connectée au réseau");

	LOG_PTZ_STRING("is not connected physically")

	for (slist_itr = ptz->other_ptz_slist; slist_itr != NULL; slist_itr = slist_itr->next) {
		other_ptz = slist_itr->data;

		other_ptz->error_code = CAMERA_IS_UNREACHABLE_ERROR;

		ptz_is_off (other_ptz);

		gtk_widget_queue_draw (other_ptz->error_drawing_area);
		gtk_widget_set_tooltip_text (other_ptz->error_drawing_area, "La caméra n'est pas connectée au réseau");
	}

	return G_SOURCE_REMOVE;
}

gboolean clear_ptz_error (struct in_addr *src_in_addr)
{
	GSList *slist_itr1, *slist_itr2;
	ptz_t *ptz, *other_ptz;

	for (slist_itr1 = ptz_slist; slist_itr1 != NULL; slist_itr1 = slist_itr1->next) {
		ptz = slist_itr1->data;

		if (ptz->address.sin_addr.s_addr == src_in_addr->s_addr) {
			LOG_PTZ_STRING("Normal");

			ptz->error_code = 0x00;
			gtk_widget_queue_draw (ptz->error_drawing_area);
			gtk_widget_set_tooltip_text (ptz->error_drawing_area, NULL);

			if (ptz->error_drawing_area_tooltip != NULL ) {
				g_free (ptz->error_drawing_area_tooltip);
				ptz->error_drawing_area_tooltip = NULL;
			}

			for (slist_itr2 = ptz->other_ptz_slist; slist_itr2 != NULL; slist_itr2 = slist_itr2->next) {
				other_ptz = slist_itr2->data;

				other_ptz->error_code = 0x00;
				gtk_widget_queue_draw (other_ptz->error_drawing_area);
				gtk_widget_set_tooltip_text (other_ptz->error_drawing_area, NULL);

				if (other_ptz->error_drawing_area_tooltip != NULL ) {
					g_free (other_ptz->error_drawing_area_tooltip);
					other_ptz->error_drawing_area_tooltip = NULL;
				}
			}

			break;
		}
	}

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

PTZ_ERROR(Network_communication_Error,0x24,"Network communication Error")
PTZ_ERROR(CAMERA_communication_Error,0x25,"CAMERA communication Error")
PTZ_ERROR(CAMERA_RX_Over_run_Error,0x26,"CAMERA RX Over run Error")
PTZ_ERROR(CAMERA_RX_Framing_Error,0x27,"CAMERA RX Framing Error")
PTZ_ERROR(CAMERA_RX_Command_Buffer_Overflow,0x28,"CAMERA RX Command Buffer Overflow")

PTZ_ERROR(FPGA_Config_Error,0x23,"FPGA Config Error")
PTZ_ERROR(NET_Life_monitoring_Error,0x24,"NET Life-monitoring Error")
PTZ_ERROR(BE_Life_monitoring_Error,0x25,"BE Life-monitoring Error")
PTZ_ERROR(IF_BE_UART_Buffer_Overflow,0x26,"IF/BE UART Buffer Overflow")
PTZ_ERROR(IF_BE_UART_Framing_Error,0x27,"IF/BE UART Framing Error")
PTZ_ERROR(IF_BE_UART_Buffer_Overflow_2,0x28,"IF/BE UART Buffer Overflow")

PTZ_ERROR(CAM_Life_monitoring_Error,0x29,"CAM Life-monitoring Error")
PTZ_ERROR(NET_Life_monitoring_Error_2,0x30,"NET Life-monitoring Error")
PTZ_ERROR(Fan1_error,0x31,"Fan1 error")
PTZ_ERROR(Fan2_error,0x32,"Fan2 error")
PTZ_ERROR(High_Temp,0x33,"High Temp")
PTZ_ERROR(Low_Temp,0x36,"Low Temp")
PTZ_ERROR(Wiper_Error,0x39,"Wiper Error")
PTZ_ERROR(Temp_Sensor_Error,0x40,"Temp Sensor Error")
PTZ_ERROR(Lens_Initialize_Error,0x41,"Lens Initialize Error")
PTZ_ERROR(PT_Initialize_Error,0x42,"PT. Initialize Error")
PTZ_ERROR(PoEpp_Software_auth_Timeout,0x43,"PoE++ Software auth. Timeout")
PTZ_ERROR(PoEp_Software_auth_Timeout,0x45,"PoE+ Software auth. Timeout")
PTZ_ERROR(USB_Streaming_Error,0x47,"USB Streaming Error")
PTZ_ERROR(MR_Level_Error,0x50,"MR Level Error")
PTZ_ERROR(MR_Offset_Error,0x52,"MR Offset Error")
PTZ_ERROR(Origin_Offset_Error,0x53,"Origin Offset Error")
PTZ_ERROR(Angle_MR_Sensor_Error,0x54,"Angle MR Sensor Error")
PTZ_ERROR(PT_Gear_Error,0x55,"PT. Gear Error")
PTZ_ERROR(Motor_Disconnect_Error,0x56,"Motor Disconnect Error")
PTZ_ERROR(Gyro_Error,0x57,"Gyro Error")
PTZ_ERROR(PT_Initialize_Error_2,0x58,"PT. Initialize Error")
PTZ_ERROR(Update_Firmware_Error,0x60,"Update Firmware Error")
PTZ_ERROR(Update_Hardware_Error,0x61,"Update Hardware Error")
PTZ_ERROR(Update_Error,0x62,"Update Error")
PTZ_ERROR(Update_Fan_Error,0x63,"Update Fan Error")

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

