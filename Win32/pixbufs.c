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

#include <gdk/gdk.h>

#include "icon.h"
#include "pad.h"
#include "up.h"
#include "down.h"
#include "logo.h"
#include "grille_1.h"
#include "grille_2.h"


GdkPixbuf *pixbuf_icon;
GdkPixbuf *pixbuf_pad;
GdkPixbuf *pixbuf_up;
GdkPixbuf *pixbuf_down;
GdkPixbuf *pixbuf_logo;
GdkPixbuf *pixbuf_grille_1;
GdkPixbuf *pixbuf_grille_2;


void load_pixbufs (void)
{
	pixbuf_icon = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_icon), pixbuf_inline_icon, FALSE, NULL);
	pixbuf_pad = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_pad), pixbuf_inline_pad, FALSE, NULL);
	pixbuf_up = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_up), pixbuf_inline_up, FALSE, NULL);
	pixbuf_down = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_down), pixbuf_inline_down, FALSE, NULL);
	pixbuf_logo = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_logo), pixbuf_inline_logo, FALSE, NULL);
	pixbuf_grille_1 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_grille_1), pixbuf_inline_grille_1, FALSE, NULL);
	pixbuf_grille_2 = gdk_pixbuf_new_from_inline (sizeof (pixbuf_inline_grille_2), pixbuf_inline_grille_2, FALSE, NULL);
}

