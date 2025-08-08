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

#ifndef __ERROR_H
#define __ERROR_H


#include "ptz.h"


#define CAMERA_IS_UNREACHABLE_ERROR 0xFFFF


gboolean camera_is_unreachable (ptz_t *ptz);

gboolean clear_ptz_error (struct in_addr *src_in_addr);

gboolean Motor_Driver_Error (struct in_addr *src_in_addr);
gboolean Pan_Sensor_Error (struct in_addr *src_in_addr);
gboolean Tilt_Sensor_Error (struct in_addr *src_in_addr);
gboolean Controller_RX_Over_run_Error (struct in_addr *src_in_addr);
gboolean Controller_RX_Framing_Error (struct in_addr *src_in_addr);
gboolean Network_RX_Over_run_Error (struct in_addr *src_in_addr);
gboolean Network_RX_Framing_Error (struct in_addr *src_in_addr);
gboolean Controller_RX_Command_Buffer_Overflow (struct in_addr *src_in_addr);
gboolean Network_RX_Command_Buffer_Overflow (struct in_addr *src_in_addr);
gboolean System_Error (struct in_addr *src_in_addr);
gboolean Spec_Limit_Over (struct in_addr *src_in_addr);
gboolean ABB_execution_failed (struct in_addr *src_in_addr);
gboolean ABB_execution_failed_busy_status (struct in_addr *src_in_addr);

gboolean Network_communication_Error_AW_HE130 (struct in_addr *src_in_addr);
gboolean CAMERA_communication_Error_AW_HE130 (struct in_addr *src_in_addr);
gboolean CAMERA_RX_Over_run_Error_AW_HE130 (struct in_addr *src_in_addr);
gboolean CAMERA_RX_Framing_Error_AW_HE130 (struct in_addr *src_in_addr);
gboolean CAMERA_RX_Command_Buffer_Overflow_AW_HE130 (struct in_addr *src_in_addr);

gboolean FPGA_Config_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean NET_Life_monitoring_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean BE_Life_monitoring_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean IF_BE_UART_Buffer_Overflow_AW_UE150 (struct in_addr *src_in_addr);
gboolean IF_BE_UART_Framing_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean IF_BE_UART_Buffer_Overflow_2_AW_UE150 (struct in_addr *src_in_addr);
gboolean CAM_Life_monitoring_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Fan1_error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Fan2_error_AW_UE150 (struct in_addr *src_in_addr);
gboolean High_Temp_AW_UE150 (struct in_addr *src_in_addr);
gboolean Low_Temp_AW_UE150 (struct in_addr *src_in_addr);
gboolean Temp_Sensor_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Lens_Initialize_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean PT_Initialize_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean MR_Level_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean MR_Offset_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Origin_Offset_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Angle_MR_Sensor_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean PT_Gear_Error_AW_UE150 (struct in_addr *src_in_addr);
gboolean Motor_Disconnect_Error_AW_UE150 (struct in_addr *src_in_addr);

gboolean error_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);


#endif

