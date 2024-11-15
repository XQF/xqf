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

#include <gtk/gtk.h>

#include "utils.h"
#include "pixmaps.h"
#include "game.h"
#include "loadpixmap.h"


// hack to make dlsym work
#define static

#include ICONS_C_INCLUDE

#include "xpm/update.xpm"
#include "xpm/refresh.xpm"
#include "xpm/refrsel.xpm"
#include "xpm/stop.xpm"

#include "xpm/connect.xpm"
#include "xpm/observe.xpm"
#include "xpm/record.xpm"

#include "xpm/sfilter.xpm"
#include "xpm/sfilter-cfg.xpm"
#include "xpm/pfilter.xpm"
#include "xpm/pfilter-cfg.xpm"

#include "xpm/green-plus.xpm"
#include "xpm/red-minus.xpm"

#include "xpm/man-black.xpm"
#include "xpm/man-red.xpm"
#include "xpm/man-yellow.xpm"

#include "xpm/group-red.xpm"
#include "xpm/group-green.xpm"
#include "xpm/group-blue.xpm"

#include "xpm/buddy-red.xpm"
#include "xpm/buddy-green.xpm"
#include "xpm/buddy-blue.xpm"

#include "xpm/error.xpm"
#include "xpm/delete.xpm"

#include "xpm/server-na.xpm"
#include "xpm/server-up.xpm"
#include "xpm/server-down.xpm"
#include "xpm/server-to.xpm"
#include "xpm/server-error.xpm"
#include "xpm/locked.xpm"
#include "xpm/punkbuster.xpm"
#include "xpm/locked_punkbuster.xpm"
#undef static

struct pixmap update_pix;
struct pixmap refresh_pix;
struct pixmap refrsel_pix;
struct pixmap stop_pix;

struct pixmap connect_pix;
struct pixmap observe_pix;
struct pixmap record_pix;

struct pixmap sfilter_pix;
struct pixmap sfilter_cfg_pix;
struct pixmap pfilter_pix;
struct pixmap pfilter_cfg_pix;

struct pixmap gplus_pix;
struct pixmap rminus_pix;

struct pixmap man_black_pix;
struct pixmap man_red_pix;
struct pixmap man_yellow_pix;

struct pixmap group_pix[3];
struct pixmap buddy_pix[9];

struct pixmap error_pix;
struct pixmap delete_pix;

struct pixmap server_status[5];
struct pixmap locked_pix;
struct pixmap punkbuster_pix;
struct pixmap locked_punkbuster_pix;

int pixmap_height (struct pixmap *pix) {
	return gdk_pixbuf_get_height (pix->pixbuf);
}


int pixmap_width (struct pixmap *pix) {
	return gdk_pixbuf_get_width (pix->pixbuf);
}


void free_pixmap (struct pixmap *pixmap) {
	if (!pixmap)
		return;

	if (pixmap->pixbuf) {
		g_object_unref (G_OBJECT (pixmap->pixbuf));
		pixmap->pixbuf = NULL;
	}
#ifdef GUI_GTK2
	if (pixmap->pix) {
		gdk_pixmap_unref (pixmap->pix);
		pixmap->pix = NULL;
	}
	if (pixmap->mask) {
		gdk_bitmap_unref (pixmap->mask);
		pixmap->mask = NULL;
	}
#endif
}


static void create_pixmap (GtkWidget *widget, const char* file, struct pixmap *pix) {
	load_pixmap_as_pixmap(widget, file, pix);

	if (!pix->pixbuf) {
		pix->pixbuf = error_pix.pixbuf;
#ifdef GUI_GTK2
		pix->pix = error_pix.pix;
		pix->mask = error_pix.mask;
#endif
		g_object_ref (G_OBJECT (pix->pixbuf));
#ifdef GUI_GTK2
		gdk_pixmap_ref(pix->pix);
		gdk_bitmap_ref(pix->mask);
#endif
	}
}

void free_pixmaps (void) {
	unsigned i;

	free_pixmap (&update_pix);
	free_pixmap (&refresh_pix);
	free_pixmap (&refrsel_pix);
	free_pixmap (&stop_pix);

	free_pixmap (&connect_pix);
	free_pixmap (&observe_pix);
	free_pixmap (&record_pix);

	free_pixmap (&sfilter_pix);
	free_pixmap (&sfilter_cfg_pix);
	free_pixmap (&pfilter_pix);
	free_pixmap (&pfilter_cfg_pix);

	free_pixmap (&gplus_pix);
	free_pixmap (&rminus_pix);

	free_pixmap (&man_black_pix);
	free_pixmap (&man_red_pix);
	free_pixmap (&man_yellow_pix); // He's actually green

	for (i = 0; i < 3; i++)
		free_pixmap (&group_pix[i]);

	for (i = 0; i < 9; i++)
		free_pixmap (&buddy_pix[i]);

	free_pixmap (&error_pix);
	free_pixmap (&delete_pix);

	for (i = 0; i < 5; i++)
		free_pixmap (&server_status[i]);

	free_pixmap (&locked_pix);

	free_pixmap (&punkbuster_pix);
	free_pixmap (&locked_punkbuster_pix);

	for (i = LAN_SERVER; i < UNKNOWN_SERVER; i++) {
		free_pixmap(games[i].pix);
		g_free(games[i].pix);
		games[i].pix = NULL;
	}
}

