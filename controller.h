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

#ifndef __CONTROLLER_H
#define __CONTROLLER_H


#include "ptz.h"


extern gboolean controller_is_used;

extern char controller_ip_address[16];
extern gboolean controller_ip_address_is_valid;

extern struct sockaddr_in controller_address;


gpointer controller_switch_ptz (ptz_thread_t *ptz_thread);

void init_controller (void);


#endif

