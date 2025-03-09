/*
 * copyright (c) 2020 2021 Thomas Paillet <thomas.paillet@net-c.fr>

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


#ifdef _WIN32
extern GdkPixbuf *pixbuf_icon;
extern GdkPixbuf *pixbuf_logo;

void load_pixbufs (void);
#elif defined (__linux)
#include "Linux/gresources.h"
#endif


const char application_name_txt[] = "Mémoires Pan Tilt Zoom pour caméras PTZ Panasonic";
const char warning_txt[] = "Attention !";
const char about_txt[] = "A propos";

GtkWidget *main_window, *main_window_notebook;
GtkWidget *thumbnail_size_scale;
GtkWidget *store_toggle_button, *delete_toggle_button, *link_toggle_button;
GtkWidget *switch_cameras_on_button, *switch_cameras_off_button;

gboolean fullscreen = TRUE;
gdouble thumbnail_size = 1.0;
int thumbnail_width = 320;
int thumbnail_height = 180;

GdkSeat *seat;
GdkDevice *mouse, *trackball = NULL;


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
	else return GDK_EVENT_STOP;
}

void ptz_main_quit (GtkWidget *confirmation_window)
{
	gtk_widget_destroy (confirmation_window);

	gtk_widget_hide (main_window);

	gtk_main_quit ();
}

gboolean quit_confirmation_window_key_press (GtkWidget *confirmation_window, GdkEventKey *event)
{
	if ((event->keyval == GDK_KEY_n) || (event->keyval == GDK_KEY_N) || (event->keyval == GDK_KEY_Escape)) gtk_widget_destroy (confirmation_window);
	else if ((event->keyval == GDK_KEY_o) || (event->keyval == GDK_KEY_O)) ptz_main_quit (confirmation_window);

	return GDK_EVENT_PROPAGATE;
}

gboolean show_quit_confirmation_window (void)
{
	GtkWidget *confirmation_window, *box2, *box3, *widget;

	confirmation_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (confirmation_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (confirmation_window), warning_txt);
	gtk_window_set_modal (GTK_WINDOW (confirmation_window), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (confirmation_window), GTK_WINDOW (main_window));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (confirmation_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (confirmation_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (confirmation_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (confirmation_window), "key-press-event", G_CALLBACK (quit_confirmation_window_key_press), NULL);

	box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box2), MARGIN_VALUE);
		widget = gtk_label_new ("Etes-vous sûr de vouloir quitter l'application ?");
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

	gtk_window_set_resizable (GTK_WINDOW (confirmation_window), FALSE);
	gtk_widget_show_all (confirmation_window);

	return GDK_EVENT_STOP;
}

void thumbnail_size_value_changed (GtkRange *range)
{
	int old_thumbnail_width = thumbnail_width;
	int i, j;
	ptz_t *ptz;

	thumbnail_size = gtk_range_get_value (range);
	thumbnail_width = 320 * thumbnail_size;
	thumbnail_height = 180 * thumbnail_size;
	current_cameras_set->thumbnail_width = thumbnail_width;

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		ptz = current_cameras_set->ptz_ptr_array[i];

		if (ptz->active) {
			gtk_widget_set_size_request (ptz->name_drawing_area, thumbnail_height, thumbnail_height + 10);
			gtk_widget_set_size_request (ptz->error_drawing_area, 8, thumbnail_height + 10);

			for (j = 0; j < MAX_MEMORIES; j++) {
				if (!ptz->memories[j].empty) {
					gtk_button_set_image (GTK_BUTTON (ptz->memories[j].button), NULL);
					gtk_widget_set_size_request (ptz->memories[j].button, thumbnail_width + 10, thumbnail_height + 10);
					if (old_thumbnail_width != 320) g_object_unref (G_OBJECT (ptz->memories[j].scaled_pixbuf));
					if (thumbnail_width == 320) ptz->memories[j].image = gtk_image_new_from_pixbuf (ptz->memories[j].pixbuf);
					else {
						ptz->memories[j].scaled_pixbuf = gdk_pixbuf_scale_simple (ptz->memories[j].pixbuf, thumbnail_width, thumbnail_height, GDK_INTERP_BILINEAR);
						ptz->memories[j].image = gtk_image_new_from_pixbuf (ptz->memories[j].scaled_pixbuf);
					}
					gtk_button_set_image (GTK_BUTTON (ptz->memories[j].button), ptz->memories[j].image);
				} else gtk_widget_set_size_request (ptz->memories[j].button, thumbnail_width + 10, thumbnail_height + 10);
			}
		} else {
			gtk_widget_set_size_request (ptz->name_drawing_area, thumbnail_height + 8, thumbnail_height / 2);
			gtk_widget_set_size_request (ptz->ghost_body, (thumbnail_width + 10) * MAX_MEMORIES, thumbnail_height / 2);
		}

		gtk_widget_set_size_request (current_cameras_set->entry_widgets_padding, thumbnail_height + 13, 34);
		gtk_widget_set_size_request (current_cameras_set->memories_labels_padding, thumbnail_height + 13, 10);

		for (j = 0; j < MAX_MEMORIES; j++) {
			gtk_widget_set_size_request (current_cameras_set->entry_widgets[j], thumbnail_width + 6, 34);
			gtk_widget_set_size_request (current_cameras_set->memories_labels[j], thumbnail_width + 6, 10);
		}
	}

	backup_needed = TRUE;
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
		if (current_cameras_set->ptz_ptr_array[i]->ip_address_is_valid) {
			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->pointer = current_cameras_set->ptz_ptr_array[i];
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_on, ptz_thread);
		}
	}
}

void switch_cameras_off (void)
{
	int i;
	ptz_thread_t *ptz_thread;

	for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
		if (current_cameras_set->ptz_ptr_array[i]->ip_address_is_valid) {
			ptz_thread = g_malloc (sizeof (ptz_thread_t));
			ptz_thread->pointer = current_cameras_set->ptz_ptr_array[i];
			ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)switch_ptz_off, ptz_thread);
		}
	}
}

gpointer get_camera_screen_shot (ptz_thread_t *ptz_thread)
{
	send_jpeg_image_request_cmd (ptz_thread->ptz_ptr);

	g_idle_add ((GSourceFunc)free_ptz_thread, ptz_thread);

	return NULL;
}

gboolean about_window_key_press (GtkWidget *window, GdkEventKey *event)
{
	gtk_widget_destroy (window);

	return GDK_EVENT_STOP;
}

void show_about_window (void)
{
	GtkWidget *about_window, *box, *widget;
	char gtk_version[64];

	about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW (about_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (about_window), about_txt);
	gtk_window_set_transient_for (GTK_WINDOW (about_window), GTK_WINDOW (main_window));
	gtk_window_set_modal (GTK_WINDOW (about_window), TRUE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (about_window), FALSE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (about_window), FALSE);
	gtk_window_set_position (GTK_WINDOW (about_window), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect (G_OBJECT (about_window), "key-press-event", G_CALLBACK (about_window_key_press), NULL);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), MARGIN_VALUE);
		widget = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (widget), "<b>Mémoires Pan Tilt Zoom pour caméras PTZ Panasonic</b>");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Version 1.2");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
#ifdef _WIN32
		widget = gtk_image_new_from_pixbuf (pixbuf_logo);
#elif defined (__linux)
		widget = gtk_image_new_from_resource ("/org/PTZ-Memory/images/logo.png");
#endif
		gtk_widget_set_margin_top (widget, MARGIN_VALUE);
		gtk_widget_set_margin_bottom (widget, MARGIN_VALUE);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Panasonic Interface Specifications v1.12");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Router Control Protocols document n°SW-P-88 issue n°4b");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("TSL UMD Protocol V5.0");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		sprintf (gtk_version, "Compiled against GTK+ version %d.%d.%d", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
		widget = gtk_label_new (gtk_version);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Interface based on \"Adwaita-dark\" theme");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("This software use the libjpeg-turbo8 library provided by the Independent JPEG Group");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("Copyright (c) 2020 2021 Thomas Paillet");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		widget = gtk_label_new ("GNU General Public License version 3");
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (about_window), box);

	gtk_window_set_resizable (GTK_WINDOW (about_window), FALSE);
	gtk_widget_show_all (about_window);
}

void main_window_notebook_switch_page (GtkNotebook *notebook, GtkWidget *page, guint page_num)
{
	g_mutex_lock (&cameras_sets_mutex);

	for (current_cameras_set = cameras_sets; current_cameras_set != NULL; current_cameras_set = current_cameras_set->next) {
		if (current_cameras_set->page == page) break;
	}

	if (current_cameras_set->thumbnail_width != thumbnail_width) {
		thumbnail_width = current_cameras_set->thumbnail_width;
		thumbnail_size_value_changed (GTK_RANGE (thumbnail_size_scale));
	}

	g_mutex_unlock (&cameras_sets_mutex);

	if (page_num != tally_cameras_set) tell_cameras_set_is_selected (page_num);
}

void main_window_notebook_page_reordered (GtkNotebook *notebook, GtkWidget *page, guint page_num)
{
	cameras_set_t *cameras_set_itr;

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		cameras_set_itr->page_num = gtk_notebook_page_num (GTK_NOTEBOOK (main_window_notebook), cameras_set_itr->page);
	}

	backup_needed = TRUE;
}

gboolean main_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	int i;
	ptz_t *ptz;
	gdouble adjustment_value;
	ptz_thread_t *ptz_thread[MAX_CAMERAS];
	ptz_thread_t *controller_thread;

	if (event->keyval == GDK_KEY_Escape) ask_to_connect_pgm_to_ctrl_opv ();
	else if ((GDK_KEY_F1 <= event->keyval) && (event->keyval <= GDK_KEY_F15)) {
		if (current_cameras_set != NULL) {
			i = event->keyval - GDK_KEY_F1;

			if (i < current_cameras_set->number_of_cameras) {
				ptz = current_cameras_set->ptz_ptr_array[i];

				if (ptz->active && gtk_widget_get_sensitive (ptz->name_grid)) {
					gtk_window_set_position (GTK_WINDOW (ptz->control_window.window), GTK_WIN_POS_CENTER);

					show_control_window (ptz);

					if (trackball != NULL) gdk_device_get_position_double (mouse, NULL, &ptz->control_window.x, &ptz->control_window.y);

					if (controller_is_used && controller_ip_address_is_valid) {
						controller_thread = g_malloc (sizeof (ptz_thread_t));
						controller_thread->pointer = ptz;
						controller_thread->thread = g_thread_new (NULL, (GThreadFunc)controller_switch_ptz, controller_thread);
					}
				}

				ask_to_connect_ptz_to_ctrl_opv (ptz);
			}
		}

		return GDK_EVENT_STOP;
	} else if (event->state & GDK_MOD1_MASK) {
		if ((event->keyval == GDK_KEY_q) || (event->keyval == GDK_KEY_Q)) show_quit_confirmation_window ();
		else if ((event->keyval == GDK_KEY_f) || (event->keyval == GDK_KEY_F)) {
			if (fullscreen) {
				gtk_window_unfullscreen (GTK_WINDOW (main_window));
				fullscreen = FALSE;
			} else {
				gtk_window_fullscreen (GTK_WINDOW (main_window));
				fullscreen = TRUE;
			}

			return GDK_EVENT_STOP;
		} else if ((event->keyval == GDK_KEY_i) || (event->keyval == GDK_KEY_I)) {
			if (current_cameras_set != NULL) {
				for (i = 0; i < current_cameras_set->number_of_cameras; i++) {
					ptz = current_cameras_set->ptz_ptr_array[i];

					if (ptz->active && gtk_widget_get_sensitive (ptz->name_grid)) {
						ptz_thread[i] = g_malloc (sizeof (ptz_thread_t));
						ptz_thread[i]->ptz_ptr = ptz;
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
			adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_hadjustment) - 50;
			gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_hadjustment, adjustment_value);

			return GDK_EVENT_STOP;
		} else if (event->keyval == GDK_KEY_Right) {
			adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_hadjustment) + 50;
			gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_hadjustment, adjustment_value);

			return GDK_EVENT_STOP;
		} else if (event->keyval == GDK_KEY_Up) {
			gtk_adjustment_set_value (current_cameras_set->scrolled_window_vadjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_vadjustment) - 50);

			return GDK_EVENT_STOP;
		} else if (event->keyval == GDK_KEY_Down) {
			gtk_adjustment_set_value (current_cameras_set->scrolled_window_vadjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_vadjustment) + 50);

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
			adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_hadjustment) - 50;
			gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_hadjustment, adjustment_value);
		} else if (event->direction == GDK_SCROLL_RIGHT) {
			adjustment_value = gtk_adjustment_get_value (current_cameras_set->memories_scrolled_window_hadjustment) + 50;
			gtk_adjustment_set_value (current_cameras_set->entry_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->memories_scrolled_window_hadjustment, adjustment_value);
			gtk_adjustment_set_value (current_cameras_set->label_scrolled_window_hadjustment, adjustment_value);
		} else if (event->direction == GDK_SCROLL_UP) {
			gtk_adjustment_set_value (current_cameras_set->scrolled_window_vadjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_vadjustment) - 50);
		} else if (event->direction == GDK_SCROLL_DOWN) {
			gtk_adjustment_set_value (current_cameras_set->scrolled_window_vadjustment, gtk_adjustment_get_value (current_cameras_set->scrolled_window_vadjustment) + 50);
		}
	}

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
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (show_quit_confirmation_window), NULL);

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
			gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_button_set_use_underline (GTK_BUTTON (widget), TRUE);
			g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (create_settings_window), NULL);
		gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					widget = gtk_label_new ("-");
				gtk_box_pack_start (GTK_BOX (box4), widget, FALSE, FALSE, 0);

					thumbnail_size_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.5, 1.0, 0.1);
					gtk_scale_set_draw_value (GTK_SCALE (thumbnail_size_scale), FALSE);
					gtk_scale_set_has_origin (GTK_SCALE (thumbnail_size_scale), FALSE);
					gtk_widget_set_size_request (thumbnail_size_scale, 200, 10);
					gtk_widget_set_margin_start (thumbnail_size_scale, 10);
					gtk_widget_set_margin_end (thumbnail_size_scale, 10);
				gtk_box_pack_start (GTK_BOX (box4), thumbnail_size_scale, FALSE, FALSE, 0);

					widget = gtk_label_new ("+");
				gtk_box_pack_start (GTK_BOX (box4), widget, FALSE, FALSE, 0);
			gtk_box_set_center_widget (GTK_BOX (box3), box4);
		gtk_box_pack_start (GTK_BOX (box2), box3, TRUE, TRUE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				store_toggle_button = gtk_toggle_button_new_with_label ("_Enregister");
				gtk_style_context_add_provider (gtk_widget_get_style_context (store_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_red), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (store_toggle_button), TRUE);
				g_signal_connect (G_OBJECT (store_toggle_button), "toggled", G_CALLBACK (store_toggle_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box3), store_toggle_button, FALSE, FALSE, 0);

				delete_toggle_button = gtk_toggle_button_new_with_label ("_Supprimer");
				gtk_widget_set_margin_start (delete_toggle_button, 6);
				gtk_widget_set_margin_end (delete_toggle_button, 6);
				gtk_style_context_add_provider (gtk_widget_get_style_context (delete_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_red), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (delete_toggle_button), TRUE);
				g_signal_connect (G_OBJECT (delete_toggle_button), "toggled", G_CALLBACK (delete_toggle_button_clicked), NULL);
			gtk_box_pack_start (GTK_BOX (box3), delete_toggle_button, FALSE, FALSE, 0);

				link_toggle_button = gtk_toggle_button_new_with_label ("_Lier");
				gtk_style_context_add_provider (gtk_widget_get_style_context (link_toggle_button), GTK_STYLE_PROVIDER (css_provider_toggle_button_blue), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_button_set_use_underline (GTK_BUTTON (link_toggle_button), TRUE);
			gtk_box_pack_start (GTK_BOX (box3), link_toggle_button, FALSE, FALSE, 0);
		gtk_box_set_center_widget (GTK_BOX (box2), box3);

			widget = gtk_button_new_with_label (about_txt);
			gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (show_about_window), NULL);
		gtk_box_pack_end (GTK_BOX (box2), widget, FALSE, FALSE, 0);

			box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
					switch_cameras_on_button = gtk_button_new_with_label ("Tout allumer");
					gtk_widget_set_margin_end (switch_cameras_on_button, 3);
					gtk_style_context_add_provider (gtk_widget_get_style_context (switch_cameras_on_button), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
					g_signal_connect (G_OBJECT (switch_cameras_on_button), "clicked", G_CALLBACK (switch_cameras_on), NULL);
				gtk_box_pack_start (GTK_BOX (box4), switch_cameras_on_button, FALSE, FALSE, 0);

					switch_cameras_off_button = gtk_button_new_with_label ("Tout éteindre");
					gtk_widget_set_margin_start (switch_cameras_off_button, 3);
					gtk_style_context_add_provider (gtk_widget_get_style_context (switch_cameras_off_button), GTK_STYLE_PROVIDER (css_provider_button), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
					g_signal_connect (G_OBJECT (switch_cameras_off_button), "clicked", G_CALLBACK (switch_cameras_off), NULL);
				gtk_box_pack_start (GTK_BOX (box4), switch_cameras_off_button, FALSE, FALSE, 0);
			gtk_box_set_center_widget (GTK_BOX (box3), box4);
		gtk_box_pack_end (GTK_BOX (box2), box3, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (box1), box2, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (main_window), box1);
}

void device_added_to_seat (GdkSeat *seat, GdkDevice *device)
{
	GList *pointing_devices, *glist;

	pointing_devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_POINTER);
	for (glist = pointing_devices; glist != NULL; glist = glist->next) {
		if (memcmp (gdk_device_get_name (glist->data), "Kensington Slimblade Trackball", 30) == 0) {
			trackball = glist->data;
			break;
		}
	}
	g_list_free (pointing_devices);
}

void device_removed_from_seat (GdkSeat *seat, GdkDevice *device)
{
	if (device == trackball) trackball = NULL;
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
	GList *pointing_devices, *glist;
	cameras_set_t *cameras_set_itr;
	int i;
	ptz_thread_t *ptz_thread;

	WSAInit ();	//_WIN32

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

	init_tally ();

	init_sw_p_08 ();

	main_css_provider = gtk_css_provider_new ();
	file = g_file_new_for_path ("resources" G_DIR_SEPARATOR_S "Adwaita-dark.css");
	gtk_css_provider_load_from_file (main_css_provider, file, NULL);
	g_object_unref (file);

	screen = gdk_screen_get_default ();
	gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (main_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	create_main_window ();

	load_config_file ();

	if (number_of_cameras_sets <= 1) gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_window_notebook), FALSE);

	gtk_range_set_value (GTK_RANGE (thumbnail_size_scale), thumbnail_size);
	g_signal_connect (G_OBJECT (thumbnail_size_scale), "value-changed", G_CALLBACK (thumbnail_size_value_changed), NULL);

	if (number_of_cameras_sets == 0) {
		gtk_widget_set_sensitive (thumbnail_size_scale, FALSE);
		gtk_widget_set_sensitive (switch_cameras_on_button, FALSE);
		gtk_widget_set_sensitive (switch_cameras_off_button, FALSE);
	}

	gtk_widget_show_all (main_window);
	gtk_window_set_focus (GTK_WINDOW (main_window), NULL);

	if (number_of_cameras_sets == 0) {
		create_settings_window ();
		add_cameras_set ();
	} else if (cameras_set_with_error != NULL) {
		create_settings_window ();
		show_cameras_set_configuration_window ();
	}

	start_error_log ();

	start_update_notification ();

	seat = gdk_display_get_default_seat (gdk_display_get_default ());
	g_signal_connect (G_OBJECT (seat), "device-added", G_CALLBACK (device_added_to_seat), NULL);
	g_signal_connect (G_OBJECT (seat), "device-removed", G_CALLBACK (device_removed_from_seat), NULL);
	mouse = gdk_seat_get_pointer (seat);
	pointing_devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_POINTER);
	for (glist = pointing_devices; glist != NULL; glist = glist->next) {
		if (memcmp (gdk_device_get_name (glist->data), "Kensington Slimblade Trackball", 30) == 0) {
			trackball = glist->data;
			break;
		}
	}
	g_list_free (pointing_devices);

	for (cameras_set_itr = cameras_sets; cameras_set_itr != NULL; cameras_set_itr = cameras_set_itr->next) {
		for (i = 0; i < cameras_set_itr->number_of_cameras; i++) {
			if (cameras_set_itr->ptz_ptr_array[i]->active) {
				ptz_is_off (cameras_set_itr->ptz_ptr_array[i]);

				if (cameras_set_itr->ptz_ptr_array[i]->ip_address_is_valid) {
					ptz_thread = g_malloc (sizeof (ptz_thread_t));
					ptz_thread->pointer = cameras_set_itr->ptz_ptr_array[i];
					ptz_thread->thread = g_thread_new (NULL, (GThreadFunc)start_ptz, ptz_thread);
				}
			}
		}
	}

	start_tally ();

	start_sw_p_08 ();

	gtk_main ();

	if (backup_needed) save_config_file ();

	stop_sw_p_08 ();

	stop_tally ();

	stop_update_notification ();

	gtk_widget_destroy (main_window);

	stop_error_log ();

	WSACleanup ();	//_WIN32

	return 0;
}

