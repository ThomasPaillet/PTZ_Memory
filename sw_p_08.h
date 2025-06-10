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

#ifndef __SW_P_08_H
#define __SW_P_08_H


#include "ptz.h"


#define SW_P_08_TCP_PORT 8000


extern struct sockaddr_in sw_p_08_address;

extern gboolean sw_p_08_server_started;

extern char tally_cameras_set;


void show_matrix_window (void);

void tell_cameras_set_is_selected (gint page_num);

void ask_to_connect_pgm_to_ctrl_opv (void);

void ask_to_connect_ptz_to_ctrl_opv (ptz_t *ptz);

void tell_memory_is_selected (int index);

void init_sw_p_08 (void);

void start_sw_p_08 (void);

void stop_sw_p_08 (void);


#endif