/** \brief concatenate two pixmaps
 *
 * horizontal concatenation
 * @param window
 * @param dest destination pixmap
 * @param s1 first pixmap
 * @param s2 second pixmap
 * @returns dest for convenience
 */
struct pixmap* cat_pixmaps (GtkWidget *window, struct pixmap *dest, struct pixmap* s1, struct pixmap* s2) {
	int h1, w1, h2, w2;
	gboolean has_alpha;

	w1 = gdk_pixbuf_get_width (s1->pixbuf);
	h1 = gdk_pixbuf_get_height (s1->pixbuf);
	w2 = gdk_pixbuf_get_width (s2->pixbuf);
	h2 = gdk_pixbuf_get_height (s2->pixbuf);

	has_alpha = gdk_pixbuf_get_has_alpha (s1->pixbuf) || gdk_pixbuf_get_has_alpha (s2->pixbuf);

	dest->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, w1 + w2, MAX (h1, h2));

	gdk_pixbuf_copy_area (s1->pixbuf, 0, 0, w1, h1, dest->pixbuf, 0, 0);
	gdk_pixbuf_copy_area (s2->pixbuf, 0, 0, w2, h2, dest->pixbuf, w1, 0);

#ifdef GUI_GTK2
	gdk_pixbuf_render_pixmap_and_mask (dest->pixbuf, &dest->pix, &dest->mask, 255);
#endif

	return dest;
}


void init_pixmaps (GtkWidget *window) {
	unsigned i = 0;

	free_pixmaps ();

	if (!gtk_widget_get_realized (window))
		gtk_widget_realize (window);

	create_pixmap (window, "update.xpm", &update_pix);
	create_pixmap (window, "refresh.xpm", &refresh_pix);
	create_pixmap (window, "refrsel.xpm", &refrsel_pix);
	create_pixmap (window, "stop.xpm", &stop_pix);

	create_pixmap (window, "connect.xpm", &connect_pix);
	create_pixmap (window, "observe.xpm", &observe_pix);
	create_pixmap (window, "record.xpm", &record_pix);

	create_pixmap (window, "sfilter.xpm", &sfilter_pix);
	create_pixmap (window, "sfilter-cfg.xpm", &sfilter_cfg_pix);

	create_pixmap (window, "pfilter.xpm", &pfilter_pix);
	create_pixmap (window, "pfilter-cfg.xpm", &pfilter_cfg_pix);

	create_pixmap (window, "green-plus.xpm", &gplus_pix);
	create_pixmap (window, "red-minus.xpm", &rminus_pix);

	create_pixmap (window, "man-black.xpm", &man_black_pix);
	create_pixmap (window, "man-red.xpm", &man_red_pix);
	create_pixmap (window, "man-yellow.xpm", &man_yellow_pix);

	create_pixmap (window, "group-red.xpm", &group_pix[0]);
	create_pixmap (window, "group-green.xpm", &group_pix[1]);
	create_pixmap (window, "group-blue.xpm", &group_pix[2]);

	create_pixmap (window, "buddy-red.xpm", &buddy_pix[1]);
	create_pixmap (window, "buddy-green.xpm", &buddy_pix[2]);
	create_pixmap (window, "buddy-blue.xpm", &buddy_pix[4]);

	create_pixmap (window, "server-na.xpm", &server_status[0]);
	create_pixmap (window, "server-up.xpm", &server_status[1]);
	create_pixmap (window, "server-down.xpm", &server_status[2]);
	create_pixmap (window, "server-to.xpm", &server_status[3]);
	create_pixmap (window, "server-error.xpm", &server_status[4]);

	create_pixmap (window, "error.xpm", &error_pix);
	create_pixmap (window, "delete.xpm", &delete_pix);
	create_pixmap (window, "locked.xpm", &locked_pix);
	create_pixmap (window, "punkbuster.xpm", &punkbuster_pix);
	cat_pixmaps(window, &locked_punkbuster_pix, &punkbuster_pix, &locked_pix);

	for (i = LAN_SERVER; i < UNKNOWN_SERVER; i++) {
		struct pixmap* pix = NULL;

		pix = g_malloc0(sizeof(struct pixmap));

		if (games[i].icon) {
			create_pixmap(window, games[i].icon, pix);
		}

		games[i].pix = pix;
	}
}


