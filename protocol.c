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

#include "protocol.h"

#include "error.h"
#include "update_notification.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>	//gethostname usleep
#if defined (__linux)
#include <sys/ioctl.h>
#include <net/if.h>
#endif
#include <jpeglib.h>


char my_ip_address[16];
char network_address[3][4] = { { '\0' }, { '\0' }, { '\0' } };
int network_address_len[3] = { 0, 0, 0 };


char *http_ptz_cmd = "GET /cgi-bin/aw_ptz?cmd=";	//strlen = 24
char *http_cam_cmd = "GET /cgi-bin/aw_cam?cmd=";	//strlen = 24

char *http_update_start_cmd = "GET /cgi-bin/event?connect=start&my_port=";	//strlen = 41
char *http_update_stop_cmd = "GET /cgi-bin/event?connect=stop&my_port=";	//strlen = 40

char *http_thumbnail_320_cmd = "GET /cgi-bin/camera?resolution=320&quality=1&page=";	//strlen = 50
char *http_thumbnail_640_cmd = "GET /cgi-bin/camera?resolution=640&quality=1&page=";	//strlen = 50
char *http_camera_cmd = "GET /cgi-bin/camera?resolution=1920&quality=1&page=";	//strlen = 51

char *http_cam_ptz_header = "&res=1";	//strlen = 6
char *http_update_header = "&uid=0";	//strlen = 6
char *http_header_1 = " HTTP/1.1\r\nAccept: image/gif, ... (omitted) ... , */*\r\nReferer: http://";	//strlen = 71
char *http_header_2 = "/\r\nAccept-Language: en\r\nAccept-Encoding: gzip, deflate\r\nUser-Agent: AW-Cam Controller\r\nHost: ";	//strlen = 93
char *http_header_3 = "\r\nConnection: Keep-Alive\r\n\r\n";	//strlen = 28

char *http_header;
int http_header_size;
int full_http_header_size;


#define WAIT_IF_NEEDED \
	gettimeofday (&current_time, NULL); \
	timersub (&current_time, &ptz->last_time, &elapsed_time); \
	if ((elapsed_time.tv_sec == 0) && (elapsed_time.tv_usec < 130000)) { \
		usleep (130000 - elapsed_time.tv_usec); \
 \
		ptz->last_time.tv_usec += 130000; \
		if (ptz->last_time.tv_usec >= 1000000) { \
			ptz->last_time.tv_sec++; \
			ptz->last_time.tv_usec -= 1000000; \
		} \
	} else ptz->last_time = current_time;

#define COMMAND_FUNCTION_END \
	} else { \
		ptz->error_code = CAMERA_IS_UNREACHABLE_ERROR; \
		ptz->is_on = FALSE; \
		g_idle_add ((GSourceFunc)camera_is_unreachable, ptz); \
	} \
 \
	closesocket (sock); \
 \
g_mutex_unlock (&ptz->cmd_mutex); \
}


void init_protocol (void)
{
	int i, j;

#ifdef _WIN32
	char host_name[256];
	char **pp;
	struct hostent *host;

	gethostname (host_name, 256);
	host = gethostbyname (host_name);

	for (pp = host->h_addr_list; *pp != NULL; pp++)
		strcpy (my_ip_address, inet_ntoa (*(struct in_addr *)*pp));
#elif defined (__linux)
	SOCKET sock;
	struct ifconf ip_interfaces;

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ip_interfaces.ifc_len = 0;
	ip_interfaces.ifc_req = NULL;
	ioctl (sock, SIOCGIFCONF, &ip_interfaces);
	ip_interfaces.ifc_req = g_malloc (ip_interfaces.ifc_len);
	ioctl (sock, SIOCGIFCONF, &ip_interfaces);

	for (i = 0; i < ip_interfaces.ifc_len / sizeof (struct ifreq); i++)
		strcpy (my_ip_address, inet_ntoa (((struct sockaddr_in *)&ip_interfaces.ifc_req[i].ifr_addr)->sin_addr));
#endif

	i = 0;
	do {
		network_address[0][i] = my_ip_address[i];
		i++;
	} while (my_ip_address[i] != '.');
	network_address[0][i] = '\0';
	network_address_len[0] = i;
	i++;

	j = 0;
	do {
		network_address[1][j++] = my_ip_address[i++];
	} while (my_ip_address[i] != '.');
	network_address[1][j] = '\0';
	network_address_len[1] = j;
	i++;

	j = 0;
	do {
		network_address[2][j++] = my_ip_address[i++];
	} while (my_ip_address[i] != '.');
	network_address[2][j] = '\0';
	network_address_len[2] = j;

	http_header = g_malloc (152);
	http_header_size = sprintf (http_header, "%s%s%s%s", my_ip_address, http_header_2, my_ip_address, http_header_3);
	full_http_header_size = http_header_size + 101;
}

