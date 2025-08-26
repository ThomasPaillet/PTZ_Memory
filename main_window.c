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

#include "main_window.h"

#include "cameras_set.h"
#include "control_window.h"
#include "controller.h"
#include "free_d.h"
#include "interface.h"
#include "logging.h"
#include "protocol.h"
#include "settings.h"
#include "sw_p_08.h"
#include "tally.h"
#include "trackball.h"
#include "update_notification.h"


#ifdef _WIN32
extern GdkPixbuf *pixbuf_icon;

void load_pixbufs (void);
#elif defined (__linux)
#include "Linux/gresources.h"
#endif

#define BUTTON_MARGIN_VALUE 6


const char application_name_txt[] = "Mémoires Pan Tilt Zoom pour caméras PTZ Panasonic";
const char warning_txt[] = "Attention !";

GtkWidget *main_window, *main_event_box, *main_window_notebook;
gulong main_event_box_motion_notify_handler_id;

GtkWidget *interface_button;
GtkWidget *store_toggle_button, *delete_toggle_button, *link_toggle_button;
GtkWidget *switch_cameras_on_button, *switch_cameras_off_button;

gboolean fullscreen = TRUE;


gboolean digit_key_press (GtkEntry *entry, GdkEventKey *event)
{
	if ((event->keyval >= GDK_KEY_0) && (event->keyval <= GDK_KEY_9)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_KP_0) && (event->keyval <= GDK_KEY_KP_9)) return GDK_EVENT_PROPAGATE;
	else if (event->state & GDK_CONTROL_MASK) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval == GDK_KEY_BackSpace) || (event->keyval == GDK_KEY_Tab) || (event->keyval == GDK_KEY_Clear) || (event->keyval == GDK_KEY_Return) || \
		(event->keyval == GDK_KEY_Escape) || (event->keyval == GDK_KEY_Delete) || (event->keyval == GDK_KEY_Cancel)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_Home) && (event->keyval <= GDK_KEY_Select)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_Execute) && (event->keyval <= GDK_KEY_Redo)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval == GDK_KEY_KP_Tab) || (event->keyval == GDK_KEY_KP_Enter)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_KP_Home) && (event->keyval <= GDK_KEY_KP_Delete)) return GDK_EVENT_PROPAGATE;
	else if ((event->keyval >= GDK_KEY_Left) && (event->keyval <= GDK_KEY_Right)) return GDK_EVENT_PROPAGATE;
	else return GDK_EVENT_STOP;
}

void ptz_main_quit (GtkWidget *confirmation_window)
{
	gtk_widget_destroy (confirmation_window);

	gtk_widget_hide (main_window);

	gtk_main_quit ();
}

gboolean exit_confirmation_window_key_press (GtkWidget *confirmation_window, GdkEventKey *event)
{
	if ((event->keyval == GDK_KEY_n) || (event->keyval == GDK_KEY_N) || (event->keyval == GDK_KEY_Escape)) gtk_widget_destroy (confirmation_window);
	else if ((event->keyval == GDK_KEY_o) || (event->keyval == GDK_KEY_O)|| (event->keyval == GDK_KEY_Return)) ptz_main_quit (confirmation_window);

	return GDK_EVENT_PROPAGATE;
}

gboolean show_exit_confirmation_window (void)
{
	GtkWidget *confirmation_window, *box2, *box3, *widget;

	confirmation_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (confirmation_window), warning_txt);
	gtk_window_set_type_hint (GTK_WINDOW (confirmation_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal (GTK_WINDOW (confirmation_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (confirmation_window), GTK_WINDOW (main_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (confirmation_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (confirmation_window), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (confirmation_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (confirmation_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (confirmation_window), "key-press-event", G_CALLBACK (exit_confirmation_window_key_press), NULL);

	box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box2), MARGIN_VALUE);
		widget = gtk_label_new ("Etes-vous sûr de vouloir quitter le logiciel ?");
		gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

		box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_halign (box3, GTK_ALIGN_CENTER);
		gtk_widget_set_margin_top (box3, MARGIN_VALUE);
		gtk_box_set_spacing (GTK_BOX (box3), MARGIN_VALUE);
		gtk_box_set_homogeneous (GTK_BOX (box3), TRUE);
			widget = gtk_button_new_with_label ("OUI");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (ptz_main_quit), confirmation_window);
			gtk_box_pack_start (GTK_BOX (box3), widget, TRUE, TRUE, 0);

			widget = gtk_button_new_with_label ("NON");
			g_signal_connect_swapped (G_OBJECT (widget), "clicked", G_CALLBACK (gtk_widget_destroy), confirmation_window);
			gtk_box_pack_start (GTK_BOX (box3), widget, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (confirmation_window), box2);

	gtk_widget_show_all (confirmation_window);

	return GDK_EVENT_STOP;
}

void store_toggle_button_clicked (GtkToggleButton *button)
{
	if (gtk_toggle_button_get_active (button)) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delete_toggle_button), FALSE);
}

void delete_toggle_button_clicked (GtkToggleButton *button)
{
	if (gtk_toggle_button_get_active (button)) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (store_toggle_button), FALSE);
}

