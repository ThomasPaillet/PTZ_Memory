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

#ifndef __LOGGING_H
#define __LOGGING_H


#include "ptz.h"

#include <stdio.h>


#define LOG_PANASONIC_INT(i) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, panasonic_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_INT(i) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, sw_p_08_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_TSL_UMD_V5_INT(i) if (logging && log_tsl_umd_v5) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, tsl_umd_v5_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_ULTIMATTE_INT(i) if (logging && log_ultimatte) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, ultimatte_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_INT(i) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_int (__FILE__, i, free_d_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PANASONIC_STRING(s) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, panasonic_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_STRING(s) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, sw_p_08_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_TSL_UMD_V5_STRING(s) if (logging && log_tsl_umd_v5) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, tsl_umd_v5_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_ULTIMATTE_STRING(s) if (logging && log_ultimatte) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, ultimatte_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_STRING(s) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_string (__FILE__, s, free_d_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PANASONIC_2_STRINGS(s,t) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, panasonic_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_2_STRINGS(s,t) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, sw_p_08_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_TSL_UMD_V5_2_STRINGS(s,t) if (logging && log_tsl_umd_v5) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, tsl_umd_v5_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_ULTIMATTE_2_STRINGS(s,t) if (logging && log_ultimatte) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, ultimatte_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_2_STRINGS(s,t) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_2_strings (__FILE__, s, t, free_d_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PANASONIC_STRING_INT(s,i) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, panasonic_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_STRING_INT(s,i) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, sw_p_08_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_TSL_UMD_V5_STRING_INT(s,i) if (logging && log_tsl_umd_v5) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, tsl_umd_v5_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_ULTIMATTE_STRING_INT(s,i) if (logging && log_ultimatte) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, ultimatte_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_STRING_INT(s,i) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_string_int (__FILE__, s, i, free_d_log_file); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PTZ_STRING(s) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_ptz_string (__FILE__, ptz, s); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PTZ_COMMAND(c) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_ptz_command (__FILE__, ptz, c); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PTZ_RESPONSE(r,l) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_ptz_response (__FILE__, ptz, r, l); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_PTZ_ERROR(s) if (logging && log_panasonic) { \
	g_mutex_lock (&logging_mutex); \
	log_ptz_error (__FILE__, ptz, s); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_OUTGOING_MESSAGE(a,m,s) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_sw_p_08_outgoing_message (__FILE__, a, m, s); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_SW_P_08_INCOMMING_MESSAGE(a,m,s) if (logging && log_sw_p_08) { \
	g_mutex_lock (&logging_mutex); \
	log_sw_p_08_incomming_message (__FILE__, a, m, s); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_TSL_UMD_V5_PACKET(p) if (logging && log_tsl_umd_v5) { \
	g_mutex_lock (&logging_mutex); \
	log_tsl_umd_v5_packet (p); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_OUTGOING_MESSAGE(a,m,s) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_free_d_outgoing_message(__FILE__, a, m, s); \
	g_mutex_unlock (&logging_mutex); \
	}

#define LOG_FREE_D_INCOMMING_MESSAGE(a,m,s) if (logging && log_free_d) { \
	g_mutex_lock (&logging_mutex); \
	log_free_d_incomming_message(__FILE__, a, m, s); \
	g_mutex_unlock (&logging_mutex); \
	}


extern gboolean logging;

extern gboolean log_panasonic;
extern gboolean log_sw_p_08;
extern gboolean log_tsl_umd_v5;
extern gboolean log_ultimatte;
extern gboolean log_free_d;

extern gboolean fsync_log;

extern GMutex logging_mutex;

extern FILE *panasonic_log_file;
extern FILE *sw_p_08_log_file;
extern FILE *tsl_umd_v5_log_file;
extern FILE *ultimatte_log_file;
extern FILE *free_d_log_file;


void log_int (const char *c_source_filename, int i, FILE *log_file);

void log_string (const char *c_source_filename, const char *str, FILE *log_file);

void log_2_strings (const char *c_source_filename, const char *str1, const char *str2, FILE *log_file);

void log_string_int (const char *c_source_filename, const char *str, int i, FILE *log_file);

void log_ptz_string (const char *c_source_filename, ptz_t *ptz, const char *str);

void log_ptz_command (const char *c_source_filename, ptz_t *ptz, const char *cmd);

void log_ptz_response (const char *c_source_filename, ptz_t *ptz, const char *response, int len);

void log_ptz_update_notification (const char *c_source_filename, const char *ip_address, const char *update_notification);

void log_ptz_error (const char *c_source_filename, ptz_t *ptz, const char *str);

void log_controller_command (const char *c_source_filename, const char *ip_address, const char *cmd, int len);

void log_controller_response (const char *c_source_filename, const char *ip_address, const char *response, int len);

void log_sw_p_08_outgoing_message (const char *c_source_filename, const char *ip_address, const char *sw_p_08_message, int len);

void log_sw_p_08_incomming_message (const char *c_source_filename, const char *ip_address, const char *sw_p_08_message, int len);

void log_tsl_umd_v5_packet (const char *packet);

void log_ultimatte_command (const char *c_source_filename, const char *ip_address, const char *ultimatte_command, int len);

void log_ultimatte_message (const char *ultimatte_message, int len);

void log_free_d_outgoing_message (const char *c_source_filename, const char *ip_address, const unsigned char *free_d_message, int len);

void log_free_d_incomming_message (const char *c_source_filename, const char *ip_address, const unsigned char *free_d_message, int len);

void init_logging (void);

void start_logging (void);

void start_panasonic_log (void);

void start_sw_p_08_log (void);

void start_tsl_umd_v5_log (void);

void start_ultimatte_log (void);

void start_free_d_log (void);

void stop_logging (void);

void stop_panasonic_log (void);

void stop_sw_p_08_log (void);

void stop_tsl_umd_v5_log (void);

void stop_ultimatte_log (void);

void stop_free_d_log (void);


#endif