void init_ptz_cmd (ptz_t *ptz)
{
	memset (&ptz->address, 0, sizeof (struct sockaddr_in));
	ptz->address.sin_family = AF_INET;
	ptz->address.sin_port = htons (80);

	ptz->jpeg_page = 1;
	memcpy (ptz->cmd_buffer + 39, http_cam_ptz_header, 6);
	memcpy (ptz->cmd_buffer + 45, http_header_1, 71);
	memcpy (ptz->cmd_buffer + 116, http_header, http_header_size);
	ptz->cmd_buffer[116 + http_header_size] = '\0';

	ptz->last_cmd = ptz->cmd_buffer + 13;
	memcpy (ptz->last_cmd, http_ptz_cmd, 24);
	ptz->cam_ptz = FALSE;

	ptz->last_ctrl_cmd[0] = '\0';
	ptz->last_ctrl_cmd_len = 0;

	ptz->last_time.tv_sec = 0;
	ptz->last_time.tv_usec = 0;
	g_mutex_init (&ptz->cmd_mutex);
}

void send_update_start_cmd (ptz_t *ptz)
{
	int size, index;
	char buffer[280];
	SOCKET sock;
	struct timeval current_time, elapsed_time;

g_mutex_lock (&ptz->cmd_mutex);

	memcpy (buffer, http_update_start_cmd, 41);
	index = 41 + sprintf (buffer + 41, "%hu", ntohs (update_notification_address.sin_port));
	memcpy (buffer + index, http_update_header, 6);
	index += 6;
	memcpy (buffer + index, http_header_1, 71);
	index += 71;
	memcpy (buffer + index, http_header, http_header_size);
	size = index + http_header_size;
	buffer[size] = '\0';

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, buffer, size, 0);

		if (ptz->error_code == CAMERA_IS_UNREACHABLE_ERROR) {
			ptz->error_code = 0x00;
			g_idle_add ((GSourceFunc)clear_ptz_error, ptz);
		}

COMMAND_FUNCTION_END


void send_update_stop_cmd (ptz_t *ptz)
{
	int size, index;
	char buffer[280];
	SOCKET sock;
	struct timeval current_time, elapsed_time;

g_mutex_lock (&ptz->cmd_mutex);

	memcpy (buffer, http_update_stop_cmd, 40);
	index = 40 + sprintf (buffer + 40, "%hu", ntohs (update_notification_address.sin_port));
	memcpy (buffer + index, http_update_header, 6);
	index += 6;
	memcpy (buffer + index, http_header_1, 71);
	index += 71;
	memcpy (buffer + index, http_header, http_header_size);
	size = index + http_header_size;
	buffer[size] = '\0';

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, buffer, size, 0);

COMMAND_FUNCTION_END


void send_ptz_request_command (ptz_t *ptz, char* cmd, int *response)
{
	int size, index;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	char buffer[264];

g_mutex_lock (&ptz->cmd_mutex);

	size = 2;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == TRUE)) {
		memcpy (http_cmd, http_ptz_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = FALSE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

		index = 0;
		size = recv (sock, buffer, sizeof (buffer), 0);

		while (size > 0) {
			index += size;
			size = recv (sock, buffer + index, sizeof (buffer) - index, 0);
		}
		buffer[index] = '\0';

		*response = buffer[--index] - 48;
	} else {
		*response = 0;

		ptz->error_code = CAMERA_IS_UNREACHABLE_ERROR;
		ptz->is_on = FALSE;
		g_idle_add ((GSourceFunc)camera_is_unreachable, ptz);
	}

	closesocket (sock);

g_mutex_unlock (&ptz->cmd_mutex);
}