void switch_cameras_on (void)
{
	int i;
	ptz_thread_t *ptz_thread;

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		if (current_cameras_set->cameras[i]->ip_address_is_valid) {
			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->ptz = current_cameras_set->cameras[i];
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_on, ptz_thread);
		}
	}
}

void switch_cameras_off (void)
{
	int i;
	ptz_thread_t *ptz_thread;

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		if (current_cameras_set->cameras[i]->ip_address_is_valid) {
			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->ptz = current_cameras_set->cameras[i];
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_off, ptz_thread);
		}
	}
}

gpointer get_camera_screen_shot (ptz_thread_t *ptz_thread)
{
	send_jpeg_image_request_cmd (ptz_thread->ptz);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

void main_window_notebook_switch_page (GtkNotebook *notebook, GtkWidget *page, guint page_num)
{
	int i;
	ptz_t *ptz;
	ultimatte_t *ultimatte;
	gboolean tallies[MAX_CAMERAS];

	g_mutex_lock (&cameras_sets_mutex);

	if (current_cameras_set != NULL) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			ptz = current_cameras_set->cameras[i];

			if (send_ip_tally) {
				if (ptz->tally_1_is_on) {
					tallies[i] = TRUE;

					if (ptz->is_on) send_ptz_control_command (ptz, "#DA0", TRUE);

					ptz->tally_1_is_on = FALSE;
				} else tallies[i] = FALSE;
			}

			if (ptz->monitor_pan_tilt) {
				g_mutex_lock (&ptz->free_d_mutex);

				ptz->monitor_pan_tilt = FALSE;

//				g_thread_join (ptz->monitor_pan_tilt_thread);
				ptz->monitor_pan_tilt_thread = NULL;

				g_mutex_unlock (&ptz->free_d_mutex);
			}

			ultimatte = ptz->ultimatte;

			if ((ultimatte != NULL) && (ultimatte->connected)) disconnect_ultimatte (ultimatte);
		}
	}

	for (current_cameras_set = cameras_sets; current_cameras_set != NULL; current_cameras_set = current_cameras_set->next) {
		if (current_cameras_set->page == page) {
			interface_default = current_cameras_set->layout;

			break;
		}
	}

	if (interface_default.orientation) ultimatte_picto_x = interface_default.thumbnail_height - (22 * interface_default.thumbnail_size);
	else ultimatte_picto_x = interface_default.thumbnail_width + 10 - (22 * interface_default.thumbnail_size);
	ultimatte_picto_y =  30 * interface_default.thumbnail_size;

	if (current_cameras_set != NULL) {
		for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
			ptz = current_cameras_set->cameras[i];

			if (send_ip_tally) {
				if (tallies[i]) {
					if (ptz->is_on) send_ptz_control_command (ptz, "#DA1", TRUE);

					ptz->tally_1_is_on = TRUE;
				}
			}

			if (ptz->is_on) {
				if ((outgoing_free_d_started) && (ptz->model == AW_HE130)) {
					g_mutex_lock (&ptz->free_d_mutex);

					if (ptz->monitor_pan_tilt_thread == NULL) ptz->monitor_pan_tilt_thread = g_thread_new (NULL, (GThreadFunc)monitor_ptz_pan_tilt_position, ptz);

					g_mutex_unlock (&ptz->free_d_mutex);
				}

				ultimatte = ptz->ultimatte;

				if ((ultimatte != NULL) && (!ultimatte->connected) && (ultimatte->connection_thread == NULL)) ultimatte->connection_thread = g_thread_new (NULL, (GThreadFunc)connect_ultimatte, ultimatte);
			}
		}
	}

	g_mutex_unlock (&cameras_sets_mutex);

	if (page_num != tally_cameras_set) tell_cameras_set_is_selected (page_num);
}