void ensure_buddy_pix (GtkWidget *window, int n) {
	int width, height;
	struct pixmap *dest;

	if (!buddy_pix[1].pixbuf)  /* not initialized */
		return;

	if (n < 0 || n > 9 || buddy_pix[n].pixbuf)
		return;

	width = gdk_pixbuf_get_width (buddy_pix[1].pixbuf);
	height = gdk_pixbuf_get_height (buddy_pix[1].pixbuf);

	dest = &buddy_pix[n];
	dest->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);

	if (n & 1) {
		gdk_pixbuf_composite (buddy_pix[1].pixbuf, dest->pixbuf, 0, 0, width, height, 0, 0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
	}

	if (n & 2) {
		gdk_pixbuf_composite (buddy_pix[2].pixbuf, dest->pixbuf, 0, 0, width, height, 0, 0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
	}

	if (n & 4) {
		gdk_pixbuf_composite (buddy_pix[4].pixbuf, dest->pixbuf, 0, 0, width, height, 0, 0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
	}

#ifdef GUI_GTK2
	gdk_pixbuf_render_pixmap_and_mask (dest->pixbuf, &dest->pix, &dest->mask, 255);
#endif
}


static guint GdkColorToColor32 (GdkColor *color) {
	guint r = color->red >> 8;
	guint g = color->green >> 8;
	guint b = color->blue >> 8;
	guint a = 255;

	return ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | a;
}

void two_colors_pixmap (GdkWindow *window, int width, int height, GdkColor *top, GdkColor *bottom, struct pixmap *dest) {
	GdkPixbuf *half_pixbuf;
	guint color32;

	dest->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
	half_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height/2);

	color32 = GdkColorToColor32 (top);
	gdk_pixbuf_fill (half_pixbuf, color32);
	gdk_pixbuf_copy_area (half_pixbuf, 0, 0, width, height/2, dest->pixbuf, 0, 0);

	color32 = GdkColorToColor32 (bottom);
	gdk_pixbuf_fill (half_pixbuf, color32);
	gdk_pixbuf_copy_area (half_pixbuf, 0, 0, width, height/2, dest->pixbuf, 0, height/2);

	g_object_unref (G_OBJECT (half_pixbuf));

#ifdef GUI_GTK2
	gdk_pixbuf_render_pixmap_and_mask (dest->pixbuf, &dest->pix, &dest->mask, 255);
#endif
}


void create_server_pixmap (GtkWidget *window, struct pixmap *stype, int n, struct pixmap *dest) {
	ensure_buddy_pix (window, n);

	cat_pixmaps (window, dest, &buddy_pix[n], stype);
}


gboolean pixmap_cache_lookup (GSList *cache, struct pixmap *pix, unsigned key) {
	struct cached_pixmap *cp;
	GdkPixbuf *res_pixbuf = NULL;
#ifdef GUI_GTK2
	GdkPixmap *res_pix = NULL;
	GdkBitmap *res_mask = NULL;
#endif

	if (!pix)
		return FALSE;

	while (cache) {
		cp = (struct cached_pixmap *) cache->data;
		if (cp->key == key) {
			cp->weight += 2;
			res_pixbuf = cp->pixbuf;
#ifdef GUI_GTK2
			res_pix = cp->pix;
			res_mask = cp->mask;
#endif
			break;
		}
		cache = cache->next;
	}

	pix->pixbuf = res_pixbuf;
	if (res_pixbuf)
		g_object_ref (G_OBJECT (res_pixbuf));

#ifdef GUI_GTK2
	pix->pix = res_pix;
	if (res_pix)
		gdk_pixmap_ref (res_pix);

	pix->mask = res_mask;
	if (res_mask)
		gdk_bitmap_ref (res_mask);
#endif

	return (pix->pixbuf != NULL);
}


void pixmap_cache_add (GSList **cache, struct pixmap *pix, unsigned key) {
	struct cached_pixmap *cp;

	if (pix && cache) {
		cp = g_malloc0 (sizeof (struct cached_pixmap));

		cp->pixbuf = pix->pixbuf;
		g_object_ref (G_OBJECT (cp->pixbuf));

#ifdef GUI_GTK2
		cp->pix = pix->pix;
		gdk_pixmap_ref (cp->pix);

		cp->mask = pix->mask;
		if (cp->mask)
			gdk_bitmap_ref (cp->mask);
#endif

		cp->key = key;
		cp->weight = 10;

		*cache = g_slist_prepend (*cache, cp);
	}
}


static int cached_pixmap_cmp (const struct cached_pixmap *a,
		const struct cached_pixmap *b) {
	return b->weight - a->weight;   /* descending order */
}


static void free_cached_pixmap (struct cached_pixmap *cp) {

	g_object_unref (G_OBJECT (cp->pixbuf));
#ifdef GUI_GTK2
	gdk_pixmap_unref (cp->pix);

	if (cp->mask)
		gdk_bitmap_unref (cp->mask);
#endif

	g_free (cp);
}


void pixmap_cache_clear (GSList **cache, int maxitems) {
	GSList *tmp = NULL;
	GSList *last = NULL;
	struct cached_pixmap *cp;

	if (cache && *cache) {
		if (maxitems > 0) {
			*cache = g_slist_sort (*cache, (GCompareFunc) cached_pixmap_cmp);

			for (tmp = *cache; (maxitems-- > 0) && tmp; tmp = tmp->next) {
				cp = (struct cached_pixmap *) tmp->data;
				cp->weight /= 2;
				last = tmp;
			}

			if (last)
				last->next = NULL;
		}
		else {
			tmp = *cache;
			*cache = NULL;
		}

		g_slist_foreach (tmp, (GFunc) free_cached_pixmap, NULL);
		g_slist_free (tmp);
	}
}