void send_ptz_request_command_string (ptz_t *ptz, char* cmd, char *response)
{
	int size, index;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	char buffer[264];

g_mutex_lock (&ptz->cmd_mutex);

	size = 3;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == TRUE)) {
		memcpy (http_cmd, http_ptz_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = FALSE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

		index = 0;
		size = recv (sock, buffer, sizeof (buffer), 0);

		while (size > 0) {
			index += size;
			size = recv (sock, buffer + index, sizeof (buffer) - index, 0);
		}
		buffer[index] = '\0';

		index--;
		while (buffer[index] != '\n') index--;
		index++;

		strcpy (response, buffer + index);

COMMAND_FUNCTION_END


void send_ptz_control_command (ptz_t *ptz, char* cmd, gboolean wait)
{
	int size;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;

g_mutex_lock (&ptz->cmd_mutex);

	size = 3;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->last_ctrl_cmd, cmd, size);
	ptz->last_ctrl_cmd[size] = '\0';
	ptz->last_ctrl_cmd_len = size;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == TRUE)) {
		memcpy (http_cmd, http_ptz_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = FALSE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (wait) {
WAIT_IF_NEEDED
	}

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

COMMAND_FUNCTION_END


void send_cam_request_command (ptz_t *ptz, char* cmd, int *response)
{
	int size, index;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	char buffer[264];

g_mutex_lock (&ptz->cmd_mutex);

	size = 3;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == FALSE)) {
		memcpy (http_cmd, http_cam_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = TRUE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

		index = 0;
		size = recv (sock, buffer, sizeof (buffer), 0);

		while (size > 0) {
			index += size;
			size = recv (sock, buffer + index, sizeof (buffer) - index, 0);
		}
		buffer[index] = '\0';

		index--;
		while ((buffer[index] != ':') && (buffer[index] != '\n')) index--;
		index++;

		sscanf (buffer + index, "%x", response);

COMMAND_FUNCTION_END


void send_cam_request_command_string (ptz_t *ptz, char* cmd, char *response)
{
	int size, index;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	char buffer[264];

g_mutex_lock (&ptz->cmd_mutex);

	size = 3;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == FALSE)) {
		memcpy (http_cmd, http_cam_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = TRUE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

		index = 0;
		size = recv (sock, buffer, sizeof (buffer), 0);

		while (size > 0) {
			index += size;
			size = recv (sock, buffer + index, sizeof (buffer) - index, 0);
		}
		buffer[index] = '\0';

		index--;
		while ((buffer[index] != ':') && (buffer[index] != '\n')) index--;
		index++;

		strcpy (response, buffer + index);

COMMAND_FUNCTION_END


void send_cam_control_command (ptz_t *ptz, char* cmd)
{
	int size;
	char *http_cmd;
	SOCKET sock;
	struct timeval current_time, elapsed_time;

g_mutex_lock (&ptz->cmd_mutex);

	size = 5;
	while (cmd[size] != '\0') size++;

	memcpy (ptz->last_ctrl_cmd, cmd, size);
	ptz->last_ctrl_cmd[size]= '\0';
	ptz->last_ctrl_cmd_len = size;

	memcpy (ptz->cmd_buffer + 39 - size, cmd, size);
	http_cmd = ptz->cmd_buffer + 15 - size;
	if ((http_cmd != ptz->last_cmd) || (ptz->cam_ptz == FALSE)) {
		memcpy (http_cmd, http_cam_cmd, 24);
		ptz->last_cmd = http_cmd;
		ptz->cam_ptz = TRUE;
	}

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, http_cmd, size + full_http_header_size, 0);

COMMAND_FUNCTION_END


void send_thumbnail_320_request_cmd (memory_t *memory)
{
	ptz_t *ptz = memory->ptz_ptr;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	int size, index, i;
	char buffer[20480];
	int rowstride;
	guchar *pixels;
	JSAMPROW scanlines[180];
	struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;

g_mutex_lock (&ptz->cmd_mutex);

	memcpy (buffer, http_thumbnail_320_cmd, 50);
	size = sprintf (buffer + 50, "%d", ptz->jpeg_page);
	ptz->jpeg_page++;
	memcpy (buffer + 50 + size, http_header_1, 71);
	memcpy (buffer + 121 + size, http_header, http_header_size);
	buffer[121 + size + http_header_size] = '\0';

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, buffer, 122 + size + http_header_size, 0);

		if (ptz->model == AW_HE130) recv (sock, buffer, sizeof (buffer), 0);
		size = recv (sock, buffer, sizeof (buffer), 0);
		size += recv (sock, buffer + size, sizeof (buffer) - size, 0);

		index = 0;
		do {
			while (buffer[index] != '\n') index++;
			index++;
		} while (buffer[index] != '\r');
		index++;
		index++;

		do {
			i = recv (sock, buffer + size, sizeof (buffer) - size, 0);
			size += i;
		} while (i > 0);

		if (memory->empty) memory->full_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 320, 180);

		rowstride = gdk_pixbuf_get_rowstride (memory->full_pixbuf);
		pixels = gdk_pixbuf_get_pixels (memory->full_pixbuf);

		for (i = 0; i < 180; i++) scanlines[i] = pixels + i * rowstride;

		cinfo.err = jpeg_std_error (&jerr);
		jpeg_create_decompress (&cinfo);
		jpeg_mem_src (&cinfo, (unsigned char *)buffer + index, size);
		jpeg_read_header (&cinfo, TRUE);
		jpeg_start_decompress (&cinfo);

		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines (&cinfo, scanlines + cinfo.output_scanline, 1);
		}
		jpeg_finish_decompress (&cinfo);
		jpeg_destroy_decompress (&cinfo);

COMMAND_FUNCTION_END


void send_thumbnail_640_request_cmd (memory_t *memory)
{
	ptz_t *ptz = memory->ptz_ptr;
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	int size, index, i;
	char buffer[32768];
	GdkPixbuf *pixbuf;
	int rowstride;
	guchar *pixels;
	JSAMPROW scanlines[360];
	struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;

g_mutex_lock (&ptz->cmd_mutex);

	memcpy (buffer, http_thumbnail_640_cmd, 50);
	size = sprintf (buffer + 50, "%d", ptz->jpeg_page);
	ptz->jpeg_page++;
	memcpy (buffer + 50 + size, http_header_1, 71);
	memcpy (buffer + 121 + size, http_header, http_header_size);
	buffer[121 + size + http_header_size] = '\0';

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, buffer, 122 + size + http_header_size, 0);

		if (ptz->model == AW_HE130) recv (sock, buffer, sizeof (buffer), 0);
		size = recv (sock, buffer, sizeof (buffer), 0);
		size += recv (sock, buffer + size, sizeof (buffer) - size, 0);

		index = 0;
		do {
			while (buffer[index] != '\n') index++;
			index++;
		} while (buffer[index] != '\r');
		index++;
		index++;

		do {
			i = recv (sock, buffer + size, sizeof (buffer) - size, 0);
			size += i;
		} while (i > 0);

		pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 640, 360);
		rowstride = gdk_pixbuf_get_rowstride (pixbuf);
		pixels = gdk_pixbuf_get_pixels (pixbuf);

		for (i = 0; i < 360; i++) scanlines[i] = pixels + i * rowstride;

		cinfo.err = jpeg_std_error (&jerr);
		jpeg_create_decompress (&cinfo);
		jpeg_mem_src (&cinfo, (unsigned char *)buffer + index, size);
		jpeg_read_header (&cinfo, TRUE);
		jpeg_start_decompress (&cinfo);

		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines (&cinfo, scanlines + cinfo.output_scanline, 1);
		}
		jpeg_finish_decompress (&cinfo);
		jpeg_destroy_decompress (&cinfo);

		if (!memory->empty) g_object_unref (G_OBJECT (memory->full_pixbuf));

		memory->full_pixbuf = gdk_pixbuf_scale_simple (pixbuf, 320, 180, GDK_INTERP_BILINEAR);

		g_object_unref (G_OBJECT (pixbuf));

