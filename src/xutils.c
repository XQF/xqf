/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "pixmaps.h"
#include "loadpixmap.h"


void iconify_window (GdkWindow *window) {
  Window xwindow;

  if (!window)
    return;



  xwindow = GDK_WINDOW_XWINDOW(window);


  XIconifyWindow (GDK_DISPLAY (), xwindow, DefaultScreen (GDK_DISPLAY ()));
}

static const char* minimize_icon = "xqf_48x48.png";

void window_set_icon (GtkWidget *win)
{
#ifdef USE_GTK2
  GdkPixbuf* pixbuf;
  pixbuf = load_pixmap_as_pixbuf(minimize_icon);
  if(pixbuf)
  {
    gtk_window_set_icon (GTK_WINDOW (main_window), pixbuf);
    gdk_pixbuf_unref (pixbuf);
  }
#else
  static struct pixmap pix;

  if(!load_pixmap_as_pixmap(win, minimize_icon, &pix))
    return;

  gdk_window_set_icon(win->window, NULL, pix.pix, pix.mask);
  gdk_window_set_icon_name(win->window, "XQF");
#endif
}
