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

#ifndef __PIXMAPS_H__
#define __PIXMAPS_H__

#include <gtk/gtk.h>

#ifdef GUI_GTK3
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

struct pixmap {
	GdkPixbuf *pixbuf;
#ifdef GUI_GTK2
	GdkPixmap *pix;
	GdkBitmap *mask;
#endif
};

struct cached_pixmap {
	unsigned key;
	GdkPixbuf *pixbuf;
#ifdef GUI_GTK2
	GdkPixmap *pix;
	GdkBitmap *mask;
#endif
	int weight;
};

extern struct pixmap update_pix;
extern struct pixmap refresh_pix;
extern struct pixmap refrsel_pix;
extern struct pixmap stop_pix;

extern struct pixmap connect_pix;
extern struct pixmap observe_pix;
extern struct pixmap record_pix;

extern struct pixmap sfilter_pix;
extern struct pixmap sfilter_cfg_pix;
extern struct pixmap pfilter_pix;
extern struct pixmap pfilter_cfg_pix;

extern struct pixmap gplus_pix;
extern struct pixmap rminus_pix;

extern struct pixmap man_black_pix;
extern struct pixmap man_red_pix;
extern struct pixmap man_yellow_pix;

extern struct pixmap group_pix[];
extern struct pixmap buddy_pix[];

extern struct pixmap error_pix;
extern struct pixmap delete_pix;

extern struct pixmap server_status[];

extern struct pixmap locked_pix;

extern struct pixmap punkbuster_pix;
extern struct pixmap locked_punkbuster_pix;

extern int pixmap_height (struct pixmap *pix);
extern int pixmap_width (struct pixmap *pix);

extern void free_pixmap (struct pixmap *pixmap);
extern void free_pixmaps (void);
extern void init_pixmaps (GtkWidget *window);

extern void ensure_buddy_pix (GtkWidget *window, int n);

extern void two_colors_pixmap (GdkWindow *window, int width, int height, GdkColor *top, GdkColor *bottom, struct pixmap *pix);

extern void create_server_pixmap (GtkWidget *window, struct pixmap *stype, int n, struct pixmap *pix);

extern gboolean pixmap_cache_lookup (GSList *cache, struct pixmap *pix, unsigned key);
extern void pixmap_cache_add (GSList **cache, struct pixmap *pix, unsigned key);
extern void pixmap_cache_clear (GSList **cache, int maxitems);

/** \brief concatenate two pixmaps
 *
 * horizontal concatenation
 * @param window
 * @param dest destination pixmap
 * @param s1 first pixmap
 * @param s2 second pixmap
 * @returns dest for convenience
 */
struct pixmap* cat_pixmaps (GtkWidget *window, struct pixmap *dest, struct pixmap* s1, struct pixmap* s2);

#endif /* __PIXMAPS_H__ */