COMMAND_FUNCTION_END


void send_jpeg_image_request_cmd (ptz_t *ptz)
{
	int size, index;
	char buffer[8192];
	SOCKET sock;
	struct timeval current_time, elapsed_time;
	struct tm *time;
	FILE *jpeg_file;
	char jpeg_file_name[40];

g_mutex_lock (&ptz->cmd_mutex);

	memcpy (buffer, http_camera_cmd, 51);
	size = sprintf (buffer + 51, "%d", ptz->jpeg_page);
	ptz->jpeg_page++;
	memcpy (buffer + 51 + size, http_header_1, 71);
	memcpy (buffer + 122 + size, http_header, http_header_size);
	buffer[122 + size + http_header_size] = '\0';

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

WAIT_IF_NEEDED

	if (connect (sock, (struct sockaddr *) &ptz->address, sizeof (struct sockaddr_in)) == 0) {
		send (sock, buffer, 122 + size + http_header_size, 0);

		time = localtime (&current_time.tv_sec);
		sprintf (jpeg_file_name, "20%02d-%02d-%02d %02d_%02d_%02d Camera %s.jpg", time->tm_year - 100, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, ptz->name);
		jpeg_file = fopen (jpeg_file_name, "wb");

		if (ptz->model == AW_HE130) recv (sock, buffer, sizeof (buffer), 0);
		size = recv (sock, buffer, sizeof (buffer), 0);
		size += recv (sock, buffer + size, sizeof (buffer) - size, 0);

		index = 0;
		do {
			while (buffer[index] != '\n') index++;
			index++;
		} while (buffer[index] != '\r');
		index++;
		index++;
		fwrite (buffer + index, 1, size - index, jpeg_file);

		size = recv (sock, buffer, sizeof (buffer), 0);

		while (size > 0) {
			fwrite (buffer, 1, size, jpeg_file);
			size = recv (sock, buffer, sizeof (buffer), 0);
		}
		fclose (jpeg_file);

COMMAND_FUNCTION_END

