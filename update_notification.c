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

#include <unistd.h>


struct sockaddr_in update_notification_address;
SOCKET update_notification_socket;

gboolean update_notification_started = FALSE;

GThread *update_notification_thread;


gpointer receive_update_notification (gpointer data)
{
#ifdef _WIN32
	int addrlen;
#elif defined (__linux)
	socklen_t addrlen;
#endif
	char buffer[556];
	struct sockaddr_in src_addr;
	struct in_addr *src_in_addr;
	SOCKET src_socket;
	cameras_set_t *cameras_set_itr;
	int i;
	ptz_t *ptz, *other_ptz;
	int response;

	while (update_notification_started) {
		addrlen = sizeof (struct sockaddr_in);
		src_socket = accept (update_notification_socket, (struct sockaddr *)&src_addr, &addrlen);

		if (src_socket == INVALID_SOCKET) break;

		recv (src_socket, buffer, 556, 0);
		closesocket (src_socket);

		if (buffer[30] == 'p') {
			if (buffer[31] == '0') {		//P0	//Power Standby
				g_mutex_lock (&cameras_sets_mutex);

				for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
					for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
						if (cameras_set_itr->ptz_ptr_array[i]->address.sin_addr.s_addr == src_addr.sin_addr.s_addr) {
							g_idle_add ((GSourceFunc)ptz_is_off, cameras_set_itr->ptz_ptr_array[i]);

							break;
						}
					}
				}

				g_mutex_unlock (&cameras_sets_mutex);
			} else if (buffer[31] == '1') {		//P1	//Power On
				ptz = NULL;

				g_mutex_lock (&cameras_sets_mutex);

				for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
					for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
						if (cameras_set_itr->ptz_ptr_array[i]->address.sin_addr.s_addr == src_addr.sin_addr.s_addr) {
							if (ptz == NULL) {
								ptz = cameras_set_itr->ptz_ptr_array[i];

								send_cam_request_command (ptz, "QAF", &response);
								if (response == 1) ptz->auto_focus = TRUE;
								else ptz->auto_focus = FALSE;

								g_mutex_lock (&ptz->lens_information_mutex);

								send_ptz_request_command_string (ptz, "#LPI", buffer);
								if (buffer[3] <= '9') ptz->zoom_position = (buffer[3] - '0') * 256;
								else ptz->zoom_position = (buffer[3] - '7') * 256;
								if (buffer[4] <= '9') ptz->zoom_position += (buffer[4] - '0') * 16;
								else ptz->zoom_position += (buffer[4] - '7') * 16;
								if (buffer[5] <= '9') ptz->zoom_position += buffer[5] - '0';
								else ptz->zoom_position += buffer[5] - '7';

								ptz->zoom_position_cmd[4] = buffer[3];
								ptz->zoom_position_cmd[5] = buffer[4];
								ptz->zoom_position_cmd[6] = buffer[5];

								if (buffer[6] <= '9') ptz->focus_position = (buffer[6] - '0') * 256;
								else ptz->focus_position = (buffer[6] - '7') * 256;
								if (buffer[7] <= '9') ptz->focus_position += (buffer[7] - '0') * 16;
								else ptz->focus_position += (buffer[7] - '7') * 16;
								if (buffer[8] <= '9') ptz->focus_position += buffer[8] - '0';
								else ptz->focus_position += buffer[8] - '7';

								ptz->focus_position_cmd[4] = buffer[6];
								ptz->focus_position_cmd[5] = buffer[7];
								ptz->focus_position_cmd[6] = buffer[8];

								send_ptz_control_command (ptz, "#LPC1", TRUE);

								g_idle_add ((GSourceFunc)ptz_is_on, ptz);
							} else {
								other_ptz = cameras_set_itr->ptz_ptr_array[i];

								other_ptz->auto_focus = ptz->auto_focus;

								g_mutex_lock (&other_ptz->lens_information_mutex);

								other_ptz->zoom_position = ptz->zoom_position;
								other_ptz->zoom_position_cmd[4] = buffer[3];
								other_ptz->zoom_position_cmd[5] = buffer[4];
								other_ptz->zoom_position_cmd[6] = buffer[5];

								other_ptz->focus_position = ptz->focus_position;
								other_ptz->focus_position_cmd[4] = buffer[6];
								other_ptz->focus_position_cmd[5] = buffer[7];
								other_ptz->focus_position_cmd[6] = buffer[8];

								g_mutex_unlock (&other_ptz->lens_information_mutex);

								g_idle_add ((GSourceFunc)ptz_is_on, other_ptz);
							}

							break;
						}
					}
				}

				if (ptz != NULL) g_mutex_unlock (&ptz->lens_information_mutex);

				g_mutex_unlock (&cameras_sets_mutex);
			}
		} else if ((buffer[30] == 'l') && (buffer[31] == 'P') && (buffer[32] == 'I')) { 	//lPI	//Lens information
			ptz = NULL;

			g_mutex_lock (&cameras_sets_mutex);

			for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
				for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
					if (cameras_set_itr->ptz_ptr_array[i]->address.sin_addr.s_addr == src_addr.sin_addr.s_addr) {
						if (ptz == NULL) {
							ptz = cameras_set_itr->ptz_ptr_array[i];

							g_mutex_lock (&ptz->lens_information_mutex);

							if ((ptz->zoom_position_cmd[4] != buffer[33]) || (ptz->zoom_position_cmd[5] != buffer[34]) || (ptz->zoom_position_cmd[6] != buffer[35])) {
								if (buffer[33] <= '9') ptz->zoom_position = (buffer[33] - '0') * 256;
								else ptz->zoom_position = (buffer[33] - '7') * 256;
								if (buffer[34] <= '9') ptz->zoom_position += (buffer[34] - '0') * 16;
								else ptz->zoom_position += (buffer[34] - '7') * 16;
								if (buffer[35] <= '9') ptz->zoom_position += buffer[35] - '0';
								else ptz->zoom_position += buffer[35] - '7';

								ptz->zoom_position_cmd[4] = buffer[33];
								ptz->zoom_position_cmd[5] = buffer[34];
								ptz->zoom_position_cmd[6] = buffer[35];

								if (ptz->control_window.is_on_screen) gtk_widget_queue_draw (ptz->control_window.zoom_level_bar_drawing_area);
							}

							if ((ptz->focus_position_cmd[4] != buffer[36]) || (ptz->focus_position_cmd[5] != buffer[37]) || (ptz->focus_position_cmd[6] != buffer[38])) {
								if (buffer[36] <= '9') ptz->focus_position = (buffer[36] - '0') * 256;
								else ptz->focus_position = (buffer[36] - '7') * 256;
								if (buffer[37] <= '9') ptz->focus_position += (buffer[37] - '0') * 16;
								else ptz->focus_position += (buffer[37] - '7') * 16;
								if (buffer[38] <= '9') ptz->focus_position += buffer[38] - '0';
								else ptz->focus_position += buffer[38] - '7';

								ptz->focus_position_cmd[4] = buffer[36];
								ptz->focus_position_cmd[5] = buffer[37];
								ptz->focus_position_cmd[6] = buffer[38];

								if (ptz->control_window.is_on_screen) gtk_widget_queue_draw (ptz->control_window.focus_level_bar_drawing_area);
							}
						} else {
							other_ptz = cameras_set_itr->ptz_ptr_array[i];

							g_mutex_lock (&other_ptz->lens_information_mutex);

							if (other_ptz->zoom_position != ptz->zoom_position) {
								other_ptz->zoom_position = ptz->zoom_position;
								other_ptz->zoom_position_cmd[4] = buffer[33];
								other_ptz->zoom_position_cmd[5] = buffer[34];
								other_ptz->zoom_position_cmd[6] = buffer[35];

								if (other_ptz->control_window.is_on_screen) gtk_widget_queue_draw (other_ptz->control_window.zoom_level_bar_drawing_area);
							}

							if (other_ptz->focus_position != ptz->focus_position) {
								other_ptz->focus_position = ptz->focus_position;
								other_ptz->focus_position_cmd[4] = buffer[36];
								other_ptz->focus_position_cmd[5] = buffer[37];
								other_ptz->focus_position_cmd[6] = buffer[38];

								if (other_ptz->control_window.is_on_screen) gtk_widget_queue_draw (other_ptz->control_window.focus_level_bar_drawing_area);
							}

							g_mutex_unlock (&other_ptz->lens_information_mutex);
						}

						break;
					}
				}
			}

			if (ptz != NULL) g_mutex_unlock (&ptz->lens_information_mutex);

			g_mutex_unlock (&cameras_sets_mutex);
		} else if ((buffer[30] == 'O') && (buffer[31] == 'A') && (buffer[32] == 'F')) { 	//OAF:	//Auto focus_position
			g_mutex_lock (&cameras_sets_mutex);

			for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
				for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
					ptz = cameras_set_itr->ptz_ptr_array[i];

					if (ptz->address.sin_addr.s_addr == src_addr.sin_addr.s_addr) {
						if (buffer[34] == '1') {
							ptz->auto_focus = TRUE;
							if (ptz->control_window.is_on_screen) g_idle_add ((GSourceFunc)update_auto_focus_toggle_button, ptz);
						} else {
							ptz->auto_focus = FALSE;
							if (ptz->control_window.is_on_screen) g_idle_add ((GSourceFunc)update_auto_focus_toggle_button, ptz);
						}

						break;
					}
				}
			}

			g_mutex_unlock (&cameras_sets_mutex);
		} else if ((buffer[30] == 'r') && (buffer[31] == 'E') && (buffer[32] == 'R')) {		//Error information
			src_in_addr = g_malloc (sizeof (struct in_addr));
			*src_in_addr = src_addr.sin_addr;

			if (buffer[33] == '0') {
				if (buffer[34] == '0') g_idle_add ((GSourceFunc)clear_ptz_error, src_in_addr); 	//Normal
				else if ((buffer[34] == '1') || (buffer[34] == '2')) g_idle_add ((GSourceFunc)clear_ptz_error, src_in_addr);
				else if (buffer[34] == '3') g_idle_add ((GSourceFunc)Motor_Driver_Error, src_in_addr);
				else if (buffer[34] == '4') g_idle_add ((GSourceFunc)Pan_Sensor_Error, src_in_addr);
				else if (buffer[34] == '5') g_idle_add ((GSourceFunc)Tilt_Sensor_Error, src_in_addr);
				else if (buffer[34] == '6') g_idle_add ((GSourceFunc)Controller_RX_Over_run_Error, src_in_addr);
				else if (buffer[34] == '7') g_idle_add ((GSourceFunc)Controller_RX_Framing_Error, src_in_addr);
				else if (buffer[34] == '8') g_idle_add ((GSourceFunc)Network_RX_Over_run_Error, src_in_addr);
				else if (buffer[34] == '9') g_idle_add ((GSourceFunc)Network_RX_Framing_Error, src_in_addr);
				else if ((buffer[34] == 'A') || (buffer[34] == 'B')) g_idle_add ((GSourceFunc)clear_ptz_error, src_in_addr);
				else g_free (src_in_addr);
			} else if (buffer[33] == '1') {
				if (buffer[34] == '7') g_idle_add ((GSourceFunc)Controller_RX_Command_Buffer_Overflow, src_in_addr);
				else if (buffer[34] == '9') g_idle_add ((GSourceFunc)Network_RX_Command_Buffer_Overflow, src_in_addr);
				else g_free (src_in_addr);
			} else if (buffer[33] == '2') {
				if (buffer[34] == '1') g_idle_add ((GSourceFunc)System_Error, src_in_addr);
				else if (buffer[34] == '2') g_idle_add ((GSourceFunc)Spec_Limit_Over, src_in_addr);
				else if (buffer[34] == '3') g_idle_add ((GSourceFunc)FPGA_Config_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '4') {
					if (ptz->model == AW_UE150) g_idle_add ((GSourceFunc)NET_Life_monitoring_Error_AW_UE150, src_in_addr);
					else g_idle_add ((GSourceFunc)Network_communication_Error_AW_HE130, src_in_addr);
				} else if (buffer[34] == '5') {
					if (ptz->model == AW_UE150) g_idle_add ((GSourceFunc)BE_Life_monitoring_Error_AW_UE150, src_in_addr);
					else g_idle_add ((GSourceFunc)CAMERA_communication_Error_AW_HE130, src_in_addr);
				} else if (buffer[34] == '6') {
					if (ptz->model == AW_UE150) g_idle_add ((GSourceFunc)IF_BE_UART_Buffer_Overflow_AW_UE150, src_in_addr);
					else g_idle_add ((GSourceFunc)CAMERA_RX_Over_run_Error_AW_HE130, src_in_addr);
				} else if (buffer[34] == '7') {
					if (ptz->model == AW_UE150) g_idle_add ((GSourceFunc)IF_BE_UART_Framing_Error_AW_UE150, src_in_addr);
					else g_idle_add ((GSourceFunc)CAMERA_RX_Framing_Error_AW_HE130, src_in_addr);
				} else if (buffer[34] == '8') {
					if (ptz->model == AW_UE150) g_idle_add ((GSourceFunc)IF_BE_UART_Buffer_Overflow_2_AW_UE150, src_in_addr);
					else g_idle_add ((GSourceFunc)CAMERA_RX_Command_Buffer_Overflow_AW_HE130, src_in_addr);
				} else if (buffer[34] == '9') g_idle_add ((GSourceFunc)CAM_Life_monitoring_Error_AW_UE150, src_in_addr);
				else g_free (src_in_addr);
			} else if (buffer[33] == '3') {
				if (buffer[34] == '1') g_idle_add ((GSourceFunc)Fan1_error_AW_UE150, src_in_addr);
				else if (buffer[34] == '2') g_idle_add ((GSourceFunc)Fan2_error_AW_UE150, src_in_addr);
				else if (buffer[34] == '3') g_idle_add ((GSourceFunc)High_Temp_AW_UE150, src_in_addr);
				else if (buffer[34] == '6') g_idle_add ((GSourceFunc)Low_Temp_AW_UE150, src_in_addr);
				else g_free (src_in_addr);
			} else if (buffer[33] == '4') {
				if (buffer[34] == '0') g_idle_add ((GSourceFunc)Temp_Sensor_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '1') g_idle_add ((GSourceFunc)Lens_Initialize_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '2') g_idle_add ((GSourceFunc)PT_Initialize_Error_AW_UE150, src_in_addr);
				else g_free (src_in_addr);
			} else if (buffer[33] == '5') {
				if (buffer[34] == '0') g_idle_add ((GSourceFunc)MR_Level_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '2') g_idle_add ((GSourceFunc)MR_Offset_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '3') g_idle_add ((GSourceFunc)Origin_Offset_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '4') g_idle_add ((GSourceFunc)Angle_MR_Sensor_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '5') g_idle_add ((GSourceFunc)PT_Gear_Error_AW_UE150, src_in_addr);
				else if (buffer[34] == '6') g_idle_add ((GSourceFunc)Motor_Disconnect_Error_AW_UE150, src_in_addr);
				else g_free (src_in_addr);
			} else g_free (src_in_addr);
		} else if ((buffer[30] == 'E') && (buffer[31] == 'R')) {
			src_in_addr = g_malloc (sizeof (struct in_addr));
			*src_in_addr = src_addr.sin_addr;

			if ((buffer[32] == '3') && (buffer[33] == ':') && (buffer[34] == 'O') && (buffer[35] == 'A') && (buffer[36] == 'S')) {	//ABB execution failed
				g_idle_add ((GSourceFunc)ABB_execution_failed, src_in_addr);
			} else if ((buffer[32] == '2') && (buffer[33] == ':') && (buffer[34] == 'O') && (buffer[35] == 'A') && (buffer[36] == 'S')) {	//ABB execution failed (busy status)
				g_idle_add ((GSourceFunc)ABB_execution_failed_busy_status, src_in_addr);
			} else g_free (src_in_addr);
		}
	}

	return NULL;
}