void main_window_notebook_page_reordered (GtkNotebook *notebook, GtkWidget *page, guint page_num)
{
	cameras_set_t *cameras_set_itr;

	g_mutex_lock (&cameras_sets_mutex);

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		cameras_set_itr->page_num = gtk_notebook_page_num (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page);
	}

	g_mutex_unlock (&cameras_sets_mutex);

	backup_needed = TRUE;
}

gboolean main_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	int i;
	ptz_t *ptz;
	gdouble adjustment_value;
	ptz_thread_t *ptz_thread[MAX_CAMERAS];
	ptz_thread_t *controller_thread;

	if (event->keyval == GDK_KEY_Escape) {
		ask_to_connect_pgm_to_ctrl_opv ();

		return GDK_EVENT_STOP;
	} else if ((GDK_KEY_F1 <= event->keyval) && (event->keyval <= GDK_KEY_F15)) {
		if (current_cameras_set != NULL) {
			i = event->keyval - GDK_KEY_F1;

			if (i < current_cameras_set->number_of_cameras) {
				ptz = current_cameras_set->cameras[i];

				if (ptz->active && gtk_widget_get_sensitive (ptz->name_grid)) {
					show_control_window (ptz, GTK_WIN_POS_CENTER);

					if (controller_is_used && controller_ip_address_is_valid) {
						controller_thread = g_malloc (sizeof (ptz_thread_t));
						controller_thread->ptz = ptz;
						controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
					}
				}

				ask_to_connect_ptz_to_ctrl_opv (ptz);
			}
		}

		return GDK_EVENT_STOP;
	} else if (event->state & GDK_MOD1_MASK) {
		if ((event->keyval == GDK_KEY_f) || (event->keyval == GDK_KEY_F)) {
			if (fullscreen) {
				gtk_window_unfullscreen (GTK_WINDOW (main_window));
				fullscreen = FALSE;
			} else {
				gtk_window_fullscreen (GTK_WINDOW (main_window));
				fullscreen = TRUE;
			}

			return GDK_EVENT_STOP;
		} else if ((event->keyval == GDK_KEY_c) || (event->keyval == GDK_KEY_C)) {
			if (current_cameras_set != NULL) {
				for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
					ptz = current_cameras_set->cameras[i];

					if (ptz->active && gtk_widget_get_sensitive (ptz->name_grid)) {
						ptz_thread[i] = g_malloc (sizeof (ptz_thread_t));
						ptz_thread[i]->ptz = ptz;
					} else ptz_thread[i] = NULL;
				}

				for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
					if (ptz_thread[i] != NULL) ptz_thread[i]->thread = g_thread_new (NULL, (GThreadFunc)get_camera_screen_shot, ptz_thread[i]);
				}
			}

			return GDK_EVENT_STOP;
		}
	} else if (current_cameras_set != NULL) {
		if (event->keyval == GDK_KEY_Left) {
			if (interface_default.orientation) {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) - 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			} else {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) - 50);
			}
		} else if (event->keyval == GDK_KEY_Right) {
			if (interface_default.orientation) {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) + 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			} else {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) + 50);
			}
		} else if (event->keyval == GDK_KEY_Up) {
			if (interface_default.orientation) {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) - 50);
			} else {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) - 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			}

			return GDK_EVENT_STOP;
		} else if (event->keyval == GDK_KEY_Down) {
			if (interface_default.orientation) {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) + 50);
			} else {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) + 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			}

			return GDK_EVENT_STOP;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

gboolean main_window_scroll (GtkWidget *widget, GdkEventScroll *event)
{
	gdouble adjustment_value;

	if (current_cameras_set != NULL) {
		if (event->direction == GDK_SCROLL_LEFT) {
			if (interface_default.orientation) {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) - 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			} else {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) - 50);
			}
		} else if (event->direction == GDK_SCROLL_RIGHT) {
			if (interface_default.orientation) {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) + 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			} else {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) + 50);
			}
		} else if (event->direction == GDK_SCROLL_UP) {
			if (interface_default.orientation) {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) - 50);
			} else {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) - 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			}
		} else if (event->direction == GDK_SCROLL_DOWN) {
			if (interface_default.orientation) {
				gtk_adjustment_set_value (current_cameras_set->scrolled_window_adjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_adjustment) + 50);
			} else {
				adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_adjustment) + 50;

				gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_adjustment, adjustment_value);
				gtk_adjustment_set_value (current_cameras_set->memories_scrollbar_adjustment, adjustment_value);
			}
		}
	}

	return GDK_EVENT_STOP;
}

