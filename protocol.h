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

#ifndef __PTZ_PROTOCOL_H
#define __PTZ_PROTOCOL_H


#include "ptz.h"


extern char my_ip_address[16];
extern char network_address[3][4];
extern int network_address_len[3];


void init_protocol (void);

void init_ptz_cmd (ptz_t *ptz);

void send_update_start_cmd (ptz_t *ptz);

void send_update_stop_cmd (ptz_t *ptz);

void send_ptz_request_command (ptz_t *ptz, char* cmd, int *response);

void send_ptz_request_command_string (ptz_t *ptz, char* cmd, char *response);

void send_ptz_control_command (ptz_t *ptz, char* cmd, gboolean wait);

void send_cam_request_command (ptz_t *ptz, char* cmd, int *response);

void send_cam_request_command_string (ptz_t *ptz, char* cmd, char *response);

void send_cam_control_command (ptz_t *ptz, char* cmd);

void send_thumbnail_320_request_cmd (memory_t *memory);

void send_thumbnail_640_request_cmd (memory_t *memory);

void send_jpeg_image_request_cmd (ptz_t *ptz);


#endif