void init_update_notification (void)
{
	memset (&update_notification_address, 0, sizeof (struct sockaddr_in));
	update_notification_address.sin_family = AF_INET;
	update_notification_address.sin_port = htons (UPDATE_NOTIFICATION_TCP_PORT);
	update_notification_address.sin_addr.s_addr = inet_addr (my_ip_address);
}

void start_update_notification (void)
{
	update_notification_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (bind (update_notification_socket, (struct sockaddr *)&update_notification_address, sizeof (struct sockaddr_in)) == 0) {
		if (listen (update_notification_socket, MAX_CAMERAS * MAX_CAMERAS_SET) == 0) {
			update_notification_started = TRUE;
			update_notification_thread = g_thread_new (NULL, receive_update_notification, NULL);
		}
	}
}

void stop_update_notification (void)
{
	cameras_set_t *cameras_set_itr;
	int i;

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			if (cameras_set_itr->ptz_ptr_array[i]->ip_address_is_valid && (cameras_set_itr->ptz_ptr_array[i]->error_code != 0x30)) {
//				send_ptz_control_command (cameras_set_itr->ptz_ptr_array[i], "#LPC0", TRUE);
				send_update_stop_cmd (cameras_set_itr->ptz_ptr_array[i]);
			}
		}
	}

	update_notification_started = FALSE;

	shutdown (update_notification_socket, SHUT_RD);
	closesocket (update_notification_socket);

	if (update_notification_thread != NULL) {
		g_thread_join (update_notification_thread);
		update_notification_thread = NULL;
	}
}

