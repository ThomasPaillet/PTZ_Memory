/*
 * copyright (c) 2020 Thomas Paillet <thomas.paillet@net-c.fr

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
 * along with PTZ-Memory.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __PTZ_H
#define __PTZ_H


#ifdef _WIN32
	#include <winsock2.h>

	void WSAInit (void);

	void timersub (const struct timeval* tvp, const struct timeval* uvp, struct timeval* vvp);

#elif defined (__linux)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	#define SOCKET int
	#define INVALID_SOCKET -1
	#define closesocket close
	#define WSAInit()
	#define WSACleanup()
#endif


#include <gtk/gtk.h>
#include <sys/time.h>


#define MAX_MEMORIES 20


typedef struct {
	gpointer ptz;
	int index;

	gboolean empty;

	GtkWidget *button;
	gulong button_handler_id;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GdkPixbuf *scaled_pixbuf;

	char pan_tilt_position_cmd[13];

	int zoom_position;
	char zoom_position_hexa[3];

	int focus_position;
	char focus_position_hexa[3];
} memory_t;

typedef struct {
	GtkWidget *window;
	gboolean is_on_screen;
	GtkWidget *tally[4];
	GtkWidget *name_drawing_area;

	GtkWidget *auto_focus_toggle_button;
	gulong auto_focus_handler_id;
	GtkWidget *focus_widget_box;
	GdkWindow *gdk_window;
	gint x_root;
	gint y_root;
	int width;
	int height;

	GtkWidget *otaf_button;
	GtkWidget *focus_level_bar_drawing_area;

	GtkWidget *zoom_level_bar_drawing_area;
	GtkWidget *zoom_tele_button;
	GtkWidget *zoom_wide_button;

	gdouble x;
	gdouble y;
	int pan_speed;
	int tilt_speed;

	guint focus_timeout_id;
	guint pan_tilt_timeout_id;
	guint zoom_timeout_id;

	gboolean key_pressed;
} control_window_t;

typedef struct {
	char name[3];
	int index;
	gboolean active;
	char ip_adresse[16];
	char new_ip_adresse[16];
	gboolean ip_adresse_is_valid;

	int matrix_source_number;

	struct sockaddr_in adresse;

	int jpeg_page;
	char cmd_buffer[272];
	char *last_cmd;
	gboolean cam_ptz;
	char last_ctrl_cmd[16];
	int last_ctrl_cmd_len;

	struct timeval last_time;
	GMutex cmd_mutex;

	memory_t memories[MAX_MEMORIES];
	int number_of_memories;

	gboolean auto_focus;

	int zoom_position;
	char zoom_position_cmd[8];

	int focus_position;
	char focus_position_cmd[8];

	control_window_t control_window;

	GtkWidget *name_separator;
	GtkWidget *name_grid;
	GtkWidget *name_drawing_area;
	gboolean enter_notify_name_drawing_area;
	GtkWidget *memories_separator;
	GtkWidget *memories_grid;
	GtkWidget *tally[6];

	guint16 tally_data;
	double tally_brightness;
	gboolean tally_1_is_on;

	GtkWidget *error_drawing_area;
	int error_code;

	GtkWidget *ghost_body;
} ptz_t;

typedef struct {
	gpointer pointer;
	GThread *thread;
} ptz_thread_t;


void init_ptz (ptz_t *ptz);

gboolean ptz_is_on (ptz_t *ptz);

gboolean ptz_is_off (ptz_t *ptz);

gboolean free_ptz_thread (ptz_thread_t *ptz_thread);

gpointer start_ptz (ptz_thread_t *ptz_thread);

gpointer switch_ptz_on (ptz_thread_t *ptz_thread);

gpointer switch_ptz_off (ptz_thread_t *ptz_thread);

void create_ptz_widgets (ptz_t *ptz);

void create_ghost_ptz_widgets (ptz_t *ptz);


//cameras_set.h
#define CAMERAS_SET_NAME_LENGTH 20
#define MAX_CAMERAS_SET 15
#define MAX_CAMERAS 15

#define MEMORIES_NAME_LENGTH 20


typedef struct cameras_set_s {
	char name[CAMERAS_SET_NAME_LENGTH + 1];

	int number_of_cameras;
	ptz_t **ptz_ptr_array;

	int number_of_ghost_cameras;

	GtkWidget *entry_widgets_padding;
	GtkWidget *entry_widgets[MAX_MEMORIES];
	GtkAdjustment *entry_scrolled_window_hadjustment;	
	GtkWidget *name_grid_box;
	GtkWidget *memories_grid_box;
	GtkAdjustment *memories_scrolled_window_hadjustment;
	GtkWidget *memories_labels_padding;
	GtkWidget *memories_labels[MAX_MEMORIES];
	GtkAdjustment *label_scrolled_window_hadjustment;
	GtkAdjustment *scrolled_window_vadjustment;
	GtkWidget *page;
	gint page_num;
	GtkWidget *list_box_row;

	int thumbnail_width;

	struct cameras_set_s *next;
} cameras_set_t;


extern const char *cameras_set_label;
extern const char *cameras_label;

extern int number_of_cameras_sets;

extern cameras_set_t *cameras_sets;

extern cameras_set_t *current_camera_set;

extern cameras_set_t *new_cameras_set;

extern cameras_set_t *cameras_set_with_error;


void show_cameras_set_configuration_window (void);

void add_cameras_set (void);

void delete_cameras_set (void);

void add_cameras_set_to_main_window_notebook (cameras_set_t *cameras_set);


//control_window.h
extern char focus_near_speed_cmd[5];
extern char focus_far_speed_cmd[5];

extern char zoom_wide_speed_cmd[5];
extern char zoom_tele_speed_cmd[5];


void show_control_window (ptz_t *ptz);

gboolean update_auto_focus_toggle_button (ptz_t *ptz);

void create_control_window (ptz_t *ptz);


//settings.h
extern const char *settings_txt;

extern gboolean backup_needed;

extern GtkWidget *settings_window;

extern GtkWidget *settings_list_box;
extern GtkWidget *settings_new_button;


void create_settings_window (void);

void load_config_file (void);

void save_config_file (void);


//protocol.h
extern char my_ip_adresse[16];

void init_protocol (void);

void init_ptz_cmd (ptz_t *ptz);

void send_update_start_cmd (ptz_t *ptz);

void send_update_stop_cmd (ptz_t *ptz);

void send_ptz_request_command (ptz_t *ptz, char* cmd, int *response);

void send_ptz_request_command_string (ptz_t *ptz, char* cmd, char *response);

void send_ptz_control_command (ptz_t *ptz, char* cmd, gboolean wait);

void send_cam_request_command (ptz_t *ptz, char* cmd, int *response);

void send_cam_control_command (ptz_t *ptz, char* cmd);

void send_thumbnail_request_cmd (memory_t *memory);

void send_jpeg_image_request_cmd (ptz_t *ptz);


//controller.h
extern gboolean controller_is_used;

extern char controller_ip_adresse[16];
extern gboolean controller_ip_adresse_is_valid;

extern struct sockaddr_in controller_adresse;


gpointer controller_switch_ptz (ptz_thread_t *ptz_thread);

void init_controller (void);


//update_notification.h
#define UPDATE_NOTIFICATION_TCP_PORT 31004


extern struct sockaddr_in update_notification_adresse;


void init_update_notification (void);

void start_update_notification (void);

void stop_update_notification (void);


//sw_p_08.h
#define SW_P_08_TCP_PORT 8000


extern struct sockaddr_in sw_p_08_adresse;

extern gboolean sw_p_08_server_started;

extern int number_of_matrix_source;

extern char *sw_p_08_grid_txt;


void ask_to_connect_pgm_to_ctrl_opv (void);

void ask_to_connect_ptz_to_ctrl_opv (ptz_t *ptz);

void show_matrix_window (void);

void init_sw_p_08 (void);

void start_sw_p_08 (void);

void stop_sw_p_08 (void);


//tally.h
#define TSL_UMD_V5_UDP_PORT 8900


extern gboolean send_ip_tally;

extern struct sockaddr_in tsl_umd_v5_adresse;


gboolean tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean ghost_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean control_window_tally_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean ghost_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

gboolean control_window_name_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

void init_tally (void);

void start_tally (void);

void stop_tally (void);


//error.h
gboolean clear_ptz_error (ptz_t *ptz);

gboolean Motor_Driver_Error (ptz_t *ptz);
gboolean Pan_Sensor_Error (ptz_t *ptz);
gboolean Tilt_Sensor_Error (ptz_t *ptz);
gboolean Controller_RX_Over_run_Error (ptz_t *ptz);
gboolean Controller_RX_Framing_Error (ptz_t *ptz);
gboolean Network_RX_Over_run_Error (ptz_t *ptz);
gboolean Network_RX_Framing_Error (ptz_t *ptz);
gboolean Controller_RX_Command_Buffer_Overflow (ptz_t *ptz);
gboolean Network_RX_Command_Buffer_Overflow (ptz_t *ptz);
gboolean System_Error (ptz_t *ptz);
gboolean Spec_Limit_Over (ptz_t *ptz);
gboolean Network_communication_Error (ptz_t *ptz);
gboolean CAMERA_communication_Error (ptz_t *ptz);
gboolean CAMERA_RX_Over_run_Error (ptz_t *ptz);
gboolean CAMERA_RX_Framing_Error (ptz_t *ptz);
gboolean CAMERA_RX_Command_Buffer_Overflow (ptz_t *ptz);

gboolean camera_is_unreachable (ptz_t *ptz);

gboolean ABB_execution_failed (ptz_t *ptz);
gboolean ABB_execution_failed_busy_status (ptz_t *ptz);

gboolean error_draw (GtkWidget *widget, cairo_t *cr, ptz_t *ptz);

void start_error_log (void);

void stop_error_log (void);


//main_window.h
#define MARGIN_VALUE 5


extern const char *warning_txt;

extern GtkWidget *main_window;
extern GtkWidget *main_window_notebook;

extern GtkWidget *thumbnail_size_scale;

extern GtkWidget *store_toggle_button;
extern GtkWidget *delete_toggle_button;
extern GtkWidget *link_toggle_button;

extern GtkWidget *switch_cameras_on_button;
extern GtkWidget *switch_cameras_off_button;

extern gdouble thumbnail_size;
extern int thumbnail_width, thumbnail_height;

extern GdkSeat *seat;
extern GdkDevice *mouse;
extern GdkDevice *trackball;


gboolean digit_key_press (GtkEntry *entry, GdkEventKey *event);

gboolean show_quit_confirmation_window (void);


#endif
