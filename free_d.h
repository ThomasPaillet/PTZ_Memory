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

#ifndef __FREE_D_H
#define __FREE_D_H


#include <gtk/gtk.h>
#include "network_header.h"


#define FREE_D_UDP_PORT 2000


extern char free_d_output_ip_address[16];

extern gboolean free_d_output_ip_address_is_valid;

extern struct sockaddr_in free_d_output_address;

extern struct sockaddr_in free_d_input_address;

extern gboolean outgoing_free_d_started;


void init_free_d (void);

void start_outgoing_freed_d (void);

void stop_outgoing_freed_d (void);

void start_incomming_free_d (void);

void stop_incomming_free_d (void);


#endif