gboolean main_event_box_events (void)
{
	if (current_ptz != NULL) hide_control_window ();

	return GDK_EVENT_STOP;
}

void create_main_window (void)
{
	GtkCssProvider *css_provider_button, *css_provider_toggle_button_red, *css_provider_toggle_button_blue;
	GFile *file;
	GtkWidget *box1, *box2, *box3, *box4;
	GtkWidget *widget;

	css_provider_button = gtk_css_provider_new ();
	file = g_file_new_for_path ("resources" G_DIR_SEPARATOR_S "Button.css");
	gtk_css_provider_load_from_file (css_provider_button, file, NULL);
	g_object_unref (file);

	css_provider_toggle_button_red = gtk_css_provider_new ();
	file = g_file_new_for_path ("resources" G_DIR_SEPARATOR_S "ToggleButton-red.css");
	gtk_css_provider_load_from_file (css_provider_toggle_button_red, file, NULL);
	g_object_unref (file);

	css_provider_toggle_button_blue = gtk_css_provider_new ();
	file = g_file_new_for_path ("resources" G_DIR_SEPARATOR_S "ToggleButton-blue.css");
	gtk_css_provider_load_from_file (css_provider_toggle_button_blue, file, NULL);
	g_object_unref (file);

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), application_name_txt);
	gtk_window_set_default_size (GTK_WINDOW (main_window), 1200, 1000);
	gtk_window_fullscreen (GTK_WINDOW (main_window));
	g_signal_connect (G_OBJECT (main_window), "key-press-event", G_CALLBACK (main_window_key_press), NULL);
	g_signal_connect (G_OBJECT (main_window), "scroll-event", G_CALLBACK (main_window_scroll), NULL);
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (show_exit_confirmation_window), NULL);

#ifdef _WIN32
	gtk_window_set_icon (GTK_WINDOW (main_window), pixbuf_icon);
#elif defined (__linux)
	widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/icon.png");
	gtk_window_set_icon (GTK_WINDOW (main_window), gtk_image_get_pixbuf (GTK_IMAGE (widget)));
#endif

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		main_window_notebook = gtk_notebook_new ();
		g_signal_connect (G_OBJECT (main_window_notebook), "switch-page", G_CALLBACK (main_window_notebook_switch_page), NULL);
		g_signal_connect (G_OBJECT (main_window_notebook), "page-reordered", G_CALLBACK (main_window_notebook_page_reordered), NULL);
	gtk_box_pack_start (GTK_BOX (box1), main_window_notebook, TRUE, TRUE, 0);

		box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			widget = gtk_button_new_with_label (settings_txt);
			gtk_widget_set_margin_end (widget, BUTTON_MARGIN_VALUE);
			gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_button_set_use_underline (GTK_BUTTON (widget), TRUE);
			g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (show_settings_window), NULL);
		gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

			interface_button = gtk_button_new_with_label (interface_settings_txt);
			gtk_widget_set_margin_end (interface_button, BUTTON_MARGIN_VALUE);
			gtk_style_context_add_provider (gtk_widget_get_style_context (interface_button), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_button_set_use_underline (GTK_BUTTON (interface_button), TRUE);
			g_signal_connect (G_OBJECT (interface_button), "clicked", G_CALLBACK (show_interface_settings_window), NULL);
		gtk_box_pack_start (GTK_BOX (box2), interface_button, FALSE, FALSE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				store_toggle_button = gtk_toggle_button_new_with_label ("_Enregister");
				gtk_widget_set_margin_start (store_toggle_button, BUTTON_MARGIN_VALUE);
				gtk_style_context_add_provider (gtk_widget_get_style_context (store_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_red), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (store_toggle_button), TRUE);
				g_signal_connect (G_OBJECT (store_toggle_button), "toggled", G_CALLBACK (store_toggle_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box3), store_toggle_button, FALSE, FALSE, 0);

				delete_toggle_button = gtk_toggle_button_new_with_label ("_Supprimer");
				gtk_widget_set_margin_start (delete_toggle_button, BUTTON_MARGIN_VALUE);
				gtk_widget_set_margin_end (delete_toggle_button, BUTTON_MARGIN_VALUE);
				gtk_style_context_add_provider (gtk_widget_get_style_context (delete_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_red), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (delete_toggle_button), TRUE);
				g_signal_connect (G_OBJECT (delete_toggle_button), "toggled", G_CALLBACK (delete_toggle_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box3), delete_toggle_button, FALSE, FALSE, 0);

				link_toggle_button = gtk_toggle_button_new_with_label ("_Lier");
				gtk_widget_set_margin_end (link_toggle_button, BUTTON_MARGIN_VALUE);
				gtk_style_context_add_provider (gtk_widget_get_style_context (link_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_blue), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (link_toggle_button), TRUE);
			gtk_box_pack_start (GTK_BOX (box3), link_toggle_button, FALSE, FALSE, 0);
		gtk_box_set_center_widget (GTK_BOX (box2), box3);

			widget = gtk_button_new_with_label ("_Quitter");
			gtk_widget_set_margin_start (widget, BUTTON_MARGIN_VALUE);
			gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_button_set_use_underline (GTK_BUTTON (widget), TRUE);
			g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (show_exit_confirmation_window), NULL);
		gtk_box_pack_end (GTK_BOX (box2), widget, FALSE, FALSE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					switch_cameras_on_button = gtk_button_new_with_label ("Tout allumer");
					gtk_widget_set_margin_end (switch_cameras_on_button, BUTTON_MARGIN_VALUE / 2);
					gtk_style_context_add_provider (gtk_widget_get_style_context (switch_cameras_on_button), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
					g_signal_connect (G_OBJECT (switch_cameras_on_button), "clicked", G_CALLBACK (switch_cameras_on), NULL);
				gtk_box_pack_start (GTK_BOX (box4), switch_cameras_on_button, FALSE, FALSE, 0);

					switch_cameras_off_button = gtk_button_new_with_label ("Tout éteindre");
					gtk_widget_set_margin_start (switch_cameras_off_button, BUTTON_MARGIN_VALUE / 2);
					gtk_style_context_add_provider (gtk_widget_get_style_context (switch_cameras_off_button), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
					g_signal_connect (G_OBJECT (switch_cameras_off_button), "clicked", G_CALLBACK (switch_cameras_off), NULL);
				gtk_box_pack_start (GTK_BOX (box4), switch_cameras_off_button, FALSE, FALSE, 0);
			gtk_box_set_center_widget (GTK_BOX (box3), box4);
		gtk_box_pack_end (GTK_BOX (box2), box3, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (box1), box2, FALSE, FALSE, 0);

	main_event_box = gtk_event_box_new ();
	gtk_widget_set_events (main_event_box, gtk_widget_get_events (main_event_box) | GDK_POINTER_MOTION_MASK);
	g_signal_connect (G_OBJECT (main_event_box), "button-press-event", G_CALLBACK (main_event_box_events), NULL);
	main_event_box_motion_notify_handler_id = g_signal_connect (G_OBJECT (main_event_box), "motion-notify-event", G_CALLBACK (control_window_motion_notify), NULL);
	g_signal_handler_block (main_event_box, main_event_box_motion_notify_handler_id);

	gtk_container_add (GTK_CONTAINER (main_event_box), box1);
	gtk_container_add (GTK_CONTAINER (main_window), main_event_box);
}

#ifdef _WIN32
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
#elif defined (__linux)
int main (int argc, char** argv)
#endif
{
	GtkCssProvider *main_css_provider;
	GFile *file;
	GdkScreen *screen;
	GdkSeat *seat;
	cameras_set_t *cameras_set_itr;
	int i;
	ptz_t *ptz;
	gboolean inactive_cameras_at_begining;
	ptz_thread_t *ptz_thread;
	ultimatte_t *ultimatte;

	WSAInit ();	//_WIN32
#ifdef _WIN32
	AddFontResource (".\\resources\\fonts\\FreeMono.ttf");
	AddFontResource (".\\resources\\fonts\\FreeMonoBold.ttf");
#endif
	g_mutex_init (&cameras_sets_mutex);

	init_protocol ();

	init_controller ();

	init_update_notification ();

#ifdef _WIN32
	gtk_init (NULL, NULL);

	load_pixbufs ();

	FreeConsole ();
#elif defined (__linux)
	gtk_init (&argc, &argv);

	g_resources_register (gresources_get_resource ());
#endif

	init_sw_p_08 ();

	init_tally ();

	init_free_d ();

	main_css_provider = gtk_css_provider_new ();
	file = g_file_new_for_path ("resources" G_DIR_SEPARATOR_S "Adwaita-dark.css");
	gtk_css_provider_load_from_file (main_css_provider, file, NULL);
	g_object_unref (file);

	screen = gdk_screen_get_default ();
	gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (main_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	seat = gdk_display_get_default_seat (gdk_display_get_default ());
	g_signal_connect (G_OBJECT (seat), "device-added", G_CALLBACK (device_added_to_seat), NULL);
	g_signal_connect (G_OBJECT (seat), "device-removed", G_CALLBACK (device_removed_from_seat), NULL);

	pointing_devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_POINTER);

	create_main_window ();

	create_control_window ();

	create_interface_settings_window ();

	create_memory_name_window ();

	load_config_file ();

	init_logging ();

	if (number_of_cameras_sets <= 1) gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_window_notebook), FALSE);

	if (number_of_cameras_sets == 0) {
		gtk_widget_set_sensitive (interface_button, FALSE);
		gtk_widget_set_sensitive (store_toggle_button, FALSE);
		gtk_widget_set_sensitive (delete_toggle_button, FALSE);
		gtk_widget_set_sensitive (link_toggle_button, FALSE);
		gtk_widget_set_sensitive (switch_cameras_on_button, FALSE);
		gtk_widget_set_sensitive (switch_cameras_off_button, FALSE);
	}

	gtk_widget_show_all (main_window);
	gtk_window_set_focus (GTK_WINDOW (main_window), NULL);

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		if (!cameras_set_itr->layout.show_linked_memories_names_entries) gtk_widget_hide (cameras_set_itr->linked_memories_names_entries);

		if (cameras_set_itr->layout.dont_show_not_active_cameras) {
			ptz = cameras_set_itr->cameras[0];

			if (ptz->active) inactive_cameras_at_begining = FALSE;
			else {
				gtk_widget_hide (ptz->name_grid);
				gtk_widget_hide (ptz->memories_grid);

				inactive_cameras_at_begining = TRUE;
			}

			for (i = 1; i < cameras_set_itr->number_of_cameras; i++) {
				ptz = cameras_set_itr->cameras[i];

				if (!ptz->active) {
					gtk_widget_hide (ptz->name_separator);
					gtk_widget_hide (ptz->name_grid);
					gtk_widget_hide (ptz->memories_separator);
					gtk_widget_hide (ptz->memories_grid);
				} else {
					if (inactive_cameras_at_begining) {
						gtk_widget_hide (ptz->name_separator);
						gtk_widget_hide (ptz->memories_separator);

						inactive_cameras_at_begining = FALSE;
					}
				}
			}
		}

		if (!cameras_set_itr->layout.show_linked_memories_names_labels) gtk_widget_hide (cameras_set_itr->linked_memories_names_labels);
	}

	if (number_of_cameras_sets == 0) {
		show_settings_window ();
		add_cameras_set ();
	} else if (cameras_set_with_error != NULL) {
		show_settings_window ();
		show_cameras_set_configuration_window ();
	}

	start_update_notification ();

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			ptz = cameras_set_itr->cameras[i];

			if (ptz->active) {
				ptz_is_off (ptz);

				if (ptz->ip_address_is_valid) {
					ptz_thread = g_malloc (sizeof (ptz_thread_t));
					ptz_thread->ptz = ptz;
					ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)start_ptz, ptz_thread);
				}
			}
		}
	}

	start_sw_p_08 ();

	start_tally ();

	start_incomming_free_d ();

	if (free_d_output_ip_address_is_valid) start_outgoing_freed_d ();


	gtk_main ();


	if (backup_needed) save_config_file ();

	stop_outgoing_freed_d ();

	stop_incomming_free_d ();

	stop_tally ();

	stop_sw_p_08 ();

	stop_update_notification ();

	destroy_control_window ();

	gtk_widget_destroy (main_window);

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			ptz = cameras_set_itr->cameras[i];

			ptz->monitor_pan_tilt = FALSE;

			ultimatte = ptz->ultimatte;

			if ((ultimatte != NULL) && (ultimatte->connected)) disconnect_ultimatte (ultimatte);
		}

		pango_font_description_free (cameras_set_itr->layout.ptz_name_font_description);
		pango_font_description_free (cameras_set_itr->layout.ghost_ptz_name_font_description);
		pango_font_description_free (cameras_set_itr->layout.memory_name_font_description);
		pango_font_description_free (cameras_set_itr->layout.ultimatte_picto_font_description);
	}

	if (logging) stop_logging ();

	g_list_free (pointing_devices);
#ifdef _WIN32
	RemoveFontResource (".\\resources\\fonts\\FreeMonoBold.ttf");
	RemoveFontResource (".\\resources\\fonts\\FreeMono.ttf");
#endif
	WSACleanup ();	//_WIN32

	return 0;
}

